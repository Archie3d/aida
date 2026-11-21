include(${CMAKE_CURRENT_LIST_DIR}/ProcessLimits.cmake)
execute_process(COMMAND "${ABI_REPORT}" RESULT_VARIABLE status OUTPUT_VARIABLE abi TIMEOUT 10)
if(NOT status STREQUAL "0")
    message(FATAL_ERROR "C ABI probe failed: ${status}")
endif()
string(JSON native GET "${abi}" native_long_double)
if(native)
    include(${CMAKE_CURRENT_LIST_DIR}/RunTasking.cmake)
    return()
endif()

string(JSON target GET "${abi}" target)
string(JSON format GET "${abi}" c_floats "long double" format)
file(MAKE_DIRECTORY "${WORKDIR}")
set(ir "${WORKDIR}/unavailable.ssa")
file(REMOVE "${ir}")
execute_process(COMMAND "${ADAC}" -o "${ir}" "${UNAVAILABLE}"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT ${AIDA_COMPILE_TIMEOUT})
set(diagnostic "error: Interfaces.C.long_double is unsupported on ${target} (C long double format: ${format})")
if(NOT status STREQUAL "1" OR EXISTS "${ir}")
    message(FATAL_ERROR "Expected rejection before code generation: ${status}\n${output}${errors}")
endif()
foreach(line IN ITEMS 6 7 9)
    string(REGEX MATCHALL "interfaceslongdoubleunavailable\\.adb:${line}:[0-9]+: [^\n]+" mentions "${errors}")
    string(FIND "${mentions}" "${diagnostic}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "Expected a long_double format diagnostic at line ${line}:\n${errors}")
    endif()
endforeach()

# A limited view must preserve the same target restriction and must leave
# the other public types usable through their incomplete views.
set(limited "${WORKDIR}/interfaces_limited.ads")
file(WRITE "${limited}" [=[limited with Interfaces.C;
package Interfaces_Limited is
    type Int_Pointer is access all Interfaces.C.int;
end Interfaces_Limited;
]=])
execute_process(COMMAND "${ADAC}" -o "${WORKDIR}/available.ssa" "${limited}"
    RESULT_VARIABLE status ERROR_VARIABLE errors TIMEOUT ${AIDA_COMPILE_TIMEOUT})
if(NOT status STREQUAL "0" OR NOT errors STREQUAL "")
    message(FATAL_ERROR "Expected other limited types to remain usable: ${status}\n${errors}")
endif()
file(WRITE "${limited}" [=[limited with Interfaces.C;
package Interfaces_Limited is
    type Long_Double_Pointer is access all Interfaces.C.long_double;
end Interfaces_Limited;
]=])
execute_process(COMMAND "${ADAC}" -o "${ir}" "${limited}"
    RESULT_VARIABLE status ERROR_VARIABLE errors TIMEOUT ${AIDA_COMPILE_TIMEOUT})
string(FIND "${errors}" "${diagnostic}" found)
if(NOT status STREQUAL "1" OR EXISTS "${ir}" OR found EQUAL -1)
    message(FATAL_ERROR "Expected a format diagnostic for the limited view: ${status}\n${errors}")
endif()

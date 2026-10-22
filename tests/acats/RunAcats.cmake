# Executable C-tests only. Expected compile failures must name the known gap;
# crashes, timeouts, runtime failures and unexpected passes are never XFAILs.
foreach(required NAME ADA ROOT WORKDIR)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "RunAcats requires ${required}")
    endif()
endforeach()

set(work "${WORKDIR}/${NAME}")
file(MAKE_DIRECTORY "${work}")
set(program "${work}/${NAME}${CMAKE_EXECUTABLE_SUFFIX}")
set(extra_support "")
if(NAME STREQUAL "c354002")
    list(APPEND extra_support "${ROOT}/support/tctouch.ads" "${ROOT}/support/tctouch.adb")
endif()
execute_process(
    COMMAND "${ADA}" -o "${program}"
        "${ROOT}/support/report.ads" "${ROOT}/support/report.adb" ${extra_support}
        "${ROOT}/upstream/${NAME}.ada"
    WORKING_DIRECTORY "${work}"
    TIMEOUT 40
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors
)
file(WRITE "${work}/compile.log" "${output}${errors}")
if(DEFINED REJECTION AND NOT REJECTION STREQUAL "")
    if(status STREQUAL "0")
        message(FATAL_ERROR "XPASS ${NAME}: remove the expected failure and run the case")
    endif()
    string(FIND "${errors}" "${REJECTION}" found)
    if(NOT "${status}" MATCHES "^[1-9][0-9]*$" OR found EQUAL -1)
        message(FATAL_ERROR "Unexpected compilation failure (${status}):\n${output}${errors}")
    endif()
    string(REPLACE "${ROOT}/upstream/" "" diagnostics "${errors}")
    string(REPLACE "\r\n" "\n" diagnostics "${diagnostics}")
    file(READ "${ROOT}/${NAME}.diagnostics" expected_diagnostics)
    if(NOT diagnostics STREQUAL expected_diagnostics)
        message(FATAL_ERROR "Changed XFAIL diagnostics for ${NAME}:\n${diagnostics}")
    endif()
    message(STATUS "XFAIL ${NAME}: ${REJECTION}")
    return()
endif()
if(NOT status STREQUAL "0")
    message(FATAL_ERROR "Compilation failed (${status}):\n${output}${errors}")
endif()

execute_process(
    COMMAND "${program}"
    WORKING_DIRECTORY "${work}"
    TIMEOUT 10
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors
)
file(WRITE "${work}/run.log" "${output}${errors}")
string(TOUPPER "${NAME}" test_name)
string(REPLACE "\r\n" "\n" output "${output}")
if(NOT status STREQUAL "0" OR NOT errors STREQUAL ""
        OR NOT output STREQUAL "TEST ${test_name}\nPASSED\n")
    message(FATAL_ERROR "ACATS ${NAME} failed (${status}):\n${output}${errors}")
endif()
message(STATUS "PASS ${NAME}")

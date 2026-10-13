set(work "${WORKDIR}/forward-modes")
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${work}")
foreach(source forward_model.ads forward_model.adb forwardlibrary.adb)
    file(COPY "${FIXTURES}/${source}" DESTINATION "${work}")
endforeach()
set(math_library)
if(SEPARATE_LIBM)
    set(math_library -lm)
endif()

function(run)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE status
        OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Command failed: ${ARGV}\n${status}\n${output}${errors}")
    endif()
    set(actual "${output}" PARENT_SCOPE)
endfunction()

function(check_program program expected)
    run("${program}")
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "Expected '${expected}', got '${actual}'")
    endif()
endfunction()

function(build_modes expected)
    run("${ADA}" -D "${work}/objects" -o "${work}/apart" "${work}/forwardlibrary.adb")
    check_program("${work}/apart" "${expected}")
    run("${ADA}" -S -o "${work}/whole.s" "${work}/forwardlibrary.adb")
    run("${CC}" -pthread "${work}/whole.s" "${RUNTIME}" ${math_library} -o "${work}/whole")
    check_program("${work}/whole" "${expected}")
endfunction()

build_modes("library template checked\n 42\n 43\nlibrary forward instances ready\n")
# Both the library's deferred instance and the client's instance must change.
file(READ "${work}/forward_model.adb" body)
string(REPLACE "Offset : Integer := 40" "Offset : Integer := 50" body "${body}")
file(WRITE "${work}/forward_model.adb" "${body}")
build_modes("library template checked\n 52\n 53\nlibrary forward instances ready\n")

# Whole-program emission must preserve the separately tested failure cleanup.
run("${ADA}" -S -o "${work}/failure.s" "${FIXTURES}/elaborationlibrarycheck.adb")
run("${CC}" -pthread "${work}/failure.s" "${RUNTIME}" ${math_library} -o "${work}/failure")
execute_process(COMMAND "${work}/failure" RESULT_VARIABLE status
    OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 10)
if(NOT status EQUAL 1 OR NOT output STREQUAL "library object finalized\n"
    OR NOT errors STREQUAL "\nraised PROGRAM_ERROR\n")
    message(FATAL_ERROR "Whole-program elaboration failure mismatch: ${status}: ${output}${errors}")
endif()

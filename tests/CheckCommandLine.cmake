# Exercise both whole-program and separately bound entry points with real argv.
set(work "${WORKDIR}/commandline_arguments")
file(MAKE_DIRECTORY "${work}")
foreach(mode IN ITEMS separate whole)
    set(program "${work}/${mode}")
    if(mode STREQUAL "separate")
        execute_process(COMMAND "${ADA}" -o "${program}" "${SOURCE}"
            RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
    else()
        execute_process(COMMAND "${ADA}" -S -o "${program}.s" "${SOURCE}"
            RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
        if(status EQUAL 0)
            execute_process(COMMAND "${CC}" "${program}.s" "${RUNTIME}" -lm -o "${program}"
                RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
        endif()
    endif()
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "${mode} compilation failed: ${output}${errors}")
    endif()
    execute_process(COMMAND "${program}" "hello world" "" "--literal" "${program}"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
    if(NOT status EQUAL 42 OR NOT output STREQUAL "commandline: passed\n")
        message(FATAL_ERROR "${mode}: status=${status}, output=${output}, errors=${errors}")
    endif()
endforeach()

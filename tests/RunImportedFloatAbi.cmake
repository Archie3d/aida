file(MAKE_DIRECTORY "${WORKDIR}")
set(program "${WORKDIR}/importedfloatabi")
execute_process(COMMAND "${ADA}" --emit-ir -o "${program}.ssa" "${SOURCE}"
    RESULT_VARIABLE status)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Imported float ABI IR generation failed")
endif()
file(READ "${program}.ssa" ir)
foreach(pattern IN ITEMS
    "=s call [$]abiFloat[(]s "
    "=d call [$]abiDouble[(]d "
    "=s call [$]abiMixedFloat[(]s [^,]+, d [^,]+, s "
    "=d call [$]abiMixedDouble[(]d [^,]+, s [^,]+, d "
    "call [$]abiUpdate[(]s [^,]+, d [^,]+, l [^,]+, l ")
    if(NOT ir MATCHES "${pattern}")
        message(FATAL_ERROR "Missing correctly typed imported call: ${pattern}")
    endif()
endforeach()
execute_process(COMMAND "${ADA}" -S -o "${program}.s" "${SOURCE}"
    RESULT_VARIABLE status)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Imported float ABI assembly generation failed")
endif()
set(math_library)
if(SEPARATE_LIBM)
    set(math_library -lm)
endif()
execute_process(COMMAND "${CC}" "${program}.s" "${HELPERS}" "${RUNTIME}" ${math_library} -o "${program}"
    RESULT_VARIABLE status)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Imported float ABI link failed")
endif()
execute_process(COMMAND "${program}" RESULT_VARIABLE status OUTPUT_VARIABLE output)
file(READ "${EXPECTED}" expected_output)
if(NOT status EQUAL 0 OR NOT output STREQUAL expected_output)
    message(FATAL_ERROR "Imported float ABI regression failed: ${status}: ${output}")
endif()

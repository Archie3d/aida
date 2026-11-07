include(${CMAKE_CURRENT_LIST_DIR}/ProcessLimits.cmake)
file(MAKE_DIRECTORY "${WORKDIR}")
execute_process(COMMAND "${ADAC}" -D "${WORKDIR}" "${SOURCE}"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors
    TIMEOUT ${AIDA_COMPILE_TIMEOUT})
if(NOT status STREQUAL "0")
    message(FATAL_ERROR "Aggregate storage compilation failed: ${output}${errors}")
endif()
file(READ "${WORKDIR}/aggregatetemporaries.ssa" ir)
if(ir MATCHES "call \\$__ada_array_local")
    message(FATAL_ERROR "Small static aggregate temporaries require arena allocation")
endif()

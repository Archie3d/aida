find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
add_test(NAME robustness.process_protocol COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_harness.py)
add_test(NAME robustness.ir_instrumentation COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_ir.py)
add_test(NAME robustness.malformed_fuzz
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/fuzz.py
        --adac $<TARGET_FILE:adac> --work ${CMAKE_CURRENT_BINARY_DIR}/fuzz --iterations 192)
set_tests_properties(robustness.process_protocol robustness.ir_instrumentation PROPERTIES TIMEOUT 15 LABELS robustness)
set_tests_properties(robustness.malformed_fuzz PROPERTIES TIMEOUT 180 LABELS robustness)
if(NOT AIDA_SANITIZER STREQUAL "none" AND UNIX)
    add_test(NAME robustness.sanitized_tasking
        COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tasking.py
            --adac $<TARGET_FILE:adac> --qbe ${AIDA_REAL_QBE} --cc ${CMAKE_C_COMPILER}
            --runtime $<TARGET_FILE:adart> --sanitizer ${AIDA_SANITIZER}
            --work ${CMAKE_CURRENT_BINARY_DIR}/tasking)
    set_tests_properties(robustness.sanitized_tasking PROPERTIES TIMEOUT 600 LABELS "robustness;sanitizer")
endif()

# The runtime test exercises checked Access as a positive control. This fixture
# contains only Unchecked_Access and must retain null checks, but no static or
# dynamic object/tag accessibility checks, including an anonymous parameter.
file(MAKE_DIRECTORY "${WORKDIR}")
set(ir "${WORKDIR}/unchecked.ssa")
execute_process(COMMAND "${ADAC}" -o "${ir}" "${SOURCE}"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Unchecked_Access compilation failed: ${output}${errors}")
endif()
file(READ "${ir}" code)
foreach(helper __ada_accessibility_check __ada_tag_check_accessibility)
    string(FIND "${code}" "call $${helper}(" position)
    if(NOT position EQUAL -1)
        message(FATAL_ERROR "Unchecked_Access emitted ${helper}")
    endif()
endforeach()
string(FIND "${code}" "@nullaccess." null_check)
if(null_check EQUAL -1)
    message(FATAL_ERROR "Unchecked_Access lost null dereference checks")
endif()

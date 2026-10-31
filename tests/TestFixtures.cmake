# Locate explicitly named fixtures; this does not discover or register tests.
# Literal paths are preferred. Basenames support mixed-feature registration loops.
include_guard(GLOBAL)

file(GLOB_RECURSE ada_fixture_files LIST_DIRECTORIES FALSE
    "${CMAKE_CURRENT_LIST_DIR}/ada/*")
foreach(path IN LISTS ada_fixture_files)
    get_filename_component(name "${path}" NAME)
    set_property(GLOBAL APPEND PROPERTY "ADA_FIXTURE_${name}" "${path}")
endforeach()

function(ada_test_fixture output fixture)
    set(path "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/ada/${fixture}")
    if(IS_ABSOLUTE "${fixture}")
        set(path "${fixture}")
    endif()
    if(NOT EXISTS "${path}")
        get_property(matches GLOBAL PROPERTY "ADA_FIXTURE_${fixture}")
        list(LENGTH matches count)
        if(NOT count EQUAL 1)
            message(FATAL_ERROR "Fixture '${fixture}' needs an explicit path: ${matches}")
        endif()
        list(GET matches 0 path)
    endif()
    set(${output} "${path}" PARENT_SCOPE)
endfunction()

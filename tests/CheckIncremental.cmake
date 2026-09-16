# Script-mode invocations do not inherit the project's CMake policy settings.
cmake_minimum_required(VERSION 3.20)

# Builds a program made of several library units, then builds it again to check
# that a unit nobody touched keeps the object it already had, and that editing
# one unit rebuilds that unit alone.

if(NOT DEFINED NAME OR NOT DEFINED SOURCES OR NOT DEFINED MAIN)
    message(FATAL_ERROR "CheckIncremental.cmake requires NAME, SOURCES and MAIN")
endif()

string(REPLACE "|" ";" source_list "${SOURCES}")
set(work "${WORKDIR}/${NAME}")
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${work}")

# The sources are copied, since the test edits one of them.  Only the main
# subprogram is named on the command line; the rest are found beside it the way
# a with clause finds them.
foreach(source IN LISTS source_list)
    get_filename_component(name "${source}" NAME)
    configure_file("${source}" "${work}/${name}" COPYONLY)
endforeach()

set(program "${work}/${NAME}")
set(main "${work}/${MAIN}")

# Every unit qbe was asked to translate in one build, taken from the commands
# the driver printed.
function(rebuilt_units commands result)
    string(REGEX MATCHALL "[^ \n]+\\.ssa" paths "${commands}")
    set(units "")
    foreach(path IN LISTS paths)
        get_filename_component(unit "${path}" NAME)
        list(APPEND units "${unit}")
    endforeach()
    list(REMOVE_DUPLICATES units)
    list(SORT units)
    set(${result} "${units}" PARENT_SCOPE)
endfunction()

macro(build label)
    execute_process(
        COMMAND "${ADA}" -v -o "${program}" "${main}"
        RESULT_VARIABLE build_status
        OUTPUT_VARIABLE build_output
        ERROR_VARIABLE build_commands
    )
    if(NOT build_status EQUAL 0)
        message(FATAL_ERROR "${label} build failed:\n${build_output}${build_commands}")
    endif()
    rebuilt_units("${build_commands}" translated)
endmacro()

macro(check_output label expectation)
    execute_process(
        COMMAND "${program}"
        WORKING_DIRECTORY "${work}"
        RESULT_VARIABLE run_status
        OUTPUT_VARIABLE actual
    )
    file(READ "${expectation}" expected)
    if(NOT run_status EQUAL 0 OR NOT actual STREQUAL expected)
        message(FATAL_ERROR
            "${label} run failed: ${run_status}\n--- expected ---\n${expected}\n--- actual ---\n${actual}")
    endif()
endmacro()

build("first")
check_output("first" "${EXPECTED}")
if(NOT translated)
    message(FATAL_ERROR "the first build translated no units at all")
endif()
if(NOT "${MODIFY_UNIT}.ssa" IN_LIST translated)
    message(FATAL_ERROR "the first build did not translate ${MODIFY_UNIT}.ssa, only ${translated}")
endif()

# Nothing changed, so every object is still the one to use.
build("second")
if(translated)
    message(FATAL_ERROR "an unchanged program rebuilt ${translated}")
endif()
check_output("second" "${EXPECTED}")

# One body says something different now, and it is the only unit affected.
file(READ "${work}/${MODIFY}" text)
string(REPLACE "${FROM}" "${TO}" text "${text}")
file(WRITE "${work}/${MODIFY}" "${text}")

build("third")
if(NOT translated STREQUAL "${MODIFY_UNIT}.ssa")
    message(FATAL_ERROR "editing ${MODIFY} rebuilt ${translated}, expected only ${MODIFY_UNIT}.ssa")
endif()
check_output("third" "${REBUILT}")

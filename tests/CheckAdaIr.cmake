# Compares the QBE intermediate language produced by adac with a recorded
# reference, so that changes to code generation are noticed.  A program is
# compiled one library unit at a time, so the reference is a directory holding
# the file each unit was emitted into.

string(REPLACE "|" ";" source_list "${SOURCES}")
set(work "${WORKDIR}/${NAME}/ir")
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${work}")

execute_process(
    COMMAND "${ADAC}" -D "${work}" ${source_list}
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_VARIABLE errors
)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "adac failed:\n${output}${errors}")
endif()

file(GLOB expected_units RELATIVE "${GOLDEN}" "${GOLDEN}/*.ssa")
file(GLOB actual_units RELATIVE "${work}" "${work}/*.ssa")
list(SORT expected_units)
list(SORT actual_units)
if(NOT expected_units STREQUAL actual_units)
    message(FATAL_ERROR
        "the set of emitted units differs from ${GOLDEN}\n"
        "--- expected ---\n${expected_units}\n--- actual ---\n${actual_units}")
endif()

foreach(unit IN LISTS expected_units)
    file(READ "${work}/${unit}" actual)
    file(READ "${GOLDEN}/${unit}" expected)
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR
            "generated IL for ${unit} differs from ${GOLDEN}/${unit}\n"
            "--- expected ---\n${expected}\n--- actual ---\n${actual}")
    endif()
endforeach()

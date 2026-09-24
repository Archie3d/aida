# Compiles a program as a whole, then one library unit at a time, and checks
# that each unit came out the same either way.  Compiling units apart is only
# sound if what a unit generates does not depend on what happened to be
# compiled beside it.

if(NOT DEFINED NAME OR NOT DEFINED SOURCE)
    message(FATAL_ERROR "CheckDeterminism.cmake requires NAME and SOURCE")
endif()

set(work "${WORKDIR}/determinism/${NAME}")
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${work}/whole")
file(MAKE_DIRECTORY "${work}/apart")

execute_process(
    COMMAND "${ADAC}" -D "${work}/whole" "${SOURCE}"
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_VARIABLE errors
)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "compiling the whole program failed:\n${output}${errors}")
endif()

# The scan records each unit and the files it was written in, the body last.
file(STRINGS "${work}/whole/units.manifest" lines)
set(units "")
set(current "")
foreach(line IN LISTS lines)
    if(line MATCHES "^unit (.*)$")
        set(current "${CMAKE_MATCH_1}")
        list(APPEND units "${current}")
    elseif(line MATCHES "^source (.*)$")
        set("source.${current}" "${CMAKE_MATCH_1}")
    endif()
endforeach()

if(NOT units)
    message(FATAL_ERROR "no units were recorded for ${SOURCE}")
endif()

foreach(unit IN LISTS units)
    execute_process(
        COMMAND "${ADAC}" -c -D "${work}/apart" "${source.${unit}}"
        RESULT_VARIABLE status
        OUTPUT_VARIABLE output
        ERROR_VARIABLE errors
    )
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "compiling ${unit} on its own failed:\n${output}${errors}")
    endif()

    file(READ "${work}/whole/${unit}.ssa" together)
    file(READ "${work}/apart/${unit}.ssa" alone)
    if(NOT together STREQUAL alone)
        message(FATAL_ERROR
            "${unit} came out differently when compiled on its own\n"
            "--- with the rest ---\n${together}\n--- on its own ---\n${alone}")
    endif()
endforeach()

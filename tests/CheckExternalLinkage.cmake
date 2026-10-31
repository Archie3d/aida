file(REMOVE_RECURSE "${WORKDIR}")
file(MAKE_DIRECTORY "${WORKDIR}")

function(run)
    execute_process(COMMAND ${ARGV} WORKING_DIRECTORY "${WORKDIR}"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Command failed: ${ARGV}\n${status}\n${output}${errors}")
    endif()
    set(actual "${output}" PARENT_SCOPE)
endfunction()

function(reject diagnostic)
    execute_process(COMMAND "${ADA}" ${ARGN} WORKING_DIRECTORY "${WORKDIR}"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
    if(status EQUAL 0 OR NOT errors MATCHES "${diagnostic}")
        message(FATAL_ERROR "Expected rejection (${diagnostic}): ${ARGN}\n${status}\n${output}${errors}")
    endif()
endfunction()

# Spaces exercise argument quoting for both objects and archives.
foreach(part Helper Middle Tail)
    run("${CC}" -c "${SOURCES}/ExternalLink${part}.c" -o "${part} input.o")
    run("${AR}" qc "${part} library.a" "${part} input.o")
    run("${RANLIB}" "${part} library.a")
endforeach()

foreach(mode objects archives mixed)
    if(mode STREQUAL objects)
        set(inputs "Helper input.o" "Middle input.o" "Tail input.o")
    elseif(mode STREQUAL archives)
        set(inputs "Helper library.a" "Middle library.a" "Tail library.a")
    else()
        set(inputs "Helper input.o" "Middle library.a" "Tail library.a")
    endif()
    run("${ADA}" -o "${mode}" "${SOURCES}/ada/driver/externallink.adb" ${inputs})
    run("${WORKDIR}/${mode}${EXE_SUFFIX}")
    if(NOT actual STREQUAL "external linkage passed\n")
        message(FATAL_ERROR "Unexpected ${mode} output: ${actual}")
    endif()
endforeach()

# External inputs before/after the source must not select the main or stem.
run("${ADA}" "Helper input.o" "${SOURCES}/ada/driver/externallink.adb"
    "Middle library.a" "Tail library.a")
run("${WORKDIR}/externallink${EXE_SUFFIX}")
if(NOT actual STREQUAL "external linkage passed\n")
    message(FATAL_ERROR "Unexpected default output: ${actual}")
endif()

# Ada sources with archive extensions (as in upstream ACATS) remain usable.
configure_file("${SOURCES}/ada/driver/externallink.adb" "${WORKDIR}/externallink.a" COPYONLY)
run("${ADA}" -o explicit --source "${WORKDIR}/externallink.a"
    "Helper input.o" "Middle library.a" "Tail library.a")
run("${WORKDIR}/explicit${EXE_SUFFIX}")
if(NOT actual STREQUAL "external linkage passed\n")
    message(FATAL_ERROR "Unexpected explicit-source output: ${actual}")
endif()
reject("missing file name after '--source'" --source)

# Relinking must pick up changed external code while reusing cached Ada units.
file(WRITE "${WORKDIR}/replacement.c"
    "int __ada_test_external_tail(void) { return 39; }\n")
run("${CC}" -c "replacement.c" -o "Tail input.o")
run("${ADA}" -o objects "${SOURCES}/ada/driver/externallink.adb"
    "Helper input.o" "Middle input.o" "Tail input.o")
execute_process(COMMAND "${WORKDIR}/objects${EXE_SUFFIX}" RESULT_VARIABLE status TIMEOUT 10
    OUTPUT_QUIET ERROR_QUIET)
if(NOT status MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "Changed external object was not used: ${status}")
endif()

reject("at least one Ada source is required" "Helper input.o")
reject("at least one Ada source is required" "Helper library.a")
foreach(stage -S --emit-ir)
    foreach(input "Helper input.o" "Helper library.a")
        reject("inputs require executable output" ${stage}
            "${SOURCES}/ada/driver/externallink.adb" "${input}")
    endforeach()
endforeach()

# A missing linker input must propagate the linker's failure.
execute_process(COMMAND "${ADA}" -o "${WORKDIR}/missing"
    "${SOURCES}/ada/driver/externallink.adb" "${WORKDIR}/missing.o"
    WORKING_DIRECTORY "${WORKDIR}" RESULT_VARIABLE status
    OUTPUT_QUIET ERROR_QUIET TIMEOUT 30)
if(NOT status EQUAL 1)
    message(FATAL_ERROR "Missing external input should fail the build: ${status}")
endif()

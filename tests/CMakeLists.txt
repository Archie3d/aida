set(ADA_TEST_WORK_DIR ${CMAKE_CURRENT_BINARY_DIR}/work)

# add_ada_test(<name> SOURCES <file>...)
#
# Builds the sources into a program with the ada driver and compares its
# standard output with <name>.expected.
function(add_ada_test name)
    cmake_parse_arguments(ADA_TEST "" "EXIT_CODE;STDERR" "SOURCES" ${ARGN})
    if(NOT DEFINED ADA_TEST_EXIT_CODE)
        set(ADA_TEST_EXIT_CODE 0)
    endif()
    set(stderr_expected "")
    if(DEFINED ADA_TEST_STDERR)
        set(stderr_expected "${CMAKE_CURRENT_SOURCE_DIR}/ada/${ADA_TEST_STDERR}")
    endif()

    set(sources "")
    foreach(source ${ADA_TEST_SOURCES})
        if(sources STREQUAL "")
            set(sources "${CMAKE_CURRENT_SOURCE_DIR}/ada/${source}")
        else()
            set(sources "${sources}|${CMAKE_CURRENT_SOURCE_DIR}/ada/${source}")
        endif()
    endforeach()

    add_test(NAME ada.${name}
        COMMAND ${CMAKE_COMMAND}
            -DNAME=${name}
            -DEXIT_CODE=${ADA_TEST_EXIT_CODE}
            -DSTDERR_EXPECTED=${stderr_expected}
            -DADA=$<TARGET_FILE:ada>
            -DSOURCES=${sources}
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${name}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunAdaTest.cmake
    )
endfunction()

# add_ada_error_test(<name> SOURCES <file>...)
#
# Compiles sources that are meant to be rejected and compares the diagnostics
# with <name>.expected.
function(add_ada_error_test name)
    cmake_parse_arguments(ADA_ERROR "" "" "SOURCES" ${ARGN})

    set(sources "")
    foreach(source ${ADA_ERROR_SOURCES})
        if(sources STREQUAL "")
            set(sources "${CMAKE_CURRENT_SOURCE_DIR}/ada/${source}")
        else()
            set(sources "${sources}|${CMAKE_CURRENT_SOURCE_DIR}/ada/${source}")
        endif()
    endforeach()

    add_test(NAME error.${name}
        COMMAND ${CMAKE_COMMAND}
            -DNAME=${name}
            -DADAC=$<TARGET_FILE:adac>
            -DSOURCES=${sources}
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${name}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckAdaErrors.cmake
    )
endfunction()

# add_ada_ir_test(<name> SOURCES <file>...)
#
# Compares the generated QBE IL against the units recorded in golden/<name>/.
function(add_ada_ir_test name)
    cmake_parse_arguments(ADA_IR "" "" "SOURCES" ${ARGN})

    set(sources "")
    foreach(source ${ADA_IR_SOURCES})
        if(sources STREQUAL "")
            set(sources "${CMAKE_CURRENT_SOURCE_DIR}/ada/${source}")
        else()
            set(sources "${sources}|${CMAKE_CURRENT_SOURCE_DIR}/ada/${source}")
        endif()
    endforeach()

    add_test(NAME ir.${name}
        COMMAND ${CMAKE_COMMAND}
            -DNAME=${name}
            -DADAC=$<TARGET_FILE:adac>
            -DSOURCES=${sources}
            -DGOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/golden/${name}
            -DWORKDIR=${ADA_TEST_WORK_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckAdaIr.cmake
    )
endfunction()

add_ada_test(hello SOURCES hello.adb)
add_ada_test(arithmetic SOURCES arithmetic.adb)
add_ada_test(control SOURCES control.adb)
add_ada_test(subprograms SOURCES subprograms.adb)
add_ada_test(scalarcopy SOURCES scalarcopy.adb)
add_ada_test(scalarcopychecks SOURCES scalarcopychecks.adb)
add_ada_test(occurrencebindings SOURCES occurrencebindings.adb)
add_ada_test(occurrencenowith SOURCES occurrencenowith.adb)
add_ada_error_test(occurrencebindingerrors SOURCES occurrencebindingerrors.adb)
add_ada_test(exceptionmessages SOURCES exceptionmessages.adb)
add_ada_test(exceptiondiagnostics SOURCES exceptiondiagnostics.adb)
add_ada_error_test(handlerchoiceerrors SOURCES handlerchoiceerrors.adb)
add_ada_error_test(handleremptyerrors SOURCES handleremptyerrors.adb)
add_ada_error_test(handlerparterrors SOURCES handlerparterrors.adb)
add_ada_error_test(handlerothererrors SOURCES handlerothererrors.adb)
add_ada_test(savedoccurrences SOURCES savedoccurrences.adb)
add_ada_error_test(savedoccurrenceerrors SOURCES savedoccurrenceerrors.adb)
add_ada_error_test(exceptionmessageerrors SOURCES exceptionmessageerrors.adb)
add_ada_test(unhandledmessage SOURCES unhandledmessage.adb EXIT_CODE 1 STDERR unhandledmessage.stderr)
add_ada_test(arrayidentity SOURCES arrayidentity.adb)
add_ada_test(groupedfields SOURCES groupedfields.adb)
add_ada_test(groupedfieldchecks SOURCES groupedfieldchecks.adb)
add_ada_error_test(groupedfielderrors SOURCES groupedfielderrors.adb)
add_ada_error_test(arrayidentityerrors SOURCES arrayidentityerrors.adb)
add_ada_error_test(arrayliteralambiguity SOURCES arrayliteralambiguity.adb)
add_ada_error_test(scalarcopyerrors SOURCES scalarcopyerrors.adb)
add_ada_test(composite SOURCES composite.adb)
add_ada_test(packages SOURCES packages.adb)
add_ada_test(checks SOURCES checks.adb)
add_ada_test(strings SOURCES strings.adb)
add_ada_test(ranges SOURCES ranges.adb)
add_ada_test(floats SOURCES floats.adb)
add_ada_test(bases SOURCES bases.adb)
add_ada_test(textfiles SOURCES textfiles.adb)
add_ada_test(generics SOURCES generics.adb)
add_ada_test(generic_subprograms SOURCES generic_subprograms.adb)
add_ada_test(generic_defaults SOURCES generic_defaults.adb)
add_ada_test(generic_library SOURCES generic_library.adb)
add_ada_test(seqio SOURCES seqio.adb)
add_ada_test(directio SOURCES directio.adb)
add_ada_test(streamio SOURCES streamio.adb)
add_ada_test(typeio SOURCES typeio.adb)
add_ada_test(enumio SOURCES enumio.adb)
add_ada_test(storage SOURCES storage.adb)
add_ada_test(cases SOURCES cases.adb)
add_ada_test(access SOURCES access.adb)
add_ada_test(private SOURCES private.adb)
add_ada_test(variants SOURCES variants.adb)

add_ada_error_test(floaterrors SOURCES floaterrors.adb)
add_ada_error_test(baseerrors SOURCES baseerrors.adb)
add_ada_error_test(genericerrors SOURCES genericerrors.adb)
add_ada_error_test(generic_subprogram_errors SOURCES generic_subprogram_errors.adb)
add_ada_error_test(ioerrors SOURCES ioerrors.adb)
add_ada_error_test(caseerrors SOURCES caseerrors.adb)
add_ada_error_test(privateerrors SOURCES privateerrors.adb)
add_ada_error_test(varianterrors SOURCES varianterrors.adb)

add_ada_ir_test(hello SOURCES hello.adb)

# add_determinism_test(<name> SOURCE <file>)
#
# Checks that a unit compiled on its own comes out the same as when the whole
# program was compiled at once.
function(add_determinism_test name)
    cmake_parse_arguments(DETERMINISM "" "SOURCE" "" ${ARGN})
    add_test(NAME determinism.${name}
        COMMAND ${CMAKE_COMMAND}
            -DNAME=${name}
            -DADAC=$<TARGET_FILE:adac>
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${DETERMINISM_SOURCE}
            -DWORKDIR=${ADA_TEST_WORK_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckDeterminism.cmake
    )
endfunction()

add_determinism_test(private SOURCE private.adb)
add_determinism_test(generics SOURCE generics.adb)
add_determinism_test(generic_subprograms SOURCE generic_subprograms.adb)
add_determinism_test(generic_library SOURCE generic_library.adb)
add_determinism_test(packages SOURCE packages.adb)

# Builds a program twice over to check that a unit nobody touched keeps the
# object it already had, and that a changed body rebuilds that unit alone.
add_test(NAME ada.incremental
    COMMAND ${CMAKE_COMMAND}
        -DNAME=incremental
        -DADA=$<TARGET_FILE:ada>
        -DSOURCES=${CMAKE_CURRENT_SOURCE_DIR}/ada/incremental.adb|${CMAKE_CURRENT_SOURCE_DIR}/ada/incrhelper.ads|${CMAKE_CURRENT_SOURCE_DIR}/ada/incrhelper.adb
        -DMAIN=incremental.adb
        -DMODIFY=incrhelper.adb
        -DMODIFY_UNIT=incrhelper
        "-DFROM=return 1"
        "-DTO=return 2"
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/incremental.expected
        -DREBUILT=${CMAKE_CURRENT_SOURCE_DIR}/ada/incremental.rebuilt.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckIncremental.cmake
)

add_ada_test(overflow32 SOURCES overflow32.adb)

add_ada_test(overflow64 SOURCES overflow64.adb)

add_ada_test(integerwidth SOURCES integerwidth.adb)

add_ada_test(integerdivision SOURCES integerdivision.adb)

add_ada_test(integerconversions SOURCES integerconversions.adb)

add_ada_test(integerloops SOURCES integerloops.adb)

add_ada_test(integerstatic SOURCES integerstatic.adb)

add_ada_test(longintegerio SOURCES longintegerio.adb)

add_ada_test(overloads SOURCES overloads.adb)
add_ada_test(operators SOURCES operators.adb)
add_ada_test(operatorprofiles SOURCES operatorprofiles.adb)
add_ada_test(operatorunits SOURCES operatorunits.adb)
add_ada_error_test(operatorerrors SOURCES operatorerrors.adb)
add_ada_error_test(operatorambiguity SOURCES operatorambiguity.adb)
add_ada_error_test(operatordesignatorerrors SOURCES operatordesignatorerrors.adb)
add_ada_error_test(operatorclosingerrors SOURCES operatorclosingerrors.adb)
add_ada_test(nestedoverloads SOURCES nestedoverloads.adb)
add_ada_error_test(nestedoverloaderrors SOURCES nestedoverloaderrors.adb)

add_ada_error_test(overloaderrors SOURCES overloaderrors.adb)

add_test(NAME harness.exit_status
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckExitStatus.cmake
)

add_ada_test(arraycompare SOURCES arraycompare.adb)
add_ada_test(recordcompare SOURCES recordcompare.adb)
add_ada_error_test(comparisonerrors SOURCES comparisonerrors.adb)

add_ada_test(defaultcalls SOURCES defaultcalls.adb)
add_ada_test(parameterless SOURCES parameterless.adb)
add_ada_error_test(parameterlesserrors SOURCES parameterlesserrors.adb)

add_ada_error_test(defaulterrors SOURCES defaulterrors.adb)

add_ada_test(compositereturns SOURCES compositereturns.adb)
add_ada_error_test(returnerrors SOURCES returnerrors.adb)

add_ada_test(functionfallthrough SOURCES functionfallthrough.adb)
add_ada_test(reraise SOURCES reraise.adb)
add_ada_error_test(exceptionusageerrors SOURCES exceptionusageerrors.adb)
add_ada_test(elaborationfailure SOURCES elaborationfailure.adb
    EXIT_CODE 1 STDERR elaborationfailure.stderr)
add_ada_test(elaborationbodyfailure SOURCES elaborationbodyfailure.adb
    EXIT_CODE 1 STDERR elaborationbodyfailure.stderr)
add_ada_test(unhandledreraise SOURCES unhandledreraise.adb
    EXIT_CODE 1 STDERR unhandledreraise.stderr)

add_ada_test(elaborationorder SOURCES elaborationorder.adb)
add_ada_test(packagehandlers SOURCES packagehandlers.adb)
add_ada_test(packagedeclarationfailure SOURCES packagedeclarationfailure.adb
    EXIT_CODE 1 STDERR packagedeclarationfailure.stderr)
add_ada_test(packagehandlerfailure SOURCES packagehandlerfailure.adb
    EXIT_CODE 1 STDERR packagehandlerfailure.stderr)
add_ada_test(genericelaborationorder SOURCES genericelaborationorder.adb)

add_ada_test(localpackages SOURCES localpackages.adb)
add_ada_test(localgenericstate SOURCES localgenericstate.adb)
add_ada_test(localpackagestartup SOURCES localpackagestartup.adb)
add_ada_test(localpackagefailure SOURCES localpackagefailure.adb)

add_ada_test(dynamicarrays SOURCES dynamicarrays.adb)
add_ada_test(dynamicarrayvalues SOURCES dynamicarrayvalues.adb)
add_ada_test(dynamicarrayfailures SOURCES dynamicarrayfailures.adb)
add_ada_error_test(dynamicarrayerrors SOURCES dynamicarrayerrors.adb)
add_ada_error_test(dynamicaggregateerrors SOURCES dynamicaggregateerrors.adb)

add_executable(array_storage_test ArrayStorageTest.c)
target_link_libraries(array_storage_test PRIVATE adart)
if(UNIX)
    target_link_libraries(array_storage_test PRIVATE m)
endif()
add_test(NAME runtime.array_storage COMMAND array_storage_test)
add_executable(float_formatting_test FloatFormattingTest.c)
target_link_libraries(float_formatting_test PRIVATE adart)
if(UNIX)
    target_link_libraries(float_formatting_test PRIVATE m)
endif()
add_test(NAME runtime.float_formatting COMMAND float_formatting_test)
add_ada_test(arraylifetimes SOURCES arraylifetimes.adb)
add_ada_test(runtimeaggregates SOURCES runtimeaggregates.adb)
add_ada_test(runtimeaggregatechecks SOURCES runtimeaggregatechecks.adb)
add_ada_test(inferredaggregates SOURCES inferredaggregates.adb)
add_ada_test(inferredaggregatechecks SOURCES inferredaggregatechecks.adb)
add_ada_test(matrices SOURCES matrices.adb)
add_ada_test(matrixshapes SOURCES matrixshapes.adb)
add_ada_error_test(matrixerrors SOURCES matrixerrors.adb)
add_ada_test(runtimematrices SOURCES runtimematrices.adb)
add_ada_test(runtimematrixchecks SOURCES runtimematrixchecks.adb)
add_ada_error_test(runtimematrixerrors SOURCES runtimematrixerrors.adb)
add_ada_error_test(inferredmatrixerrors SOURCES inferredmatrixerrors.adb)
add_ada_test(inferredmatrices SOURCES inferredmatrices.adb)
add_ada_test(inferredmatrixchecks SOURCES inferredmatrixchecks.adb)
add_ada_error_test(matrixaggregateerrors SOURCES matrixaggregateerrors.adb)

# Semantic and boundary regression cases.
add_ada_test(shortcircuit SOURCES shortcircuit.adb)
add_ada_test(booleanoperators SOURCES booleanoperators.adb)
add_ada_test(membership SOURCES membership.adb)
add_ada_test(loopedges SOURCES loopedges.adb)
add_ada_test(sliceassignment SOURCES sliceassignment.adb)
add_ada_test(discreteattributes SOURCES discreteattributes.adb)
add_ada_test(signeddivision SOURCES signeddivision.adb)
add_ada_test(nestedcaptures SOURCES nestedcaptures.adb)
add_ada_test(accesschecks SOURCES accesschecks.adb)
add_ada_test(valueparsing SOURCES valueparsing.adb)
add_ada_test(textfileedges SOURCES textfileedges.adb)
add_ada_test(streambounds SOURCES streambounds.adb)
add_ada_error_test(returnusageerrors SOURCES returnusageerrors.adb)
add_ada_error_test(limitedprivateerrors SOURCES limitedprivateerrors.adb)
add_ada_error_test(declarationerrors SOURCES declarationerrors.adb)
add_ada_error_test(stringliteralerrors SOURCES stringliteralerrors.adb)
add_ada_error_test(delimitererrors SOURCES delimitererrors.adb)
add_ada_test(valuebased SOURCES valuebased.adb)
add_ada_test(streamrawedges SOURCES streamrawedges.adb)

add_ada_error_test(identifiererrors SOURCES identifiererrors.adb)
add_ada_error_test(closingnameerrors SOURCES closingnameerrors.adb)
add_ada_error_test(duplicatedeclarationerrors SOURCES duplicatedeclarationerrors.adb)

include(${CMAKE_CURRENT_SOURCE_DIR}/BasePowerTests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/ImportedFloatAbiTests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/NumericsPrecisionTests.cmake)

add_ada_test(runtimearraytypes SOURCES runtimearraytypes.adb)
add_ada_test(runtimearraytypechecks SOURCES runtimearraytypechecks.adb)
add_ada_error_test(runtimearraytypeerrors SOURCES runtimearraytypeerrors.adb)
add_ada_error_test(runtimearraylibraryerrors SOURCES runtimearraylibraryerrors.ads)

add_ada_test(runtimescalars SOURCES runtimescalars.adb)
add_ada_test(runtimescalarchecks SOURCES runtimescalarchecks.adb)
add_ada_error_test(runtimescalarerrors SOURCES runtimescalarerrors.adb)
add_ada_error_test(runtimescalarlibraryerrors SOURCES runtimescalarlibraryerrors.ads)

add_ada_test(handlerchoices SOURCES handlerchoices.adb)
add_ada_test(reverseloops SOURCES reverseloops.adb)

foreach(case generic order compiler paths binding)
    add_test(NAME separate.${case}
        COMMAND ${CMAKE_COMMAND}
            -DCASE=${case}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/ada/separate
            -DWORKDIR=${ADA_TEST_WORK_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckSeparateCompilation.cmake
    )
endforeach()

add_ada_test(modular SOURCES modular.adb)
add_ada_test(modularchecks SOURCES modularchecks.adb)
add_ada_error_test(modularerrors SOURCES modularerrors.adb)

add_ada_test(generic_contracts SOURCES generic_contracts.adb)
add_ada_error_test(generic_contract_errors SOURCES generic_contract_errors.adb)
add_determinism_test(generic_contracts SOURCE generic_contracts.adb)
add_ada_error_test(generic_separate_contract_errors
    SOURCES separate/contract_bad.ads separate/contract_bad.adb)

# Character and string standard-library packages.
add_ada_test(characterhandling SOURCES characterhandling.adb)
add_ada_test(stringmaps SOURCES stringmaps.adb)
add_ada_test(fixedstrings SOURCES fixedstrings.adb)
add_ada_test(fixedstringchecks SOURCES fixedstringchecks.adb)
add_determinism_test(fixedstrings SOURCE fixedstrings.adb)

add_ada_test(commandline SOURCES commandline.adb)
add_ada_test(commandlinefailure SOURCES commandlinefailure.adb EXIT_CODE 1)
add_test(NAME runtime.commandline_arguments
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/commandline.adb
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckCommandLine.cmake
)

add_ada_test(boundedstrings SOURCES boundedstrings.adb)
add_determinism_test(boundedstrings SOURCE boundedstrings.adb)
add_ada_test(genericcapacity SOURCES genericcapacity.adb)
add_ada_error_test(genericcapacityerrors SOURCES genericcapacityerrors.adb)

add_ada_test(generic_objects SOURCES generic_objects.adb)
add_determinism_test(generic_objects SOURCE generic_objects.adb)
add_ada_test(generic_object_checks SOURCES generic_object_checks.adb)
add_ada_error_test(generic_object_errors SOURCES generic_object_errors.adb)
add_ada_error_test(generic_object_modes SOURCES generic_object_modes.adb)
add_ada_test(generic_object_library SOURCES generic_object_library.adb)
add_ada_test(generic_object_nested SOURCES generic_object_nested.adb)

add_ada_test(generic_composites SOURCES generic_composites.adb)
add_determinism_test(generic_composites SOURCE generic_composites.adb)
add_ada_test(generic_dynamic_objects SOURCES generic_dynamic_objects.adb)
add_determinism_test(generic_dynamic_objects SOURCE generic_dynamic_objects.adb)
add_ada_test(generic_composite_checks SOURCES generic_composite_checks.adb)
add_ada_error_test(generic_composite_errors SOURCES generic_composite_errors.adb)
add_ada_test(generic_composite_library SOURCES generic_composite_library.adb)
add_ada_error_test(generic_composite_library_errors SOURCES generic_composite_library_errors.adb)

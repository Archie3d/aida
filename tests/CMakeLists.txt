set(ADA_TEST_WORK_DIR ${CMAKE_CURRENT_BINARY_DIR}/work)

add_subdirectory(acats)

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

add_ada_test(reservedwordboundaries SOURCES reservedwordboundaries.adb)
add_ada_error_test(reservedworderrors SOURCES reservedworderrors.adb)
add_ada_error_test(reservedconstructerrors SOURCES reservedconstructerrors.adb)
add_ada_error_test(reservedgenericerrors SOURCES reservedgenericerrors.adb)
add_ada_error_test(genericformalerrors SOURCES genericformalerrors.adb)
add_ada_error_test(genericformalsyntaxerrors SOURCES genericformalsyntaxerrors.adb)
add_ada_error_test(deallocationformalerrors SOURCES deallocationformalerrors.adb)
add_ada_error_test(unsupportedrecoveryerrors SOURCES unsupportedrecoveryerrors.adb)
add_ada_error_test(unsupportedconstructerrors SOURCES unsupportedconstructerrors.adb)
add_ada_error_test(unsupportedcontexterrors SOURCES unsupportedcontexterrors.adb)

add_ada_test(subprogramcompletions SOURCES subprogramcompletions.adb)
add_ada_error_test(missingbodyerrors SOURCES missingbodyerrors.adb)
add_ada_error_test(integerliteralerrors SOURCES integerliteralerrors.adb)
add_ada_test(integerliteralbounds SOURCES integerliteralbounds.adb)
add_ada_test(staticarithmetic SOURCES staticarithmetic.adb)
add_ada_error_test(staticarithmeticerrors SOURCES staticarithmeticerrors.adb)
set_tests_properties(error.integerliteralerrors ada.integerliteralbounds PROPERTIES TIMEOUT 10)
add_ada_error_test(subprogramstuberrors SOURCES subprogramstuberrors.adb)

add_ada_test(importconventions SOURCES importconventions.adb)
add_ada_error_test(importconventionerrors SOURCES importconventionerrors.adb)
add_ada_error_test(importconventionsyntaxerrors SOURCES importconventionsyntaxerrors.adb)

add_ada_test(sizeclauses SOURCES sizeclauses.adb)
add_ada_error_test(sizevalueerrors SOURCES sizevalueerrors.adb)
add_ada_error_test(sizerangeerrors SOURCES sizerangeerrors.adb)
add_ada_error_test(sizelayouterrors SOURCES sizelayouterrors.adb)
add_ada_error_test(representationerrors SOURCES representationerrors.adb)

add_ada_test(pragmas SOURCES pragmas.adb)
add_ada_error_test(pragmasyntaxerrors SOURCES pragmasyntaxerrors.adb)
add_ada_error_test(pragmarepresentationerrors SOURCES pragmarepresentationerrors.adb)
add_ada_error_test(pragmaelaborationerrors SOURCES pragmaelaborationerrors.adb)
add_ada_error_test(pragmataskingerrors SOURCES pragmataskingerrors.adb)
add_ada_error_test(pragmacheckerrors SOURCES pragmacheckerrors.adb)
add_ada_error_test(pragmaunknownerrors SOURCES pragmaunknownerrors.adb)

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

add_determinism_test(sizeclauses SOURCE sizeclauses.adb)
add_determinism_test(staticarithmetic SOURCE staticarithmetic.adb)
set_tests_properties(ada.staticarithmetic error.staticarithmeticerrors
    determinism.staticarithmetic PROPERTIES TIMEOUT 10)
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
add_determinism_test(genericelaborationorder SOURCE genericelaborationorder.adb)

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
add_test(NAME runtime.array_storage COMMAND array_storage_test)
find_package(Threads REQUIRED)
add_executable(task_context_test TaskContextTest.c)
target_link_libraries(task_context_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.task_context COMMAND task_context_test)
set_tests_properties(runtime.task_context PROPERTIES TIMEOUT 30)
add_executable(threading_test ThreadingTest.c)
target_link_libraries(threading_test PRIVATE adart)
add_test(NAME runtime.threading COMMAND threading_test)
set_tests_properties(runtime.threading PROPERTIES TIMEOUT 30)
add_executable(fixed_text_test FixedTextTest.c)
target_link_libraries(fixed_text_test PRIVATE adart)
add_test(NAME runtime.fixed_text COMMAND fixed_text_test)

add_executable(float_formatting_test FloatFormattingTest.c)
target_link_libraries(float_formatting_test PRIVATE adart)
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
add_ada_test(modularwide SOURCES modularwide.adb)
add_ada_test(modularwidechecks SOURCES modularwidechecks.adb)
add_ada_test(modulartasking SOURCES modulartasking.adb)
add_determinism_test(modulartasking SOURCE modulartasking.adb)
add_ada_error_test(modularwideerrors SOURCES modularwideerrors.adb)
add_ada_test(modulario SOURCES modulario.adb)
add_determinism_test(modulario SOURCE modulario.adb)
add_determinism_test(modularwide SOURCE modularwide.adb)
add_ada_test(modularchecks SOURCES modularchecks.adb)
add_ada_error_test(modularerrors SOURCES modularerrors.adb)

add_ada_test(generic_fixed SOURCES generic_fixed.adb)
add_ada_error_test(generic_fixed_errors SOURCES generic_fixed_errors.adb)
add_determinism_test(generic_fixed SOURCE generic_fixed.adb)
add_ada_error_test(generic_fixed_syntax SOURCES generic_fixed_syntax.adb)
add_ada_error_test(generic_fixed_decimal SOURCES generic_fixed_decimal.adb)
add_ada_error_test(generic_fixed_trailing SOURCES generic_fixed_trailing.adb)
add_ada_error_test(generic_fixed_instance_errors SOURCES generic_fixed_instance_errors.adb)
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

add_ada_test(recordrenames SOURCES renamed_fields.ads recordrenames.adb)
add_ada_error_test(recordrenameerrors SOURCES recordrenameerrors.adb)

add_ada_test(fixedtextchecks SOURCES fixedtextchecks.adb)
add_ada_error_test(fixedtexterrors SOURCES fixedtexterrors.adb)
add_ada_test(fixedtext SOURCES fixedtext.adb)
add_determinism_test(fixedtext SOURCE fixedtext.adb)
add_ada_test(fixedpoint SOURCES fixedpoint.adb)
add_determinism_test(fixedpoint SOURCE fixedpoint.adb)
add_ada_test(fixedchecks SOURCES fixedchecks.adb)
add_ada_test(fixedrounding SOURCES fixedrounding.adb)
add_ada_error_test(fixederrors SOURCES fixederrors.adb)
add_ada_error_test(fixeddecimalerrors SOURCES fixeddecimalerrors.adb)

add_ada_test(callbacks SOURCES callbacks.adb)
add_ada_test(callbacklibrary SOURCES callback_library.ads callback_library.adb callbacklibrary.adb)
add_ada_error_test(callbackerrors SOURCES callbackerrors.adb)
add_ada_error_test(callbackprofileerrors SOURCES callbackprofileerrors.adb)
add_ada_test(callbackchecks SOURCES callbackchecks.adb)

add_ada_test(subprogramrenames SOURCES subprogramrenames.adb)
add_ada_test(callbackrenames SOURCES callbackrenames.adb)
add_ada_test(renamelibrary SOURCES rename_library.ads rename_library.adb renamelibrary.adb)
add_ada_error_test(subprogramrenameerrors SOURCES subprogramrenameerrors.adb)
add_ada_test(renamemain SOURCES rename_entry.adb renamemain.adb)

add_ada_test(derivedcategories SOURCES derivedcategories.adb)
add_determinism_test(derivedcategories SOURCE derivedcategories.adb)
add_ada_test(derivedprofiles SOURCES derived_profile_model.ads derived_profile_model.adb derivedprofiles.adb)
add_determinism_test(derivedprofiles SOURCE derivedprofiles.adb)
add_ada_error_test(derivedcategoryerrors SOURCES derivedcategoryerrors.adb)
add_ada_test(attributeaudit SOURCES attributeaudit.adb)
add_determinism_test(attributeaudit SOURCE attributeaudit.adb)
add_ada_test(attributebounds SOURCES attributebounds.adb)
add_determinism_test(attributebounds SOURCE attributebounds.adb)
add_ada_error_test(attributeauditerrors SOURCES attributeauditerrors.adb)
add_ada_test(derivedtypes SOURCES derivedtypes.adb)
add_determinism_test(derivedtypes SOURCE derivedtypes.adb)
add_ada_test(derivedconstraints SOURCES derivedconstraints.adb)
add_determinism_test(derivedconstraints SOURCE derivedconstraints.adb)
add_ada_test(derivedaccess SOURCES derivedaccess.adb)
add_determinism_test(derivedaccess SOURCE derivedaccess.adb)
add_ada_test(derivedlibrary SOURCES derived_model.ads derived_model.adb derivedlibrary.adb)
add_determinism_test(derivedlibrary SOURCE derivedlibrary.adb)
add_ada_error_test(derivedtypeerrors SOURCES derivedtypeerrors.adb)
add_ada_test(primitives SOURCES primitives.adb)
add_determinism_test(primitives SOURCE primitives.adb)
add_ada_test(primitivevisibility SOURCES primitivevisibility.adb)
add_ada_error_test(primitiveerrors SOURCES primitiveerrors.adb)
add_ada_test(primitivechecks SOURCES primitivechecks.adb)
add_determinism_test(primitivechecks SOURCE primitivechecks.adb)
add_ada_test(primitiveseparate SOURCES primitive_base.ads primitive_base.adb
    primitive_child.ads primitive_child.adb primitiveseparate.adb)
add_determinism_test(primitiveseparate SOURCE primitiveseparate.adb)

add_ada_test(taggedrecords SOURCES taggedrecords.adb)
add_determinism_test(taggedrecords SOURCE taggedrecords.adb)
add_ada_test(taggedequality SOURCES taggedequality.adb)
add_ada_error_test(taggederrors SOURCES taggederrors.adb)
add_test(NAME ada.taggedlayout
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/taggedlayout.adb
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/TaggedLayout.c
        -DWORKDIR=${ADA_TEST_WORK_DIR}/taggedlayout
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTaggedLayout.cmake
)
add_determinism_test(taggedlayout SOURCE taggedlayout.adb)

add_ada_test(classwidedispatch SOURCES classwidedispatch.adb)
add_ada_error_test(classwideerrors SOURCES classwideerrors.adb)
add_ada_test(classwidelibrary SOURCES dispatch_model.ads dispatch_model.adb classwidelibrary.adb)
add_determinism_test(classwidedispatch SOURCE classwidedispatch.adb)
add_ada_test(classwideobjects SOURCES classwideobjects.adb)
add_ada_test(classwideframes SOURCES dispatch_model.ads dispatch_model.adb classwideframes.adb)
add_ada_test(classwidechecks SOURCES classwidechecks.adb)
add_determinism_test(classwideobjects SOURCE classwideobjects.adb)
add_determinism_test(classwideframes SOURCE classwideframes.adb)

add_ada_test(controlledlifetimes SOURCES controlledlifetimes.adb)
add_ada_test(controlledexceptions SOURCES controlledexceptions.adb)
add_determinism_test(controlledlifetimes SOURCE controlledlifetimes.adb)
add_ada_test(controlledlibrary SOURCES controlled_model.ads controlled_model.adb controlledlibrary.adb)
add_determinism_test(controlledlibrary SOURCE controlledlibrary.adb)
add_ada_error_test(controllederrors SOURCES controllederrors.adb)
add_ada_test(controlledglobals SOURCES controlled_shutdown_model.ads controlled_shutdown_model.adb
    controlled_globals.ads controlled_globals.adb controlledglobals.adb)
add_determinism_test(controlledglobals SOURCE controlledglobals.adb)
add_ada_test(controlledshutdownfailure SOURCES controlled_shutdown_model.ads controlled_shutdown_model.adb
    controlled_globals.ads controlled_globals.adb controlledshutdownfailure.adb
    EXIT_CODE 1 STDERR controlledshutdownfailure.stderr)
add_ada_test(controlledmainfailure SOURCES controlled_shutdown_model.ads controlled_shutdown_model.adb
    controlled_globals.ads controlled_globals.adb controlledmainfailure.adb
    EXIT_CODE 1 STDERR controlledmainfailure.stderr)
add_ada_test(controlledelaborationfailure SOURCES controlled_shutdown_model.ads controlled_shutdown_model.adb
    controlled_failed_globals.ads controlled_failed_globals.adb controlledelaborationfailure.adb
    EXIT_CODE 1 STDERR controlledelaborationfailure.stderr)
add_ada_test(controlledcopies SOURCES controlledcopies.adb)
add_ada_test(controlledparts SOURCES controlledparts.adb)
add_determinism_test(controlledcopies SOURCE controlledcopies.adb)
add_ada_test(controlledaggregates SOURCES controlledaggregates.adb)
add_ada_test(controlledarrays SOURCES controlledarrays.adb)
add_ada_test(controlledcopylibrary SOURCES controlled_copy_model.ads controlled_copy_model.adb controlledcopylibrary.adb)
add_determinism_test(controlledcopylibrary SOURCE controlledcopylibrary.adb)
add_ada_error_test(controlledcomponenterrors SOURCES controlledcomponenterrors.adb)
add_ada_test(controlledviews SOURCES controlledviews.adb)
add_ada_test(controlledresults SOURCES controlled_result_model.ads controlled_result_model.adb controlledresults.adb)
add_determinism_test(controlledresults SOURCE controlledresults.adb)
add_ada_test(controlledresultfailures SOURCES controlled_result_model.ads controlled_result_model.adb controlledresultfailures.adb)
add_ada_error_test(controlledresulterrors SOURCES controlledresulterrors.adb)
add_ada_test(controlledresultdispatch SOURCES controlled_result_model.ads controlled_result_model.adb controlledresultdispatch.adb)
add_ada_test(controlledresultglobals SOURCES controlled_result_model.ads controlled_result_model.adb
    controlled_result_globals.ads controlled_result_globals.adb controlledresultglobals.adb)
add_ada_test(controlledallocations SOURCES controlled_result_model.ads controlled_result_model.adb controlledallocations.adb)
add_determinism_test(controlledallocations SOURCE controlledallocations.adb)
add_ada_test(controlledallocationfailures SOURCES controlled_result_model.ads controlled_result_model.adb controlledallocationfailures.adb)
add_ada_test(controlledallocationglobals SOURCES controlled_result_model.ads controlled_result_model.adb
    controlled_allocation_globals.ads controlled_allocation_globals.adb controlledallocationglobals.adb)
add_ada_test(controlledallocationglobalfailure SOURCES controlled_result_model.ads controlled_result_model.adb
    controlled_allocation_globals.ads controlled_allocation_globals.adb controlledallocationglobalfailure.adb
    EXIT_CODE 1 STDERR controlledallocationglobalfailure.stderr)
add_ada_test(controlledconversions SOURCES controlled_result_model.ads controlled_result_model.adb controlledconversions.adb)
add_determinism_test(controlledconversions SOURCE controlledconversions.adb)
add_ada_test(privatecontrolled SOURCES controlled_result_model.ads controlled_result_model.adb
    private_controlled_model.ads private_controlled_model.adb privatecontrolled.adb)
add_determinism_test(privatecontrolled SOURCE privatecontrolled.adb)
add_ada_error_test(privatecontrollederrors SOURCES privatecontrollederrors.adb)

add_ada_test(opaquecontrolled SOURCES controlled_result_model.ads controlled_result_model.adb
    opaque_controlled_model.ads opaque_controlled_model.adb opaquecontrolled.adb)
add_determinism_test(opaquecontrolled SOURCE opaquecontrolled.adb)
add_ada_error_test(opaquecontrollederrors SOURCES opaquecontrollederrors.adb)

add_ada_test(controlledwideallocations SOURCES controlled_result_model.ads controlled_result_model.adb controlledwideallocations.adb)
add_determinism_test(controlledwideallocations SOURCE controlledwideallocations.adb)
add_ada_test(controlledwideallocationfailures SOURCES controlled_result_model.ads controlled_result_model.adb controlledwideallocationfailures.adb)
add_ada_test(controlledwideallocationglobals SOURCES controlled_result_model.ads controlled_result_model.adb
    controlled_wide_allocation_globals.ads controlled_wide_allocation_globals.adb controlledwideallocationglobals.adb)

add_ada_test(controlleddynamic SOURCES controlled_result_model.ads controlled_result_model.adb controlleddynamic.adb)
add_determinism_test(controlleddynamic SOURCE controlleddynamic.adb)
add_ada_test(controlledlimitedresults SOURCES controlledlimitedresults.adb)
add_determinism_test(controlledlimitedresults SOURCE controlledlimitedresults.adb)
add_ada_test(controlledancestor SOURCES controlled_result_model.ads controlled_result_model.adb controlledancestor.adb)
add_ada_test(controlledcompletion SOURCES controlled_result_model.ads controlled_result_model.adb
    controlled_completion_model.ads controlled_completion_model.adb controlledcompletion.adb)
add_determinism_test(controlledcompletion SOURCE controlledcompletion.adb)
add_ada_error_test(controlledconstructionerrors SOURCES controlledconstructionerrors.adb)

add_test(NAME runtime.generated_contexts
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/generatedcontexts.adb
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/GeneratedContexts.c
        -DWORKDIR=${ADA_TEST_WORK_DIR}/generatedcontexts
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunGeneratedContexts.cmake
)
set_tests_properties(runtime.generated_contexts PROPERTIES TIMEOUT 30)
add_determinism_test(generatedcontexts SOURCE generatedcontexts.adb)

# Timing is opt-in, never a noisy pass/fail threshold in the regression suite.
add_executable(context_access_benchmark EXCLUDE_FROM_ALL
    benchmarks/ContextAccess.c benchmarks/ContextProbe.c)
target_link_libraries(context_access_benchmark PRIVATE adart)

add_ada_test(imageresults SOURCES imageresults.adb)
add_determinism_test(imageresults SOURCE imageresults.adb)
add_ada_test(streamresults SOURCES streamresults.adb)
add_determinism_test(streamresults SOURCE streamresults.adb)

add_executable(result_buffers_test ResultBuffersTest.c)
target_link_libraries(result_buffers_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.result_buffers COMMAND result_buffers_test)
set_tests_properties(runtime.result_buffers PROPERTIES TIMEOUT 30)

add_executable(shared_tables_test SharedTablesTest.c)
target_link_libraries(shared_tables_test PRIVATE adart Threads::Threads)
file(MAKE_DIRECTORY ${ADA_TEST_WORK_DIR}/shared_tables)
add_test(NAME runtime.shared_tables COMMAND shared_tables_test)
set_tests_properties(runtime.shared_tables PROPERTIES
    TIMEOUT 30 WORKING_DIRECTORY ${ADA_TEST_WORK_DIR}/shared_tables)
add_ada_test(filesnapshots SOURCES filesnapshots.adb)
add_determinism_test(filesnapshots SOURCE filesnapshots.adb)

add_executable(shared_tables_benchmark EXCLUDE_FROM_ALL benchmarks/SharedTables.c)
target_link_libraries(shared_tables_benchmark PRIVATE adart)

if(UNIX OR MINGW)
    add_executable(blocking_io_test BlockingIoTest.c)
    target_link_libraries(blocking_io_test PRIVATE adart Threads::Threads)
    add_test(NAME runtime.blocking_io COMMAND blocking_io_test)
    set_tests_properties(runtime.blocking_io PROPERTIES TIMEOUT 15)
endif()

foreach(master_case masterhooks masterfailure masterlibraryfailure)
    set(master_status 0)
    set(master_failure OFF)
    if(NOT master_case STREQUAL "masterhooks")
        set(master_status 1)
        set(master_failure ON)
    endif()
    add_test(NAME runtime.${master_case}
        COMMAND ${CMAKE_COMMAND}
            -DNAME=${master_case}
            -DADA=$<TARGET_FILE:ada>
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${master_case}.adb
            -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/MasterHooks.c
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${master_case}
            -DEXPECT_STATUS=${master_status}
            -DEXPECT_FAILURE=${master_failure}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunMasterHooks.cmake)
    set_tests_properties(runtime.${master_case} PROPERTIES TIMEOUT 30)
endforeach()
add_determinism_test(masterhooks SOURCE masterhooks.adb)

foreach(loading_case cycles identity sources child bundles completions)
    add_test(NAME loading.${loading_case}
        COMMAND ${CMAKE_COMMAND}
            -DCASE=${loading_case}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/ada/unitloading
            -DWORKDIR=${ADA_TEST_WORK_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckUnitLoading.cmake)
    set_tests_properties(loading.${loading_case} PROPERTIES TIMEOUT 30)
endforeach()
add_ada_test(load_main SOURCES unitloading/load_main.adb)
add_ada_test(child_main SOURCES unitloading/load_parent-child.ads unitloading/child_main.adb)
add_determinism_test(load_main SOURCE unitloading/load_main.adb)
add_ada_test(bundle_main SOURCES unitloading/bundle_main.adb)
add_determinism_test(bundle_main SOURCE unitloading/bundle_main.adb)
add_ada_test(bundle_client SOURCES unitloading/bundle_parts.adb unitloading/bundle_client.adb)
add_ada_test(completion_main SOURCES unitloading/completion_main.adb)
add_determinism_test(completion_main SOURCE unitloading/completion_main.adb)
add_test(NAME binding.elaboration_order
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/unitloading/load_main.adb
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckElaborationOrder.cmake)
set_tests_properties(binding.elaboration_order PROPERTIES TIMEOUT 60)

add_ada_error_test(duplicatetypeerrors SOURCES duplicatetypeerrors.adb)
add_ada_test(typedeclarations SOURCES typedeclarations.adb)

add_ada_error_test(recordrecoveryaccess SOURCES recordrecoveryaccess.adb)
add_ada_error_test(recordrecoverymissingsemicolon SOURCES recordrecoverymissingsemicolon.adb)
add_ada_error_test(recordrecoveryerrors SOURCES recordrecoveryerrors.adb)
set_tests_properties(error.recordrecoveryaccess error.recordrecoverymissingsemicolon
    error.recordrecoveryerrors PROPERTIES TIMEOUT 10)

add_ada_error_test(generalaccesserrors SOURCES generalaccesserrors.adb)

add_ada_test(elaborationchecks SOURCES elaborationchecks.adb)
add_determinism_test(elaborationchecks SOURCE elaborationchecks.adb)
add_ada_test(elaborationlibrarycheck SOURCES elaborationlibrarycheck.adb
    EXIT_CODE 1 STDERR elaborationlibrarycheck.stderr)
add_determinism_test(elaborationlibrarycheck SOURCE elaborationlibrarycheck.adb)
add_ada_error_test(elaborationgenericerrors SOURCES elaborationgenericerrors.adb)

add_ada_test(forwardelaboration SOURCES forwardelaboration.adb)
add_determinism_test(forwardelaboration SOURCE forwardelaboration.adb)
add_ada_test(forwardcontracts SOURCES forwardcontracts.adb)
add_determinism_test(forwardcontracts SOURCE forwardcontracts.adb)
add_ada_test(forwardlibrary SOURCES forwardlibrary.adb)
add_determinism_test(forwardlibrary SOURCE forwardlibrary.adb)
add_test(NAME elaboration.forward_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/ada
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckForwardElaboration.cmake)
set_tests_properties(elaboration.forward_modes PROPERTIES TIMEOUT 30)

add_ada_test(ep_main SOURCES elaborationpragmas/ep_main.adb)
add_determinism_test(ep_main SOURCE elaborationpragmas/ep_main.adb)
add_test(NAME elaboration.pragmas
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/ada/elaborationpragmas
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckElaborationPragmas.cmake)
set_tests_properties(elaboration.pragmas PROPERTIES TIMEOUT 60)

add_ada_error_test(duplicatenumbererrors SOURCES duplicatenumbererrors.adb)
add_ada_error_test(duplicateexceptionerrors SOURCES duplicateexceptionerrors.adb)
add_ada_test(numberexceptiondeclarations SOURCES numberexceptiondeclarations.adb)
add_determinism_test(numberexceptiondeclarations SOURCE numberexceptiondeclarations.adb)

add_ada_error_test(profileconformanceerrors SOURCES profileconformanceerrors.adb)
add_ada_error_test(duplicateprofileerrors SOURCES duplicateprofileerrors.adb)
add_ada_error_test(visibilityerrors SOURCES visibilityerrors.adb)
add_ada_error_test(requiredcompletionerrors SOURCES requiredcompletionerrors.adb)
add_ada_test(visibilityconformance SOURCES visibilityconformance.adb)
add_determinism_test(visibilityconformance SOURCE visibilityconformance.adb)

add_test(NAME semantics.visibility_conformance
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckVisibilityConformance.cmake
)
set_tests_properties(semantics.visibility_conformance PROPERTIES TIMEOUT 90)

add_ada_test(profiledefaults SOURCES profiledefaults.adb)
add_determinism_test(profiledefaults SOURCE profiledefaults.adb)
add_ada_error_test(defaultbindingerrors SOURCES defaultbindingerrors.adb)

add_ada_test(elaborationblocks SOURCES elaborationblocks.adb)
add_determinism_test(elaborationblocks SOURCE elaborationblocks.adb)
add_ada_error_test(elaborationblockerrors SOURCES elaborationblockerrors.adb)
add_ada_test(elaborationblockfailure SOURCES elaborationblockfailure.adb
    EXIT_CODE 1 STDERR elaborationblockfailure.stderr)

add_ada_test(aggregateaudit SOURCES aggregateaudit.adb)
add_determinism_test(aggregateaudit SOURCE aggregateaudit.adb)
add_ada_test(aggregateauditchecks SOURCES aggregateauditchecks.adb)
add_ada_error_test(aggregateauditerrors SOURCES aggregateauditerrors.adb)
add_ada_error_test(aggregateboxerrors SOURCES aggregateboxerrors.adb)
add_ada_test(aggregatecleanup SOURCES aggregatecleanup.adb)
add_determinism_test(aggregatecleanup SOURCE aggregatecleanup.adb)

add_ada_test(arrayconversions SOURCES arrayconversions.adb)
add_determinism_test(arrayconversions SOURCE arrayconversions.adb)
add_ada_test(arrayconversionchecks SOURCES arrayconversionchecks.adb)
add_ada_error_test(arrayconversionerrors SOURCES arrayconversionerrors.adb)
add_determinism_test(arrayconversionchecks SOURCE arrayconversionchecks.adb)

add_ada_test(primitiveaudit SOURCES primitive_audit_model.ads primitive_audit_model.adb primitiveaudit.adb)
add_determinism_test(primitiveaudit SOURCE primitiveaudit.adb)
add_ada_error_test(primitiveauditerrors SOURCES primitiveauditerrors.adb)
add_ada_error_test(primitiveplacementerrors SOURCES primitiveplacementerrors.adb)

add_ada_test(nestedvariants SOURCES nestedvariants.adb)
add_determinism_test(nestedvariants SOURCE nestedvariants.adb)
add_ada_error_test(nestedvariantserrors SOURCES nestedvariantserrors.adb)

add_ada_test(interactioncalls SOURCES interactioncalls.adb)
add_determinism_test(interactioncalls SOURCE interactioncalls.adb)
add_ada_test(interactionownership SOURCES interactionownership.adb)
add_determinism_test(interactionownership SOURCE interactionownership.adb)
set_tests_properties(ada.interactioncalls ada.interactionownership
    determinism.interactioncalls determinism.interactionownership PROPERTIES TIMEOUT 60)
add_ada_error_test(interactionerrors SOURCES interactionerrors.adb)
set_tests_properties(error.interactionerrors PROPERTIES TIMEOUT 10)
add_test(NAME semantics.interaction_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/ada
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckInteractionModes.cmake)
set_tests_properties(semantics.interaction_modes PROPERTIES TIMEOUT 60)

foreach(task_case tasks taskallocations)
    add_ada_test(${task_case} SOURCES ${task_case}.adb)
    add_determinism_test(${task_case} SOURCE ${task_case}.adb)
    set_tests_properties(ada.${task_case} determinism.${task_case} PROPERTIES TIMEOUT 30)
endforeach()
add_ada_test(tasklibrarymain SOURCES tasklibrarymain.adb)
add_determinism_test(tasklibrarymain SOURCE tasklibrarymain.adb)
add_ada_error_test(taskerrors SOURCES taskerrors.adb)
set_tests_properties(ada.tasklibrarymain determinism.tasklibrarymain error.taskerrors PROPERTIES TIMEOUT 30)
add_determinism_test(taskconcurrency SOURCE taskconcurrency.adb)
add_test(NAME runtime.tasking
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/taskconcurrency.adb
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/TaskingHooks.c
        -DWORKDIR=${ADA_TEST_WORK_DIR}/tasking
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(runtime.tasking determinism.taskconcurrency PROPERTIES TIMEOUT 60)
add_executable(tasking_runtime_test TaskingRuntimeTest.c)
target_link_libraries(tasking_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.tasking_lifecycle COMMAND tasking_runtime_test)
set_tests_properties(runtime.tasking_lifecycle PROPERTIES TIMEOUT 30)
add_ada_test(taskactivationfailure SOURCES taskactivationfailure.adb
    EXIT_CODE 1 STDERR taskactivationfailure.stderr)
add_determinism_test(taskactivationfailure SOURCE taskactivationfailure.adb)
set_tests_properties(ada.taskactivationfailure determinism.taskactivationfailure PROPERTIES TIMEOUT 30)
add_ada_error_test(tasklegalityerrors SOURCES tasklegalityerrors.adb)
set_tests_properties(error.tasklegalityerrors PROPERTIES TIMEOUT 10)

add_ada_test(protectedobjects SOURCES protectedobjects.adb)
add_determinism_test(protectedobjects SOURCE protectedobjects.adb)
set_tests_properties(ada.protectedobjects determinism.protectedobjects PROPERTIES TIMEOUT 30)
add_ada_test(protectedconcurrency SOURCES protectedconcurrency.adb)
add_determinism_test(protectedconcurrency SOURCE protectedconcurrency.adb)
set_tests_properties(ada.protectedconcurrency determinism.protectedconcurrency PROPERTIES TIMEOUT 30)
add_ada_test(protectedlibrary SOURCES protectedlibrary.adb)
add_determinism_test(protectedlibrary SOURCE protectedlibrary.adb)
set_tests_properties(ada.protectedlibrary determinism.protectedlibrary PROPERTIES TIMEOUT 30)

add_executable(protected_runtime_test ProtectedRuntimeTest.c)
target_link_libraries(protected_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.protected_lifecycle COMMAND protected_runtime_test)
set_tests_properties(runtime.protected_lifecycle PROPERTIES TIMEOUT 30)
foreach(protected_case protectedobjects protectedconcurrency protectedlibrary protectedgenerics protectedcleanup)
    add_test(NAME runtime.${protected_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${protected_case}.adb
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${protected_case}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${protected_case}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(runtime.${protected_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_ada_error_test(protectederrors SOURCES protectederrors.adb)
add_ada_error_test(protectedsyntaxerrors SOURCES protectedsyntaxerrors.adb)
set_tests_properties(error.protectederrors error.protectedsyntaxerrors PROPERTIES TIMEOUT 10)

foreach(protected_case protectedgenerics protectedcleanup)
    add_ada_test(${protected_case} SOURCES ${protected_case}.adb)
    add_determinism_test(${protected_case} SOURCE ${protected_case}.adb)
    set_tests_properties(ada.${protected_case} determinism.${protected_case} PROPERTIES TIMEOUT 30)
endforeach()
add_ada_error_test(protectedlegalityerrors SOURCES protectedlegalityerrors.adb)
set_tests_properties(error.protectedlegalityerrors PROPERTIES TIMEOUT 10)
add_ada_error_test(protectedcompletionerrors SOURCES protectedcompletionerrors.adb)
set_tests_properties(error.protectedcompletionerrors PROPERTIES TIMEOUT 10)
add_ada_error_test(protectedgenericerrors SOURCES protectedgenericerrors.adb)
set_tests_properties(error.protectedgenericerrors PROPERTIES TIMEOUT 10)

foreach(entry_case protectedentries entryproducer entrylibrary entrygenerics)
    add_ada_test(${entry_case} SOURCES ${entry_case}.adb)
    add_determinism_test(${entry_case} SOURCE ${entry_case}.adb)
    add_test(NAME runtime.${entry_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${entry_case}.adb
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${entry_case}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${entry_case}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${entry_case} determinism.${entry_case}
        runtime.${entry_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_executable(entry_runtime_test EntryRuntimeTest.c)
target_link_libraries(entry_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.entry_lifecycle COMMAND entry_runtime_test)
set_tests_properties(runtime.entry_lifecycle PROPERTIES TIMEOUT 30)
add_ada_test(entryexceptions SOURCES entryexceptions.adb)
add_determinism_test(entryexceptions SOURCE entryexceptions.adb)
add_ada_error_test(entryerrors SOURCES entryerrors.adb)
add_test(NAME runtime.entryexceptions_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/entryexceptions.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/entryexceptions.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/entryexceptions-modes
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(ada.entryexceptions determinism.entryexceptions
    runtime.entryexceptions_modes error.entryerrors PROPERTIES TIMEOUT 30)

foreach(time_case timevalues timedelays timelibrary)
    add_ada_test(${time_case} SOURCES ${time_case}.adb)
    add_determinism_test(${time_case} SOURCE ${time_case}.adb)
    add_test(NAME runtime.${time_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${time_case}.adb
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${time_case}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${time_case}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${time_case} determinism.${time_case}
        runtime.${time_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_ada_error_test(timeerrors SOURCES timeerrors.adb)
set_tests_properties(error.timeerrors PROPERTIES TIMEOUT 10)
add_executable(time_runtime_test TimeRuntimeTest.c)
target_link_libraries(time_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.time_lifecycle COMMAND time_runtime_test)
set_tests_properties(runtime.time_lifecycle PROPERTIES TIMEOUT 30)

foreach(shared_case sharedscalars sharedconcurrency sharedlibrary)
    add_ada_test(${shared_case} SOURCES ${shared_case}.adb)
    add_determinism_test(${shared_case} SOURCE ${shared_case}.adb)
    add_test(NAME runtime.${shared_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${shared_case}.adb
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${shared_case}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${shared_case}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${shared_case} determinism.${shared_case}
        runtime.${shared_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_determinism_test(sharedimports SOURCE sharedimports.adb)
add_test(NAME runtime.sharedimports_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/sharedimports.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/sharedimports.expected
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/SharedHooks.c
        -DWORKDIR=${ADA_TEST_WORK_DIR}/sharedimports-modes
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(runtime.sharedimports_modes determinism.sharedimports PROPERTIES TIMEOUT 60)
foreach(shared_error sharederrors sharedcompletionerrors sharedsyntaxerrors)
    add_ada_error_test(${shared_error} SOURCES ${shared_error}.adb)
    set_tests_properties(error.${shared_error} PROPERTIES TIMEOUT 10)
endforeach()
add_test(NAME semantics.shared_ir
    COMMAND ${CMAKE_COMMAND}
        -DADAC=$<TARGET_FILE:adac>
        -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/ada
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckSharedIR.cmake)
set_tests_properties(semantics.shared_ir PROPERTIES TIMEOUT 30)

foreach(identity_case taskidentity taskidentitylifecycle taskidentityobjects)
    add_ada_test(${identity_case} SOURCES ${identity_case}.adb)
    add_determinism_test(${identity_case} SOURCE ${identity_case}.adb)
    add_test(NAME runtime.${identity_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${identity_case}.adb
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${identity_case}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${identity_case}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${identity_case} determinism.${identity_case}
        runtime.${identity_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_ada_error_test(taskidentityerrors SOURCES taskidentityerrors.adb)

# Numeric and array boundaries in whole/separate compilation, deterministic
# IR, and several independent Ada task contexts.
foreach(boundary_case realedges realtasking realvalue realvaluetasking arraydescriptors arraydescriptortasking concatenation concattasking)
    add_ada_test(${boundary_case} SOURCES ${boundary_case}.adb)
    add_determinism_test(${boundary_case} SOURCE ${boundary_case}.adb)
    add_test(NAME runtime.${boundary_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${boundary_case}.adb
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${boundary_case}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${boundary_case}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${boundary_case} determinism.${boundary_case}
        runtime.${boundary_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_ada_error_test(realedgeerrors SOURCES realedgeerrors.adb)
set_tests_properties(error.realedgeerrors PROPERTIES TIMEOUT 10)
add_executable(real_arithmetic_test RealArithmeticTest.c)
target_link_libraries(real_arithmetic_test PRIVATE adart)
add_test(NAME runtime.real_arithmetic COMMAND real_arithmetic_test)
set_tests_properties(runtime.real_arithmetic PROPERTIES TIMEOUT 20)

add_ada_error_test(realvalueerrors SOURCES realvalueerrors.adb)
add_executable(real_value_test RealValueTest.c)
target_link_libraries(real_value_test PRIVATE adart)
add_test(NAME runtime.real_value COMMAND real_value_test)
set_tests_properties(runtime.real_value PROPERTIES TIMEOUT 20)

add_executable(array_descriptor_test ArrayDescriptorTest.c)
target_link_libraries(array_descriptor_test PRIVATE adart)
add_test(NAME runtime.array_descriptors COMMAND array_descriptor_test)
set_tests_properties(runtime.array_descriptors PROPERTIES TIMEOUT 20)
add_ada_error_test(arraydescriptorerrors SOURCES arraydescriptorerrors.adb)
add_ada_error_test(arrayframeerrors SOURCES arrayframeerrors.adb)
add_ada_test(arraydescriptorstreams SOURCES arraydescriptorstreams.adb)
add_determinism_test(arraydescriptorstreams SOURCE arraydescriptorstreams.adb)
add_ada_error_test(concaterrors SOURCES concaterrors.adb)
add_executable(concat_runtime_test ConcatRuntimeTest.c)
target_link_libraries(concat_runtime_test PRIVATE adart)
add_test(NAME runtime.concatenation COMMAND concat_runtime_test)
set_tests_properties(runtime.concatenation PROPERTIES TIMEOUT 20)

add_executable(modular_runtime_test ModularRuntimeTest.c)
target_link_libraries(modular_runtime_test PRIVATE adart)
add_test(NAME runtime.modular COMMAND modular_runtime_test)
set_tests_properties(runtime.modular PROPERTIES TIMEOUT 20)

foreach(modular_case modularwide modulario modulartasking)
    add_test(NAME runtime.${modular_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${modular_case}.adb
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${modular_case}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${modular_case}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${modular_case} determinism.${modular_case}
        runtime.${modular_case}_modes PROPERTIES TIMEOUT 60)
endforeach()

add_ada_test(fixedwide SOURCES fixedwide.adb)
add_determinism_test(fixedwide SOURCE fixedwide.adb)

add_ada_test(fixedstreams SOURCES fixedstreams.adb)
add_determinism_test(fixedstreams SOURCE fixedstreams.adb)

add_ada_test(fixedwidechecks SOURCES fixedwidechecks.adb)
add_ada_error_test(fixedwideerrors SOURCES fixedwideerrors.adb)
add_ada_error_test(fixedoperatorerrors SOURCES fixedoperatorerrors.adb)

add_executable(fixed_arithmetic_test FixedArithmeticTest.c)
target_link_libraries(fixed_arithmetic_test PRIVATE adart)
add_test(NAME runtime.fixed_arithmetic COMMAND fixed_arithmetic_test)
set_tests_properties(runtime.fixed_arithmetic PROPERTIES TIMEOUT 20)

add_ada_test(fixedtasking SOURCES fixedtasking.adb)
add_determinism_test(fixedtasking SOURCE fixedtasking.adb)
foreach(fixed_case fixedwide fixedstreams fixedtasking)
    add_test(NAME runtime.${fixed_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${fixed_case}.adb
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${fixed_case}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${fixed_case}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${fixed_case} determinism.${fixed_case}
        runtime.${fixed_case}_modes PROPERTIES TIMEOUT 60)
endforeach()

add_ada_test(fixedoperators SOURCES fixedoperators.adb)
add_determinism_test(fixedoperators SOURCE fixedoperators.adb)

add_ada_test(decimalpoint SOURCES decimalpoint.adb)
add_determinism_test(decimalpoint SOURCE decimalpoint.adb)
add_ada_test(decimalchecks SOURCES decimalchecks.adb)
add_ada_error_test(decimalerrors SOURCES decimalerrors.adb)
add_ada_test(decimalstreams SOURCES decimalstreams.adb)
add_determinism_test(decimalstreams SOURCE decimalstreams.adb)
add_ada_test(decimaltasking SOURCES decimaltasking.adb)
add_determinism_test(decimaltasking SOURCE decimaltasking.adb)
foreach(decimal_case decimalpoint decimalchecks decimalstreams decimaltasking)
    add_test(NAME runtime.${decimal_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${decimal_case}.adb
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${decimal_case}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${decimal_case}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(runtime.${decimal_case}_modes PROPERTIES TIMEOUT 60)
endforeach()

add_executable(decimal_runtime_test DecimalRuntimeTest.c)
target_link_libraries(decimal_runtime_test PRIVATE adart)
add_test(NAME runtime.decimal COMMAND decimal_runtime_test)
set_tests_properties(runtime.decimal PROPERTIES TIMEOUT 20)

add_ada_error_test(alignmenterrors SOURCES alignmenterrors.adb)
add_determinism_test(alignmentclauses SOURCE alignmentclauses.adb)
add_test(NAME runtime.alignmentclauses_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/Alignment.c
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/alignmentclauses.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/alignmentclauses.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/alignmentclauses-modes
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(error.alignmenterrors determinism.alignmentclauses runtime.alignmentclauses_modes
    PROPERTIES TIMEOUT 60)

add_ada_error_test(enumrepresentationerrors SOURCES enumrepresentationerrors.adb)
add_ada_test(enumstreams SOURCES enumstreams.adb)
foreach(enum_case enumrepresentation enumstreams enumtasking)
    add_determinism_test(${enum_case} SOURCE ${enum_case}.adb)
    add_test(NAME runtime.${enum_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/EnumRepresentation.c
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${enum_case}.adb
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${enum_case}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${enum_case}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(runtime.${enum_case}_modes determinism.${enum_case} PROPERTIES TIMEOUT 60)
endforeach()

add_executable(enumeration_runtime_test EnumerationRuntimeTest.c)
target_link_libraries(enumeration_runtime_test PRIVATE adart)
add_test(NAME runtime.enumeration COMMAND enumeration_runtime_test)
set_tests_properties(runtime.enumeration PROPERTIES TIMEOUT 20)
set_tests_properties(error.enumrepresentationerrors ada.enumstreams PROPERTIES TIMEOUT 60)

add_ada_error_test(sizestorageerrors SOURCES sizestorageerrors.adb)
foreach(size_case sizestorage sizestreams sizetasking)
    add_determinism_test(${size_case} SOURCE ${size_case}.adb)
    add_test(NAME runtime.${size_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/SizeStorage.c
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/${size_case}.adb
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/${size_case}.expected
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${size_case}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(runtime.${size_case}_modes determinism.${size_case} PROPERTIES TIMEOUT 60)
endforeach()
set_tests_properties(error.sizestorageerrors PROPERTIES TIMEOUT 60)

add_executable(type_storage_test TypeStorageTest.cpp ${PROJECT_SOURCE_DIR}/adac/Type.cpp)
target_compile_features(type_storage_test PRIVATE cxx_std_17)
add_test(NAME semantics.type_storage COMMAND type_storage_test)
set_tests_properties(semantics.type_storage PROPERTIES TIMEOUT 20)

add_determinism_test(checkedarithmetic SOURCE checkedarithmetic.adb)
add_test(NAME runtime.checkedarithmetic_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/CheckedArithmetic.c
        -DFORBIDDEN_SYMBOL=__ada_integer_operation
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/checkedarithmetic.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/checkedarithmetic.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/checkedarithmetic-modes
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(runtime.checkedarithmetic_modes determinism.checkedarithmetic PROPERTIES TIMEOUT 60)

add_ada_test(checkedtasking SOURCES checkedtasking.adb)
add_determinism_test(checkedtasking SOURCE checkedtasking.adb)
add_test(NAME runtime.checkedtasking_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DFORBIDDEN_SYMBOL=__ada_integer_operation
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/checkedtasking.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/checkedtasking.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/checkedtasking-modes
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(ada.checkedtasking runtime.checkedtasking_modes
    determinism.checkedtasking PROPERTIES TIMEOUT 60)

add_ada_test(namedoperators SOURCES namedoperators.adb)
add_determinism_test(namedoperators SOURCE namedoperators.adb)
add_test(NAME runtime.namedoperators_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/namedoperators.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/namedoperators.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/namedoperators-modes
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(ada.namedoperators determinism.namedoperators
    runtime.namedoperators_modes PROPERTIES TIMEOUT 60)
add_ada_error_test(namedoperatorerrors SOURCES namedoperatorerrors.adb)
set_tests_properties(error.namedoperatorerrors PROPERTIES TIMEOUT 60)

add_executable(abort_deferral_test AbortDeferralTest.c)
target_link_libraries(abort_deferral_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.abort_deferral COMMAND abort_deferral_test)
set_tests_properties(runtime.abort_deferral PROPERTIES TIMEOUT 30)
add_determinism_test(abortdeferral SOURCE abortdeferral.adb)
add_test(NAME runtime.abortdeferral_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/abortdeferral.adb
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/AbortDeferralProbe.c
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/abortdeferral.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/abortdeferral-modes
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(determinism.abortdeferral runtime.abortdeferral_modes PROPERTIES TIMEOUT 60)

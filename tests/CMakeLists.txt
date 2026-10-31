include(${CMAKE_CURRENT_SOURCE_DIR}/TestFixtures.cmake)

set(ADA_TEST_WORK_DIR ${CMAKE_CURRENT_BINARY_DIR}/work)

add_subdirectory(acats)

add_test(NAME driver.external_linkage
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DAR=${CMAKE_AR}
        -DRANLIB=${CMAKE_RANLIB}
        -DEXE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}
        -DSOURCES=${CMAKE_CURRENT_SOURCE_DIR}
        "-DWORKDIR=${ADA_TEST_WORK_DIR}/external linkage"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckExternalLinkage.cmake)
set_tests_properties(driver.external_linkage PROPERTIES TIMEOUT 120)

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
        ada_test_fixture(stderr_expected "${ADA_TEST_STDERR}")
    endif()

    set(sources "")
    foreach(source ${ADA_TEST_SOURCES})
        ada_test_fixture(source_path "${source}")
        if(sources STREQUAL "")
            set(sources "${source_path}")
        else()
            set(sources "${sources}|${source_path}")
        endif()
    endforeach()

    ada_test_fixture(expected_path "${name}.expected")
    add_test(NAME ada.${name}
        COMMAND ${CMAKE_COMMAND}
            -DNAME=${name}
            -DEXIT_CODE=${ADA_TEST_EXIT_CODE}
            -DSTDERR_EXPECTED=${stderr_expected}
            -DADA=$<TARGET_FILE:ada>
            -DSOURCES=${sources}
            -DEXPECTED=${expected_path}
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
        ada_test_fixture(source_path "${source}")
        if(sources STREQUAL "")
            set(sources "${source_path}")
        else()
            set(sources "${sources}|${source_path}")
        endif()
    endforeach()

    ada_test_fixture(expected_path "${name}.expected")
    add_test(NAME error.${name}
        COMMAND ${CMAKE_COMMAND}
            -DNAME=${name}
            -DADAC=$<TARGET_FILE:adac>
            -DSOURCES=${sources}
            -DEXPECTED=${expected_path}
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
        ada_test_fixture(source_path "${source}")
        if(sources STREQUAL "")
            set(sources "${source_path}")
        else()
            set(sources "${sources}|${source_path}")
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

add_ada_test(reservedwordboundaries SOURCES syntax/reservedwordboundaries.adb)
add_ada_error_test(reservedworderrors SOURCES syntax/reservedworderrors.adb)
add_ada_error_test(reservedconstructerrors SOURCES syntax/reservedconstructerrors.adb)
add_ada_error_test(reservedgenericerrors SOURCES syntax/reservedgenericerrors.adb)
add_ada_error_test(genericformalerrors SOURCES generics/genericformalerrors.adb)
add_ada_error_test(genericformalsyntaxerrors SOURCES generics/genericformalsyntaxerrors.adb)
add_ada_error_test(deallocationformalerrors SOURCES generics/deallocationformalerrors.adb)
add_ada_error_test(unsupportedrecoveryerrors SOURCES syntax/unsupportedrecoveryerrors.adb)
add_ada_error_test(unsupportedconstructerrors SOURCES syntax/unsupportedconstructerrors.adb)
add_ada_test(unsupportedcontexterrors SOURCES syntax/unsupportedcontexterrors.adb)

add_ada_test(subprogramcompletions SOURCES subprograms/subprogramcompletions.adb)
add_ada_error_test(missingbodyerrors SOURCES subprograms/missingbodyerrors.adb)
add_ada_error_test(integerliteralerrors SOURCES numerics/integerliteralerrors.adb)
add_ada_error_test(numericseparatorerrors SOURCES library/numericseparatorerrors.adb)
add_ada_test(integerliteralbounds SOURCES numerics/integerliteralbounds.adb)
add_ada_test(staticarithmetic SOURCES numerics/staticarithmetic.adb)
add_ada_error_test(staticarithmeticerrors SOURCES numerics/staticarithmeticerrors.adb)
set_tests_properties(error.integerliteralerrors ada.integerliteralbounds PROPERTIES TIMEOUT 10)
add_ada_error_test(subprogramstuberrors SOURCES subprograms/subprogramstuberrors.adb)

add_ada_test(importconventions SOURCES subprograms/importconventions.adb)
add_ada_error_test(importconventionerrors SOURCES subprograms/importconventionerrors.adb)
add_ada_error_test(importconventionsyntaxerrors SOURCES subprograms/importconventionsyntaxerrors.adb)

add_ada_test(sizeclauses SOURCES representation/sizeclauses.adb)
add_ada_error_test(sizevalueerrors SOURCES representation/sizevalueerrors.adb)
add_ada_error_test(sizerangeerrors SOURCES representation/sizerangeerrors.adb)
add_ada_error_test(sizelayouterrors SOURCES representation/sizelayouterrors.adb)
add_ada_error_test(representationerrors SOURCES representation/representationerrors.adb)

add_ada_test(pragmas SOURCES elaboration/pragmas.adb)
add_ada_error_test(pragmasyntaxerrors SOURCES elaboration/pragmasyntaxerrors.adb)
add_ada_error_test(pragmarepresentationerrors SOURCES elaboration/pragmarepresentationerrors.adb)
add_ada_error_test(pragmaelaborationerrors SOURCES elaboration/pragmaelaborationerrors.adb)
add_ada_error_test(pragmataskingerrors SOURCES elaboration/pragmataskingerrors.adb)
add_ada_error_test(pragmacheckerrors SOURCES elaboration/pragmacheckerrors.adb)
add_ada_error_test(pragmaunknownerrors SOURCES elaboration/pragmaunknownerrors.adb)

add_ada_test(hello SOURCES basics/hello.adb)
add_ada_test(arithmetic SOURCES numerics/arithmetic.adb)
add_ada_test(control SOURCES control_flow/control.adb)
add_ada_test(subprograms SOURCES subprograms/subprograms.adb)
add_ada_test(scalarcopy SOURCES subprograms/scalarcopy.adb)
add_ada_test(scalarcopychecks SOURCES subprograms/scalarcopychecks.adb)
add_ada_test(occurrencebindings SOURCES exceptions/occurrencebindings.adb)
add_ada_test(occurrencenowith SOURCES exceptions/occurrencenowith.adb)
add_ada_error_test(occurrencebindingerrors SOURCES exceptions/occurrencebindingerrors.adb)
add_ada_test(exceptionmessages SOURCES exceptions/exceptionmessages.adb)
add_ada_test(exceptiondiagnostics SOURCES exceptions/exceptiondiagnostics.adb)
add_ada_error_test(handlerchoiceerrors SOURCES exceptions/handlerchoiceerrors.adb)
add_ada_error_test(handleremptyerrors SOURCES exceptions/handleremptyerrors.adb)
add_ada_error_test(handlerparterrors SOURCES exceptions/handlerparterrors.adb)
add_ada_error_test(handlerothererrors SOURCES exceptions/handlerothererrors.adb)
add_ada_test(savedoccurrences SOURCES exceptions/savedoccurrences.adb)
add_ada_error_test(savedoccurrenceerrors SOURCES exceptions/savedoccurrenceerrors.adb)
add_ada_error_test(exceptionmessageerrors SOURCES exceptions/exceptionmessageerrors.adb)
add_ada_test(unhandledmessage SOURCES exceptions/unhandledmessage.adb EXIT_CODE 1 STDERR exceptions/unhandledmessage.stderr)
add_ada_test(arrayidentity SOURCES arrays/arrayidentity.adb)
add_ada_test(groupedfields SOURCES types/groupedfields.adb)
add_ada_test(groupedfieldchecks SOURCES types/groupedfieldchecks.adb)
add_ada_error_test(groupedfielderrors SOURCES types/groupedfielderrors.adb)
add_ada_error_test(arrayidentityerrors SOURCES arrays/arrayidentityerrors.adb)
add_ada_error_test(arrayliteralambiguity SOURCES arrays/arrayliteralambiguity.adb)
add_ada_error_test(scalarcopyerrors SOURCES subprograms/scalarcopyerrors.adb)
add_ada_test(composite SOURCES types/composite.adb)
add_ada_test(packages SOURCES subprograms/packages.adb)
add_ada_test(checks SOURCES types/checks.adb)
add_ada_test(strings SOURCES strings/strings.adb)
add_ada_test(ranges SOURCES types/ranges.adb)
add_ada_test(floats SOURCES numerics/floats.adb)
add_ada_test(bases SOURCES types/bases.adb)
add_ada_test(textfiles SOURCES io/textfiles.adb)
add_ada_test(generics SOURCES generics/generics.adb)
add_ada_test(generic_subprograms SOURCES generics/generic_subprograms.adb)
add_ada_test(generic_defaults SOURCES generics/generic_defaults.adb)
add_ada_test(generic_library SOURCES generics/generic_library.adb)
add_ada_test(seqio SOURCES io/seqio.adb)
add_ada_test(directio SOURCES io/directio.adb)
add_ada_test(streamio SOURCES io/streamio.adb)
add_ada_test(typeio SOURCES io/typeio.adb)
add_ada_test(enumio SOURCES io/enumio.adb)
add_ada_test(storage SOURCES access/storage.adb)
add_ada_test(cases SOURCES control_flow/cases.adb)
add_ada_test(access SOURCES access/access.adb)
add_ada_test(private SOURCES types/private.adb)
add_ada_test(variants SOURCES types/variants.adb)

add_ada_error_test(floaterrors SOURCES numerics/floaterrors.adb)
add_ada_error_test(baseerrors SOURCES types/baseerrors.adb)
add_ada_error_test(genericerrors SOURCES generics/genericerrors.adb)
add_ada_error_test(generic_subprogram_errors SOURCES generics/generic_subprogram_errors.adb)
add_ada_error_test(ioerrors SOURCES io/ioerrors.adb)
add_ada_error_test(caseerrors SOURCES control_flow/caseerrors.adb)
add_ada_error_test(privateerrors SOURCES types/privateerrors.adb)
add_ada_error_test(varianterrors SOURCES types/varianterrors.adb)

add_ada_ir_test(hello SOURCES basics/hello.adb)

# add_determinism_test(<name> SOURCE <file>)
#
# Checks that a unit compiled on its own comes out the same as when the whole
# program was compiled at once.
function(add_determinism_test name)
    cmake_parse_arguments(DETERMINISM "" "SOURCE" "" ${ARGN})
    ada_test_fixture(source_path "${DETERMINISM_SOURCE}")
    add_test(NAME determinism.${name}
        COMMAND ${CMAKE_COMMAND}
            -DNAME=${name}
            -DADAC=$<TARGET_FILE:adac>
            -DSOURCE=${source_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckDeterminism.cmake
    )
endfunction()

add_determinism_test(sizeclauses SOURCE representation/sizeclauses.adb)
add_determinism_test(staticarithmetic SOURCE numerics/staticarithmetic.adb)
set_tests_properties(ada.staticarithmetic error.staticarithmeticerrors
    determinism.staticarithmetic PROPERTIES TIMEOUT 10)
add_determinism_test(private SOURCE types/private.adb)
add_determinism_test(generics SOURCE generics/generics.adb)
add_determinism_test(generic_subprograms SOURCE generics/generic_subprograms.adb)
add_determinism_test(generic_library SOURCE generics/generic_library.adb)
add_determinism_test(packages SOURCE subprograms/packages.adb)

# Builds a program twice over to check that a unit nobody touched keeps the
# object it already had, and that a changed body rebuilds that unit alone.
add_test(NAME ada.incremental
    COMMAND ${CMAKE_COMMAND}
        -DNAME=incremental
        -DADA=$<TARGET_FILE:ada>
        -DSOURCES=${CMAKE_CURRENT_SOURCE_DIR}/ada/driver/incremental.adb|${CMAKE_CURRENT_SOURCE_DIR}/ada/driver/incrhelper.ads|${CMAKE_CURRENT_SOURCE_DIR}/ada/driver/incrhelper.adb
        -DMAIN=incremental.adb
        -DMODIFY=incrhelper.adb
        -DMODIFY_UNIT=incrhelper
        "-DFROM=return 1"
        "-DTO=return 2"
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/driver/incremental.expected
        -DREBUILT=${CMAKE_CURRENT_SOURCE_DIR}/ada/driver/incremental.rebuilt.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckIncremental.cmake
)

add_ada_test(overflow32 SOURCES numerics/overflow32.adb)

add_ada_test(overflow64 SOURCES numerics/overflow64.adb)

add_ada_test(integerwidth SOURCES numerics/integerwidth.adb)

add_ada_test(integerdivision SOURCES numerics/integerdivision.adb)

add_ada_test(integerconversions SOURCES numerics/integerconversions.adb)

add_ada_test(integerloops SOURCES numerics/integerloops.adb)

add_ada_test(integerstatic SOURCES numerics/integerstatic.adb)

add_ada_test(longintegerio SOURCES io/longintegerio.adb)

add_ada_test(overloads SOURCES subprograms/overloads.adb)
add_ada_test(operators SOURCES subprograms/operators.adb)
add_ada_test(operatorprofiles SOURCES subprograms/operatorprofiles.adb)
add_ada_test(operatorunits SOURCES subprograms/operatorunits.adb)
add_ada_error_test(operatorerrors SOURCES subprograms/operatorerrors.adb)
add_ada_error_test(operatorambiguity SOURCES subprograms/operatorambiguity.adb)
add_ada_error_test(operatordesignatorerrors SOURCES subprograms/operatordesignatorerrors.adb)
add_ada_error_test(operatorclosingerrors SOURCES subprograms/operatorclosingerrors.adb)
add_ada_test(nestedoverloads SOURCES subprograms/nestedoverloads.adb)
add_ada_error_test(nestedoverloaderrors SOURCES subprograms/nestedoverloaderrors.adb)

add_ada_error_test(overloaderrors SOURCES subprograms/overloaderrors.adb)

add_test(NAME harness.exit_status
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckExitStatus.cmake
)

add_ada_test(arraycompare SOURCES arrays/arraycompare.adb)
add_ada_test(recordcompare SOURCES types/recordcompare.adb)
add_ada_error_test(comparisonerrors SOURCES syntax/comparisonerrors.adb)

add_ada_test(defaultcalls SOURCES subprograms/defaultcalls.adb)
add_ada_test(parameterless SOURCES subprograms/parameterless.adb)
add_ada_error_test(parameterlesserrors SOURCES subprograms/parameterlesserrors.adb)

add_ada_error_test(defaulterrors SOURCES subprograms/defaulterrors.adb)

add_ada_test(compositereturns SOURCES types/compositereturns.adb)
add_ada_error_test(returnerrors SOURCES subprograms/returnerrors.adb)

add_ada_test(functionfallthrough SOURCES subprograms/functionfallthrough.adb)
add_ada_test(reraise SOURCES exceptions/reraise.adb)
add_ada_error_test(exceptionusageerrors SOURCES exceptions/exceptionusageerrors.adb)
add_ada_test(elaborationfailure SOURCES elaboration/elaborationfailure.adb
    EXIT_CODE 1 STDERR elaboration/elaborationfailure.stderr)
add_ada_test(elaborationbodyfailure SOURCES elaboration/elaborationbodyfailure.adb
    EXIT_CODE 1 STDERR elaboration/elaborationbodyfailure.stderr)
add_ada_test(unhandledreraise SOURCES exceptions/unhandledreraise.adb
    EXIT_CODE 1 STDERR exceptions/unhandledreraise.stderr)

add_ada_test(elaborationorder SOURCES elaboration/elaborationorder.adb)
add_ada_test(packagehandlers SOURCES exceptions/packagehandlers.adb)
add_ada_test(packagedeclarationfailure SOURCES exceptions/packagedeclarationfailure.adb
    EXIT_CODE 1 STDERR exceptions/packagedeclarationfailure.stderr)
add_ada_test(packagehandlerfailure SOURCES exceptions/packagehandlerfailure.adb
    EXIT_CODE 1 STDERR exceptions/packagehandlerfailure.stderr)
add_ada_test(genericelaborationorder SOURCES generics/genericelaborationorder.adb)
add_determinism_test(genericelaborationorder SOURCE generics/genericelaborationorder.adb)

add_ada_test(localpackages SOURCES units/localpackages.adb)
add_ada_test(localgenericstate SOURCES generics/localgenericstate.adb)
add_ada_test(localpackagestartup SOURCES units/localpackagestartup.adb)
add_ada_test(localpackagefailure SOURCES units/localpackagefailure.adb)

add_ada_test(dynamicarrays SOURCES arrays/dynamicarrays.adb)
add_ada_test(dynamicarrayvalues SOURCES arrays/dynamicarrayvalues.adb)
add_ada_test(dynamicarrayfailures SOURCES arrays/dynamicarrayfailures.adb)
add_ada_error_test(dynamicarrayerrors SOURCES arrays/dynamicarrayerrors.adb)
add_ada_error_test(dynamicaggregateerrors SOURCES arrays/dynamicaggregateerrors.adb)

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
add_ada_test(arraylifetimes SOURCES arrays/arraylifetimes.adb)
add_ada_test(runtimeaggregates SOURCES arrays/runtimeaggregates.adb)
add_ada_test(runtimeaggregatechecks SOURCES arrays/runtimeaggregatechecks.adb)
add_ada_test(inferredaggregates SOURCES arrays/inferredaggregates.adb)
add_ada_test(inferredaggregatechecks SOURCES arrays/inferredaggregatechecks.adb)
add_ada_test(matrices SOURCES arrays/matrices.adb)
add_ada_test(matrixshapes SOURCES arrays/matrixshapes.adb)
add_ada_error_test(matrixerrors SOURCES arrays/matrixerrors.adb)
add_ada_test(runtimematrices SOURCES arrays/runtimematrices.adb)
add_ada_test(runtimematrixchecks SOURCES arrays/runtimematrixchecks.adb)
add_ada_error_test(runtimematrixerrors SOURCES arrays/runtimematrixerrors.adb)
add_ada_error_test(inferredmatrixerrors SOURCES arrays/inferredmatrixerrors.adb)
add_ada_test(inferredmatrices SOURCES arrays/inferredmatrices.adb)
add_ada_test(inferredmatrixchecks SOURCES arrays/inferredmatrixchecks.adb)
add_ada_error_test(matrixaggregateerrors SOURCES arrays/matrixaggregateerrors.adb)

# Semantic and boundary regression cases.
add_ada_test(shortcircuit SOURCES control_flow/shortcircuit.adb)
add_ada_test(booleanoperators SOURCES types/booleanoperators.adb)
add_ada_test(membership SOURCES types/membership.adb)
add_ada_test(loopedges SOURCES control_flow/loopedges.adb)
add_ada_test(sliceassignment SOURCES arrays/sliceassignment.adb)
add_ada_test(discreteattributes SOURCES types/discreteattributes.adb)
add_ada_test(signeddivision SOURCES numerics/signeddivision.adb)
add_ada_test(nestedcaptures SOURCES subprograms/nestedcaptures.adb)
add_ada_test(accesschecks SOURCES access/accesschecks.adb)
add_ada_test(valueparsing SOURCES numerics/valueparsing.adb)
add_ada_test(textfileedges SOURCES io/textfileedges.adb)
add_ada_test(streambounds SOURCES io/streambounds.adb)
add_ada_error_test(returnusageerrors SOURCES subprograms/returnusageerrors.adb)
add_ada_error_test(limitedprivateerrors SOURCES units/limitedprivateerrors.adb)
add_ada_error_test(declarationerrors SOURCES syntax/declarationerrors.adb)
add_ada_error_test(stringliteralerrors SOURCES strings/stringliteralerrors.adb)
add_ada_error_test(delimitererrors SOURCES syntax/delimitererrors.adb)
add_ada_test(valuebased SOURCES numerics/valuebased.adb)
add_ada_test(streamrawedges SOURCES io/streamrawedges.adb)
add_ada_test(streamcontainers SOURCES io/streamcontainers.adb)
add_ada_error_test(streamerrors SOURCES io/streamerrors.adb)

add_ada_error_test(identifiererrors SOURCES syntax/identifiererrors.adb)
add_ada_error_test(closingnameerrors SOURCES syntax/closingnameerrors.adb)
add_ada_error_test(duplicatedeclarationerrors SOURCES syntax/duplicatedeclarationerrors.adb)

include(${CMAKE_CURRENT_SOURCE_DIR}/BasePowerTests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/ImportedFloatAbiTests.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/NumericsPrecisionTests.cmake)

add_ada_test(runtimearraytypes SOURCES arrays/runtimearraytypes.adb)
add_ada_test(runtimearraytypechecks SOURCES arrays/runtimearraytypechecks.adb)
add_ada_error_test(runtimearraytypeerrors SOURCES arrays/runtimearraytypeerrors.adb)

add_ada_test(runtimescalars SOURCES types/runtimescalars.adb)
add_ada_test(runtimescalarchecks SOURCES types/runtimescalarchecks.adb)
add_ada_error_test(runtimescalarerrors SOURCES types/runtimescalarerrors.adb)
add_ada_error_test(runtimescalarlibraryerrors SOURCES types/runtimescalarlibraryerrors.ads)

add_ada_test(handlerchoices SOURCES exceptions/handlerchoices.adb)
add_ada_test(reverseloops SOURCES control_flow/reverseloops.adb)

foreach(case generic order compiler paths binding)
    add_test(NAME separate.${case}
        COMMAND ${CMAKE_COMMAND}
            -DCASE=${case}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/ada/units/separate
            -DWORKDIR=${ADA_TEST_WORK_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckSeparateCompilation.cmake
    )
endforeach()

add_ada_test(modular SOURCES numerics/modular.adb)
add_ada_test(modularwide SOURCES numerics/modularwide.adb)
add_ada_test(modularwidechecks SOURCES numerics/modularwidechecks.adb)
add_ada_test(modulartasking SOURCES numerics/modulartasking.adb)
add_determinism_test(modulartasking SOURCE numerics/modulartasking.adb)
add_ada_error_test(modularwideerrors SOURCES numerics/modularwideerrors.adb)
add_ada_test(modulario SOURCES io/modulario.adb)
add_determinism_test(modulario SOURCE io/modulario.adb)
add_determinism_test(modularwide SOURCE numerics/modularwide.adb)
add_ada_test(modularchecks SOURCES numerics/modularchecks.adb)
add_ada_error_test(modularerrors SOURCES numerics/modularerrors.adb)

add_ada_test(generic_fixed SOURCES generics/generic_fixed.adb)
add_ada_error_test(generic_fixed_errors SOURCES generics/generic_fixed_errors.adb)
add_determinism_test(generic_fixed SOURCE generics/generic_fixed.adb)
add_ada_error_test(generic_fixed_syntax SOURCES generics/generic_fixed_syntax.adb)
add_ada_error_test(generic_fixed_decimal SOURCES generics/generic_fixed_decimal.adb)
add_ada_error_test(generic_fixed_trailing SOURCES generics/generic_fixed_trailing.adb)
add_ada_error_test(generic_fixed_instance_errors SOURCES generics/generic_fixed_instance_errors.adb)
add_ada_test(generic_contracts SOURCES generics/generic_contracts.adb)
add_ada_error_test(generic_contract_errors SOURCES generics/generic_contract_errors.adb)
add_determinism_test(generic_contracts SOURCE generics/generic_contracts.adb)
add_ada_error_test(generic_separate_contract_errors
    SOURCES units/separate/contract_bad.ads units/separate/contract_bad.adb)

# Character and string standard-library packages.
add_ada_test(characterhandling SOURCES strings/characterhandling.adb)
add_ada_test(stringmaps SOURCES strings/stringmaps.adb)
add_ada_test(fixedstrings SOURCES strings/fixedstrings.adb)
add_ada_test(fixedstringchecks SOURCES strings/fixedstringchecks.adb)
add_determinism_test(fixedstrings SOURCE strings/fixedstrings.adb)

add_ada_test(commandline SOURCES driver/commandline.adb)
add_ada_test(commandlinefailure SOURCES driver/commandlinefailure.adb EXIT_CODE 1)
add_test(NAME runtime.commandline_arguments
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/driver/commandline.adb
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckCommandLine.cmake
)

add_ada_test(boundedstrings SOURCES strings/boundedstrings.adb)
add_determinism_test(boundedstrings SOURCE strings/boundedstrings.adb)
add_ada_test(genericcapacity SOURCES generics/genericcapacity.adb)
add_ada_error_test(genericcapacityerrors SOURCES generics/genericcapacityerrors.adb)

add_ada_test(generic_objects SOURCES generics/generic_objects.adb)
add_determinism_test(generic_objects SOURCE generics/generic_objects.adb)
add_ada_test(generic_object_checks SOURCES generics/generic_object_checks.adb)
add_ada_error_test(generic_object_errors SOURCES generics/generic_object_errors.adb)
add_ada_error_test(generic_object_modes SOURCES generics/generic_object_modes.adb)
add_ada_test(generic_object_library SOURCES generics/generic_object_library.adb)
add_ada_test(generic_object_nested SOURCES generics/generic_object_nested.adb)

add_ada_test(generic_composites SOURCES generics/generic_composites.adb)
add_determinism_test(generic_composites SOURCE generics/generic_composites.adb)
add_ada_test(generic_dynamic_objects SOURCES generics/generic_dynamic_objects.adb)
add_determinism_test(generic_dynamic_objects SOURCE generics/generic_dynamic_objects.adb)
add_ada_test(generic_composite_checks SOURCES generics/generic_composite_checks.adb)
add_ada_error_test(generic_composite_errors SOURCES generics/generic_composite_errors.adb)
add_ada_test(generic_composite_library SOURCES generics/generic_composite_library.adb)

add_ada_test(recordrenames SOURCES types/renamed_fields.ads types/recordrenames.adb)
add_ada_error_test(recordrenameerrors SOURCES types/recordrenameerrors.adb)

add_ada_test(fixedtextchecks SOURCES numerics/fixedtextchecks.adb)
add_ada_error_test(fixedtexterrors SOURCES numerics/fixedtexterrors.adb)
add_ada_test(fixedtext SOURCES numerics/fixedtext.adb)
add_determinism_test(fixedtext SOURCE numerics/fixedtext.adb)
add_ada_test(fixedpoint SOURCES numerics/fixedpoint.adb)
add_determinism_test(fixedpoint SOURCE numerics/fixedpoint.adb)
add_ada_test(fixedchecks SOURCES numerics/fixedchecks.adb)
add_ada_test(fixedrounding SOURCES numerics/fixedrounding.adb)
add_ada_error_test(fixederrors SOURCES numerics/fixederrors.adb)
add_ada_error_test(fixeddecimalerrors SOURCES numerics/fixeddecimalerrors.adb)

add_ada_test(callbacks SOURCES access/callbacks.adb)
add_ada_test(callbacklibrary SOURCES access/callback_library.ads access/callback_library.adb access/callbacklibrary.adb)
add_ada_error_test(callbackerrors SOURCES access/callbackerrors.adb)
add_ada_error_test(callbackprofileerrors SOURCES access/callbackprofileerrors.adb)
add_ada_test(callbackchecks SOURCES access/callbackchecks.adb)

add_ada_test(subprogramrenames SOURCES subprograms/subprogramrenames.adb)
add_ada_test(callbackrenames SOURCES access/callbackrenames.adb)
add_ada_test(renamelibrary SOURCES units/rename_library.ads units/rename_library.adb units/renamelibrary.adb)
add_ada_error_test(subprogramrenameerrors SOURCES subprograms/subprogramrenameerrors.adb)
add_ada_test(renamemain SOURCES units/rename_entry.adb units/renamemain.adb)

add_ada_test(derivedcategories SOURCES types/derivedcategories.adb)
add_determinism_test(derivedcategories SOURCE types/derivedcategories.adb)
add_ada_test(derivedprofiles SOURCES types/derived_profile_model.ads types/derived_profile_model.adb types/derivedprofiles.adb)
add_determinism_test(derivedprofiles SOURCE types/derivedprofiles.adb)
add_ada_error_test(derivedcategoryerrors SOURCES types/derivedcategoryerrors.adb)
add_ada_test(attributeaudit SOURCES types/attributeaudit.adb)
add_determinism_test(attributeaudit SOURCE types/attributeaudit.adb)
add_ada_test(attributebounds SOURCES types/attributebounds.adb)
add_determinism_test(attributebounds SOURCE types/attributebounds.adb)
add_ada_error_test(attributeauditerrors SOURCES types/attributeauditerrors.adb)
add_ada_test(derivedtypes SOURCES types/derivedtypes.adb)
add_determinism_test(derivedtypes SOURCE types/derivedtypes.adb)
add_ada_test(derivedconstraints SOURCES types/derivedconstraints.adb)
add_determinism_test(derivedconstraints SOURCE types/derivedconstraints.adb)
add_ada_test(derivedaccess SOURCES types/derivedaccess.adb)
add_determinism_test(derivedaccess SOURCE types/derivedaccess.adb)
add_ada_test(derivedlibrary SOURCES types/derived_model.ads types/derived_model.adb types/derivedlibrary.adb)
add_determinism_test(derivedlibrary SOURCE types/derivedlibrary.adb)
add_ada_error_test(derivedtypeerrors SOURCES types/derivedtypeerrors.adb)
add_ada_test(primitives SOURCES tagged/primitives.adb)
add_determinism_test(primitives SOURCE tagged/primitives.adb)
add_ada_test(primitivevisibility SOURCES tagged/primitivevisibility.adb)
add_ada_error_test(primitiveerrors SOURCES tagged/primitiveerrors.adb)
add_ada_test(primitivechecks SOURCES tagged/primitivechecks.adb)
add_determinism_test(primitivechecks SOURCE tagged/primitivechecks.adb)
add_ada_test(primitiveseparate SOURCES tagged/primitive_base.ads tagged/primitive_base.adb
    tagged/primitive_child.ads tagged/primitive_child.adb tagged/primitiveseparate.adb)
add_determinism_test(primitiveseparate SOURCE tagged/primitiveseparate.adb)

add_ada_test(taggedrecords SOURCES tagged/taggedrecords.adb)
add_determinism_test(taggedrecords SOURCE tagged/taggedrecords.adb)
add_ada_test(taggedequality SOURCES tagged/taggedequality.adb)
add_ada_error_test(taggederrors SOURCES tagged/taggederrors.adb)
add_ada_error_test(taggedviewerrors SOURCES tagged/taggedviewerrors.adb)
add_ada_error_test(privatecompletionerrors SOURCES types/privatecompletionerrors.adb)
add_ada_error_test(interfaceerrors SOURCES tagged/interfaceerrors.adb)
add_ada_error_test(interfacecategoryerrors SOURCES tagged/interfacecategoryerrors.adb)
add_ada_error_test(interfaceprivacyerrors SOURCES tagged/interfaceprivacyerrors.adb)
add_ada_error_test(interfaceconversionerrors SOURCES tagged/interfaceconversionerrors.adb)
add_test(NAME ada.taggedlayout
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/tagged/taggedlayout.adb
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/TaggedLayout.c
        -DWORKDIR=${ADA_TEST_WORK_DIR}/taggedlayout
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTaggedLayout.cmake
)
add_determinism_test(taggedlayout SOURCE tagged/taggedlayout.adb)

add_ada_test(classwidedispatch SOURCES tagged/classwidedispatch.adb)
add_ada_error_test(classwideerrors SOURCES tagged/classwideerrors.adb)
add_ada_test(classwidelibrary SOURCES tagged/dispatch_model.ads tagged/dispatch_model.adb tagged/classwidelibrary.adb)
add_determinism_test(classwidedispatch SOURCE tagged/classwidedispatch.adb)
add_ada_test(classwideobjects SOURCES tagged/classwideobjects.adb)
add_ada_test(classwideframes SOURCES tagged/dispatch_model.ads tagged/dispatch_model.adb tagged/classwideframes.adb)
add_ada_test(classwidechecks SOURCES tagged/classwidechecks.adb)
add_determinism_test(classwideobjects SOURCE tagged/classwideobjects.adb)
add_determinism_test(classwideframes SOURCE tagged/classwideframes.adb)

add_ada_test(controlledlifetimes SOURCES controlled/controlledlifetimes.adb)
add_ada_test(controlledexceptions SOURCES controlled/controlledexceptions.adb)
add_determinism_test(controlledlifetimes SOURCE controlled/controlledlifetimes.adb)
add_ada_test(controlledlibrary SOURCES controlled/controlled_model.ads controlled/controlled_model.adb controlled/controlledlibrary.adb)
add_determinism_test(controlledlibrary SOURCE controlled/controlledlibrary.adb)
add_ada_error_test(controllederrors SOURCES controlled/controllederrors.adb)
add_ada_test(controlledglobals SOURCES controlled/controlled_shutdown_model.ads controlled/controlled_shutdown_model.adb
    controlled/controlled_globals.ads controlled/controlled_globals.adb controlled/controlledglobals.adb)
add_determinism_test(controlledglobals SOURCE controlled/controlledglobals.adb)
add_ada_test(controlledshutdownfailure SOURCES controlled/controlled_shutdown_model.ads controlled/controlled_shutdown_model.adb
    controlled/controlled_globals.ads controlled/controlled_globals.adb controlled/controlledshutdownfailure.adb
    EXIT_CODE 1 STDERR controlled/controlledshutdownfailure.stderr)
add_ada_test(controlledmainfailure SOURCES controlled/controlled_shutdown_model.ads controlled/controlled_shutdown_model.adb
    controlled/controlled_globals.ads controlled/controlled_globals.adb controlled/controlledmainfailure.adb
    EXIT_CODE 1 STDERR controlled/controlledmainfailure.stderr)
add_ada_test(controlledelaborationfailure SOURCES controlled/controlled_shutdown_model.ads controlled/controlled_shutdown_model.adb
    controlled/controlled_failed_globals.ads controlled/controlled_failed_globals.adb controlled/controlledelaborationfailure.adb
    EXIT_CODE 1 STDERR controlled/controlledelaborationfailure.stderr)
add_ada_test(controlledcopies SOURCES controlled/controlledcopies.adb)
add_ada_test(controlledparts SOURCES controlled/controlledparts.adb)
add_determinism_test(controlledcopies SOURCE controlled/controlledcopies.adb)
add_ada_test(controlledaggregates SOURCES controlled/controlledaggregates.adb)
add_ada_test(controlledarrays SOURCES controlled/controlledarrays.adb)
add_ada_test(controlledcopylibrary SOURCES controlled/controlled_copy_model.ads controlled/controlled_copy_model.adb controlled/controlledcopylibrary.adb)
add_determinism_test(controlledcopylibrary SOURCE controlled/controlledcopylibrary.adb)
add_ada_error_test(controlledcomponenterrors SOURCES controlled/controlledcomponenterrors.adb)
add_ada_test(controlledviews SOURCES controlled/controlledviews.adb)
add_ada_test(controlledresults SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb controlled/controlledresults.adb)
add_determinism_test(controlledresults SOURCE controlled/controlledresults.adb)
add_ada_test(controlledresultfailures SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb controlled/controlledresultfailures.adb)
add_ada_error_test(controlledresulterrors SOURCES controlled/controlledresulterrors.adb)
add_ada_test(controlledresultdispatch SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb controlled/controlledresultdispatch.adb)
add_ada_test(controlledresultglobals SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb
    controlled/controlled_result_globals.ads controlled/controlled_result_globals.adb controlled/controlledresultglobals.adb)
add_ada_test(controlledallocations SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb controlled/controlledallocations.adb)
add_determinism_test(controlledallocations SOURCE controlled/controlledallocations.adb)
add_ada_test(controlledallocationfailures SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb controlled/controlledallocationfailures.adb)
add_ada_test(controlledallocationglobals SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb
    controlled/controlled_allocation_globals.ads controlled/controlled_allocation_globals.adb controlled/controlledallocationglobals.adb)
add_ada_test(controlledallocationglobalfailure SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb
    controlled/controlled_allocation_globals.ads controlled/controlled_allocation_globals.adb controlled/controlledallocationglobalfailure.adb
    EXIT_CODE 1 STDERR controlled/controlledallocationglobalfailure.stderr)
add_ada_test(controlledconversions SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb controlled/controlledconversions.adb)
add_determinism_test(controlledconversions SOURCE controlled/controlledconversions.adb)
add_ada_test(privatecontrolled SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb
    controlled/private_controlled_model.ads controlled/private_controlled_model.adb controlled/privatecontrolled.adb)
add_determinism_test(privatecontrolled SOURCE controlled/privatecontrolled.adb)
add_ada_error_test(privatecontrollederrors SOURCES controlled/privatecontrollederrors.adb)

add_ada_test(opaquecontrolled SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb
    controlled/opaque_controlled_model.ads controlled/opaque_controlled_model.adb controlled/opaquecontrolled.adb)
add_determinism_test(opaquecontrolled SOURCE controlled/opaquecontrolled.adb)
add_ada_error_test(opaquecontrollederrors SOURCES controlled/opaquecontrollederrors.adb)

add_ada_test(controlledwideallocations SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb controlled/controlledwideallocations.adb)
add_determinism_test(controlledwideallocations SOURCE controlled/controlledwideallocations.adb)
add_ada_test(controlledwideallocationfailures SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb controlled/controlledwideallocationfailures.adb)
add_ada_test(controlledwideallocationglobals SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb
    controlled/controlled_wide_allocation_globals.ads controlled/controlled_wide_allocation_globals.adb controlled/controlledwideallocationglobals.adb)

add_ada_test(controlleddynamic SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb controlled/controlleddynamic.adb)
add_determinism_test(controlleddynamic SOURCE controlled/controlleddynamic.adb)
add_ada_test(controlledlimitedresults SOURCES controlled/controlledlimitedresults.adb)
add_determinism_test(controlledlimitedresults SOURCE controlled/controlledlimitedresults.adb)
add_ada_test(controlledancestor SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb controlled/controlledancestor.adb)
add_ada_test(controlledcompletion SOURCES controlled/controlled_result_model.ads controlled/controlled_result_model.adb
    controlled/controlled_completion_model.ads controlled/controlled_completion_model.adb controlled/controlledcompletion.adb)
add_determinism_test(controlledcompletion SOURCE controlled/controlledcompletion.adb)
add_ada_error_test(controlledconstructionerrors SOURCES controlled/controlledconstructionerrors.adb)

add_test(NAME runtime.generated_contexts
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/units/generatedcontexts.adb
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/GeneratedContexts.c
        -DWORKDIR=${ADA_TEST_WORK_DIR}/generatedcontexts
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunGeneratedContexts.cmake
)
set_tests_properties(runtime.generated_contexts PROPERTIES TIMEOUT 30)
add_determinism_test(generatedcontexts SOURCE units/generatedcontexts.adb)

# Timing is opt-in, never a noisy pass/fail threshold in the regression suite.
add_executable(context_access_benchmark EXCLUDE_FROM_ALL
    benchmarks/ContextAccess.c benchmarks/ContextProbe.c)
target_link_libraries(context_access_benchmark PRIVATE adart)

add_ada_test(imageresults SOURCES types/imageresults.adb)
add_determinism_test(imageresults SOURCE types/imageresults.adb)
add_ada_test(streamresults SOURCES io/streamresults.adb)
add_determinism_test(streamresults SOURCE io/streamresults.adb)

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
add_ada_test(filesnapshots SOURCES io/filesnapshots.adb)
add_determinism_test(filesnapshots SOURCE io/filesnapshots.adb)

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
    ada_test_fixture(source_path "${master_case}.adb")
    add_test(NAME runtime.${master_case}
        COMMAND ${CMAKE_COMMAND}
            -DNAME=${master_case}
            -DADA=$<TARGET_FILE:ada>
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/MasterHooks.c
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${master_case}
            -DEXPECT_STATUS=${master_status}
            -DEXPECT_FAILURE=${master_failure}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunMasterHooks.cmake)
    set_tests_properties(runtime.${master_case} PROPERTIES TIMEOUT 30)
endforeach()
add_determinism_test(masterhooks SOURCE tasks/masterhooks.adb)

foreach(loading_case cycles identity sources child bundles completions)
    add_test(NAME loading.${loading_case}
        COMMAND ${CMAKE_COMMAND}
            -DCASE=${loading_case}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/ada/units/loading
            -DWORKDIR=${ADA_TEST_WORK_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckUnitLoading.cmake)
    set_tests_properties(loading.${loading_case} PROPERTIES TIMEOUT 30)
endforeach()
add_ada_test(load_main SOURCES units/loading/load_main.adb)
add_ada_test(child_main SOURCES units/loading/load_parent-child.ads units/loading/child_main.adb)
add_determinism_test(load_main SOURCE units/loading/load_main.adb)
add_ada_test(bundle_main SOURCES units/loading/bundle_main.adb)
add_determinism_test(bundle_main SOURCE units/loading/bundle_main.adb)
add_ada_test(bundle_client SOURCES units/loading/bundle_parts.adb units/loading/bundle_client.adb)
add_ada_test(completion_main SOURCES units/loading/completion_main.adb)
add_determinism_test(completion_main SOURCE units/loading/completion_main.adb)
add_test(NAME binding.elaboration_order
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/units/loading/load_main.adb
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckElaborationOrder.cmake)
set_tests_properties(binding.elaboration_order PROPERTIES TIMEOUT 60)

add_ada_error_test(duplicatetypeerrors SOURCES syntax/duplicatetypeerrors.adb)
add_ada_test(typedeclarations SOURCES types/typedeclarations.adb)

add_ada_error_test(recordrecoveryaccess SOURCES types/recordrecoveryaccess.adb)
add_ada_error_test(recordrecoverymissingsemicolon SOURCES types/recordrecoverymissingsemicolon.adb)
add_ada_error_test(recordrecoveryerrors SOURCES types/recordrecoveryerrors.adb)
set_tests_properties(error.recordrecoveryaccess error.recordrecoverymissingsemicolon
    error.recordrecoveryerrors PROPERTIES TIMEOUT 10)

add_ada_error_test(generalaccesserrors SOURCES access/generalaccesserrors.adb)

add_ada_test(elaborationchecks SOURCES elaboration/elaborationchecks.adb)
add_determinism_test(elaborationchecks SOURCE elaboration/elaborationchecks.adb)
add_ada_test(elaborationlibrarycheck SOURCES elaboration/elaborationlibrarycheck.adb
    EXIT_CODE 1 STDERR elaboration/elaborationlibrarycheck.stderr)
add_determinism_test(elaborationlibrarycheck SOURCE elaboration/elaborationlibrarycheck.adb)
add_ada_error_test(elaborationgenericerrors SOURCES elaboration/elaborationgenericerrors.adb)

add_ada_test(forwardelaboration SOURCES elaboration/forwardelaboration.adb)
add_determinism_test(forwardelaboration SOURCE elaboration/forwardelaboration.adb)
add_ada_test(forwardcontracts SOURCES elaboration/forwardcontracts.adb)
add_determinism_test(forwardcontracts SOURCE elaboration/forwardcontracts.adb)
add_ada_test(forwardlibrary SOURCES elaboration/forwardlibrary.adb)
add_determinism_test(forwardlibrary SOURCE elaboration/forwardlibrary.adb)
add_test(NAME elaboration.forward_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/ada/elaboration
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckForwardElaboration.cmake)
set_tests_properties(elaboration.forward_modes PROPERTIES TIMEOUT 30)

add_ada_test(ep_main SOURCES elaboration/pragmas/ep_main.adb)
add_determinism_test(ep_main SOURCE elaboration/pragmas/ep_main.adb)
add_test(NAME elaboration.pragmas
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/ada/elaboration/pragmas
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckElaborationPragmas.cmake)
set_tests_properties(elaboration.pragmas PROPERTIES TIMEOUT 60)

add_ada_error_test(duplicatenumbererrors SOURCES syntax/duplicatenumbererrors.adb)
add_ada_error_test(duplicateexceptionerrors SOURCES exceptions/duplicateexceptionerrors.adb)
add_ada_test(numberexceptiondeclarations SOURCES exceptions/numberexceptiondeclarations.adb)
add_determinism_test(numberexceptiondeclarations SOURCE exceptions/numberexceptiondeclarations.adb)

add_ada_error_test(profileconformanceerrors SOURCES subprograms/profileconformanceerrors.adb)
add_ada_error_test(duplicateprofileerrors SOURCES syntax/duplicateprofileerrors.adb)
add_ada_error_test(visibilityerrors SOURCES units/visibilityerrors.adb)
add_ada_error_test(requiredcompletionerrors SOURCES subprograms/requiredcompletionerrors.adb)
add_ada_test(visibilityconformance SOURCES units/visibilityconformance.adb)
add_determinism_test(visibilityconformance SOURCE units/visibilityconformance.adb)

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

add_ada_test(profiledefaults SOURCES subprograms/profiledefaults.adb)
add_determinism_test(profiledefaults SOURCE subprograms/profiledefaults.adb)
add_ada_error_test(defaultbindingerrors SOURCES subprograms/defaultbindingerrors.adb)

add_ada_test(elaborationblocks SOURCES elaboration/elaborationblocks.adb)
add_determinism_test(elaborationblocks SOURCE elaboration/elaborationblocks.adb)
add_ada_error_test(elaborationblockerrors SOURCES elaboration/elaborationblockerrors.adb)
add_ada_test(elaborationblockfailure SOURCES elaboration/elaborationblockfailure.adb
    EXIT_CODE 1 STDERR elaboration/elaborationblockfailure.stderr)

add_ada_test(privateaggregateresolution SOURCES types/privateaggregateresolution.adb)
add_determinism_test(privateaggregateresolution SOURCE types/privateaggregateresolution.adb)
add_ada_error_test(aggregateoverloaderrors SOURCES types/aggregateoverloaderrors.adb)
add_ada_test(aggregateaudit SOURCES arrays/aggregateaudit.adb)
add_determinism_test(aggregateaudit SOURCE arrays/aggregateaudit.adb)
add_ada_test(aggregateauditchecks SOURCES arrays/aggregateauditchecks.adb)
add_ada_error_test(aggregateauditerrors SOURCES arrays/aggregateauditerrors.adb)
add_ada_error_test(aggregateboxerrors SOURCES arrays/aggregateboxerrors.adb)
add_ada_test(aggregatecleanup SOURCES arrays/aggregatecleanup.adb)
add_determinism_test(aggregatecleanup SOURCE arrays/aggregatecleanup.adb)

add_ada_test(arrayconversions SOURCES arrays/arrayconversions.adb)
add_determinism_test(arrayconversions SOURCE arrays/arrayconversions.adb)
add_ada_test(arrayconversionchecks SOURCES arrays/arrayconversionchecks.adb)
add_ada_error_test(arrayconversionerrors SOURCES arrays/arrayconversionerrors.adb)
add_determinism_test(arrayconversionchecks SOURCE arrays/arrayconversionchecks.adb)

add_ada_test(primitiveaudit SOURCES tagged/primitive_audit_model.ads tagged/primitive_audit_model.adb tagged/primitiveaudit.adb)
add_determinism_test(primitiveaudit SOURCE tagged/primitiveaudit.adb)
add_ada_error_test(primitiveauditerrors SOURCES tagged/primitiveauditerrors.adb)
add_ada_error_test(primitiveplacementerrors SOURCES tagged/primitiveplacementerrors.adb)

add_ada_test(nestedvariants SOURCES types/nestedvariants.adb)
add_determinism_test(nestedvariants SOURCE types/nestedvariants.adb)
add_ada_error_test(nestedvariantserrors SOURCES types/nestedvariantserrors.adb)

add_ada_test(interactioncalls SOURCES interactions/interactioncalls.adb)
add_determinism_test(interactioncalls SOURCE interactions/interactioncalls.adb)
add_ada_test(interactionownership SOURCES controlled/interactionownership.adb)
add_determinism_test(interactionownership SOURCE controlled/interactionownership.adb)
set_tests_properties(ada.interactioncalls ada.interactionownership
    determinism.interactioncalls determinism.interactionownership PROPERTIES TIMEOUT 60)
add_ada_error_test(interactionerrors SOURCES interactions/interactionerrors.adb)
set_tests_properties(error.interactionerrors PROPERTIES TIMEOUT 10)
add_test(NAME semantics.interaction_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckInteractionModes.cmake)
set_tests_properties(semantics.interaction_modes PROPERTIES TIMEOUT 60)

foreach(task_case tasks taskallocations)
    add_ada_test(${task_case} SOURCES ${task_case}.adb)
    add_determinism_test(${task_case} SOURCE ${task_case}.adb)
    set_tests_properties(ada.${task_case} determinism.${task_case} PROPERTIES TIMEOUT 30)
endforeach()
add_ada_test(tasklibrarymain SOURCES tasks/tasklibrarymain.adb)
add_determinism_test(tasklibrarymain SOURCE tasks/tasklibrarymain.adb)
add_ada_error_test(taskerrors SOURCES tasks/taskerrors.adb)
set_tests_properties(ada.tasklibrarymain determinism.tasklibrarymain error.taskerrors PROPERTIES TIMEOUT 30)
add_determinism_test(taskconcurrency SOURCE tasks/taskconcurrency.adb)
add_test(NAME runtime.tasking
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/tasks/taskconcurrency.adb
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/TaskingHooks.c
        -DWORKDIR=${ADA_TEST_WORK_DIR}/tasking
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(runtime.tasking determinism.taskconcurrency PROPERTIES TIMEOUT 60)
add_executable(tasking_runtime_test TaskingRuntimeTest.c)
target_link_libraries(tasking_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.tasking_lifecycle COMMAND tasking_runtime_test)
set_tests_properties(runtime.tasking_lifecycle PROPERTIES TIMEOUT 30)
add_ada_test(taskactivationfailure SOURCES tasks/taskactivationfailure.adb
    EXIT_CODE 1 STDERR tasks/taskactivationfailure.stderr)
add_determinism_test(taskactivationfailure SOURCE tasks/taskactivationfailure.adb)
set_tests_properties(ada.taskactivationfailure determinism.taskactivationfailure PROPERTIES TIMEOUT 30)
add_ada_error_test(tasklegalityerrors SOURCES tasks/tasklegalityerrors.adb)
set_tests_properties(error.tasklegalityerrors PROPERTIES TIMEOUT 10)

add_ada_test(protectedobjects SOURCES tasks/protectedobjects.adb)
add_determinism_test(protectedobjects SOURCE tasks/protectedobjects.adb)
set_tests_properties(ada.protectedobjects determinism.protectedobjects PROPERTIES TIMEOUT 30)
add_ada_test(protectedconcurrency SOURCES tasks/protectedconcurrency.adb)
add_determinism_test(protectedconcurrency SOURCE tasks/protectedconcurrency.adb)
set_tests_properties(ada.protectedconcurrency determinism.protectedconcurrency PROPERTIES TIMEOUT 30)
add_ada_test(protectedlibrary SOURCES tasks/protectedlibrary.adb)
add_determinism_test(protectedlibrary SOURCE tasks/protectedlibrary.adb)
set_tests_properties(ada.protectedlibrary determinism.protectedlibrary PROPERTIES TIMEOUT 30)

add_executable(protected_runtime_test ProtectedRuntimeTest.c)
target_link_libraries(protected_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.protected_lifecycle COMMAND protected_runtime_test)
set_tests_properties(runtime.protected_lifecycle PROPERTIES TIMEOUT 30)
foreach(protected_case protectedobjects protectedconcurrency protectedlibrary protectedgenerics protectedcleanup)
    ada_test_fixture(source_path "${protected_case}.adb")
    ada_test_fixture(expected_path "${protected_case}.expected")
    add_test(NAME runtime.${protected_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${protected_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(runtime.${protected_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_ada_error_test(protectederrors SOURCES tasks/protectederrors.adb)
add_ada_error_test(protectedsyntaxerrors SOURCES tasks/protectedsyntaxerrors.adb)
set_tests_properties(error.protectederrors error.protectedsyntaxerrors PROPERTIES TIMEOUT 10)

foreach(protected_case protectedgenerics protectedcleanup)
    add_ada_test(${protected_case} SOURCES ${protected_case}.adb)
    add_determinism_test(${protected_case} SOURCE ${protected_case}.adb)
    set_tests_properties(ada.${protected_case} determinism.${protected_case} PROPERTIES TIMEOUT 30)
endforeach()
add_ada_error_test(protectedlegalityerrors SOURCES tasks/protectedlegalityerrors.adb)
set_tests_properties(error.protectedlegalityerrors PROPERTIES TIMEOUT 10)
add_ada_error_test(protectedcompletionerrors SOURCES tasks/protectedcompletionerrors.adb)
set_tests_properties(error.protectedcompletionerrors PROPERTIES TIMEOUT 10)
add_ada_error_test(protectedgenericerrors SOURCES tasks/protectedgenericerrors.adb)
set_tests_properties(error.protectedgenericerrors PROPERTIES TIMEOUT 10)

foreach(entry_case protectedentries entryproducer entrylibrary entrygenerics)
    add_ada_test(${entry_case} SOURCES ${entry_case}.adb)
    add_determinism_test(${entry_case} SOURCE ${entry_case}.adb)
    ada_test_fixture(source_path "${entry_case}.adb")
    ada_test_fixture(expected_path "${entry_case}.expected")
    add_test(NAME runtime.${entry_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${entry_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${entry_case} determinism.${entry_case}
        runtime.${entry_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_executable(entry_runtime_test EntryRuntimeTest.c)
target_link_libraries(entry_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.entry_lifecycle COMMAND entry_runtime_test)
set_tests_properties(runtime.entry_lifecycle PROPERTIES TIMEOUT 30)
add_ada_test(entryexceptions SOURCES tasks/entryexceptions.adb)
add_determinism_test(entryexceptions SOURCE tasks/entryexceptions.adb)
add_ada_error_test(entryerrors SOURCES tasks/entryerrors.adb)
add_test(NAME runtime.entryexceptions_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/tasks/entryexceptions.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/tasks/entryexceptions.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/entryexceptions-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(ada.entryexceptions determinism.entryexceptions
    runtime.entryexceptions_modes error.entryerrors PROPERTIES TIMEOUT 30)

foreach(time_case timevalues timedelays timelibrary)
    add_ada_test(${time_case} SOURCES ${time_case}.adb)
    add_determinism_test(${time_case} SOURCE ${time_case}.adb)
    ada_test_fixture(source_path "${time_case}.adb")
    ada_test_fixture(expected_path "${time_case}.expected")
    add_test(NAME runtime.${time_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${time_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${time_case} determinism.${time_case}
        runtime.${time_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_ada_error_test(timeerrors SOURCES tasks/timeerrors.adb)
set_tests_properties(error.timeerrors PROPERTIES TIMEOUT 10)
add_executable(time_runtime_test TimeRuntimeTest.c)
target_link_libraries(time_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.time_lifecycle COMMAND time_runtime_test)
set_tests_properties(runtime.time_lifecycle PROPERTIES TIMEOUT 30)

foreach(shared_case sharedscalars sharedconcurrency sharedlibrary)
    add_ada_test(${shared_case} SOURCES ${shared_case}.adb)
    add_determinism_test(${shared_case} SOURCE ${shared_case}.adb)
    ada_test_fixture(source_path "${shared_case}.adb")
    ada_test_fixture(expected_path "${shared_case}.expected")
    add_test(NAME runtime.${shared_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${shared_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${shared_case} determinism.${shared_case}
        runtime.${shared_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_determinism_test(sharedimports SOURCE tasks/sharedimports.adb)
add_test(NAME runtime.sharedimports_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/tasks/sharedimports.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/tasks/sharedimports.expected
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/SharedHooks.c
        -DWORKDIR=${ADA_TEST_WORK_DIR}/sharedimports-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(runtime.sharedimports_modes determinism.sharedimports PROPERTIES TIMEOUT 60)
foreach(shared_error sharederrors sharedcompletionerrors sharedsyntaxerrors)
    add_ada_error_test(${shared_error} SOURCES ${shared_error}.adb)
    set_tests_properties(error.${shared_error} PROPERTIES TIMEOUT 10)
endforeach()
add_test(NAME semantics.shared_ir
    COMMAND ${CMAKE_COMMAND}
        -DADAC=$<TARGET_FILE:adac>
        -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/ada/tasks
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckSharedIR.cmake)
set_tests_properties(semantics.shared_ir PROPERTIES TIMEOUT 30)

foreach(identity_case taskidentity taskidentitylifecycle taskidentityobjects)
    add_ada_test(${identity_case} SOURCES ${identity_case}.adb)
    add_determinism_test(${identity_case} SOURCE ${identity_case}.adb)
    ada_test_fixture(source_path "${identity_case}.adb")
    ada_test_fixture(expected_path "${identity_case}.expected")
    add_test(NAME runtime.${identity_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${identity_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${identity_case} determinism.${identity_case}
        runtime.${identity_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_ada_error_test(taskidentityerrors SOURCES tasks/taskidentityerrors.adb)

# Numeric and array boundaries in whole/separate compilation, deterministic
# IR, and several independent Ada task contexts.
foreach(boundary_case realedges realtasking floatattributes floatattributetasking realvalue realvaluetasking derivedscalaroperations arraydescriptors arraydescriptortasking concatenation concattasking)
    add_ada_test(${boundary_case} SOURCES ${boundary_case}.adb)
    add_determinism_test(${boundary_case} SOURCE ${boundary_case}.adb)
    ada_test_fixture(source_path "${boundary_case}.adb")
    ada_test_fixture(expected_path "${boundary_case}.expected")
    add_test(NAME runtime.${boundary_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${boundary_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${boundary_case} determinism.${boundary_case}
        runtime.${boundary_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_ada_error_test(floatattributeerrors SOURCES numerics/floatattributeerrors.adb)
add_executable(float_attributes_test FloatAttributesTest.c)
target_link_libraries(float_attributes_test PRIVATE adart)
add_test(NAME runtime.float_attributes COMMAND float_attributes_test)
set_tests_properties(runtime.float_attributes PROPERTIES TIMEOUT 20)
add_ada_error_test(realedgeerrors SOURCES numerics/realedgeerrors.adb)
set_tests_properties(error.realedgeerrors PROPERTIES TIMEOUT 10)
add_executable(real_arithmetic_test RealArithmeticTest.c)
target_link_libraries(real_arithmetic_test PRIVATE adart)
add_test(NAME runtime.real_arithmetic COMMAND real_arithmetic_test)
set_tests_properties(runtime.real_arithmetic PROPERTIES TIMEOUT 20)

add_ada_error_test(realvalueerrors SOURCES numerics/realvalueerrors.adb)
add_executable(real_value_test RealValueTest.c)
target_link_libraries(real_value_test PRIVATE adart)
add_test(NAME runtime.real_value COMMAND real_value_test)
set_tests_properties(runtime.real_value PROPERTIES TIMEOUT 20)

add_executable(array_descriptor_test ArrayDescriptorTest.c)
target_link_libraries(array_descriptor_test PRIVATE adart)
add_test(NAME runtime.array_descriptors COMMAND array_descriptor_test)
set_tests_properties(runtime.array_descriptors PROPERTIES TIMEOUT 20)
add_ada_error_test(arraydescriptorerrors SOURCES arrays/arraydescriptorerrors.adb)
add_ada_error_test(arrayframeerrors SOURCES arrays/arrayframeerrors.adb)
add_ada_test(arraydescriptorstreams SOURCES arrays/arraydescriptorstreams.adb)
add_determinism_test(arraydescriptorstreams SOURCE arrays/arraydescriptorstreams.adb)
add_ada_error_test(concaterrors SOURCES arrays/concaterrors.adb)
add_executable(concat_runtime_test ConcatRuntimeTest.c)
target_link_libraries(concat_runtime_test PRIVATE adart)
add_test(NAME runtime.concatenation COMMAND concat_runtime_test)
set_tests_properties(runtime.concatenation PROPERTIES TIMEOUT 20)

add_executable(modular_runtime_test ModularRuntimeTest.c)
target_link_libraries(modular_runtime_test PRIVATE adart)
add_test(NAME runtime.modular COMMAND modular_runtime_test)
set_tests_properties(runtime.modular PROPERTIES TIMEOUT 20)

foreach(modular_case modularwide modulario modulartasking)
    ada_test_fixture(source_path "${modular_case}.adb")
    ada_test_fixture(expected_path "${modular_case}.expected")
    add_test(NAME runtime.${modular_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${modular_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${modular_case} determinism.${modular_case}
        runtime.${modular_case}_modes PROPERTIES TIMEOUT 60)
endforeach()

add_ada_test(fixedwide SOURCES numerics/fixedwide.adb)
add_determinism_test(fixedwide SOURCE numerics/fixedwide.adb)

add_ada_test(fixedstreams SOURCES numerics/fixedstreams.adb)
add_determinism_test(fixedstreams SOURCE numerics/fixedstreams.adb)

add_ada_test(fixedwidechecks SOURCES numerics/fixedwidechecks.adb)
add_ada_error_test(fixedwideerrors SOURCES numerics/fixedwideerrors.adb)
add_ada_error_test(fixedoperatorerrors SOURCES numerics/fixedoperatorerrors.adb)

add_executable(fixed_arithmetic_test FixedArithmeticTest.c)
target_link_libraries(fixed_arithmetic_test PRIVATE adart)
add_test(NAME runtime.fixed_arithmetic COMMAND fixed_arithmetic_test)
set_tests_properties(runtime.fixed_arithmetic PROPERTIES TIMEOUT 20)

add_ada_test(fixedtasking SOURCES numerics/fixedtasking.adb)
add_determinism_test(fixedtasking SOURCE numerics/fixedtasking.adb)
foreach(fixed_case fixedwide fixedstreams fixedtasking)
    ada_test_fixture(source_path "${fixed_case}.adb")
    ada_test_fixture(expected_path "${fixed_case}.expected")
    add_test(NAME runtime.${fixed_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${fixed_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${fixed_case} determinism.${fixed_case}
        runtime.${fixed_case}_modes PROPERTIES TIMEOUT 60)
endforeach()

add_ada_test(fixedoperators SOURCES numerics/fixedoperators.adb)
add_determinism_test(fixedoperators SOURCE numerics/fixedoperators.adb)

add_ada_test(decimalpoint SOURCES numerics/decimalpoint.adb)
add_determinism_test(decimalpoint SOURCE numerics/decimalpoint.adb)
add_ada_test(decimalchecks SOURCES numerics/decimalchecks.adb)
add_ada_error_test(decimalerrors SOURCES numerics/decimalerrors.adb)
add_ada_test(decimalstreams SOURCES numerics/decimalstreams.adb)
add_determinism_test(decimalstreams SOURCE numerics/decimalstreams.adb)
add_ada_test(decimaltasking SOURCES numerics/decimaltasking.adb)
add_determinism_test(decimaltasking SOURCE numerics/decimaltasking.adb)
foreach(decimal_case decimalpoint decimalchecks decimalstreams decimaltasking)
    ada_test_fixture(source_path "${decimal_case}.adb")
    ada_test_fixture(expected_path "${decimal_case}.expected")
    add_test(NAME runtime.${decimal_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${decimal_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(runtime.${decimal_case}_modes PROPERTIES TIMEOUT 60)
endforeach()

add_executable(decimal_runtime_test DecimalRuntimeTest.c)
target_link_libraries(decimal_runtime_test PRIVATE adart)
add_test(NAME runtime.decimal COMMAND decimal_runtime_test)
set_tests_properties(runtime.decimal PROPERTIES TIMEOUT 20)

add_ada_error_test(alignmenterrors SOURCES representation/alignmenterrors.adb)
add_determinism_test(alignmentclauses SOURCE representation/alignmentclauses.adb)
add_test(NAME runtime.alignmentclauses_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/Alignment.c
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/representation/alignmentclauses.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/representation/alignmentclauses.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/alignmentclauses-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(error.alignmenterrors determinism.alignmentclauses runtime.alignmentclauses_modes
    PROPERTIES TIMEOUT 60)

add_ada_error_test(enumrepresentationerrors SOURCES representation/enumrepresentationerrors.adb)
add_ada_test(enumstreams SOURCES types/enumstreams.adb)
foreach(enum_case enumrepresentation enumstreams enumtasking)
    add_determinism_test(${enum_case} SOURCE ${enum_case}.adb)
    ada_test_fixture(source_path "${enum_case}.adb")
    ada_test_fixture(expected_path "${enum_case}.expected")
    add_test(NAME runtime.${enum_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/EnumRepresentation.c
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${enum_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(runtime.${enum_case}_modes determinism.${enum_case} PROPERTIES TIMEOUT 60)
endforeach()

add_executable(enumeration_runtime_test EnumerationRuntimeTest.c)
target_link_libraries(enumeration_runtime_test PRIVATE adart)
add_test(NAME runtime.enumeration COMMAND enumeration_runtime_test)
set_tests_properties(runtime.enumeration PROPERTIES TIMEOUT 20)
set_tests_properties(error.enumrepresentationerrors ada.enumstreams PROPERTIES TIMEOUT 60)

add_ada_error_test(sizestorageerrors SOURCES representation/sizestorageerrors.adb)
foreach(size_case sizestorage sizestreams sizetasking)
    add_determinism_test(${size_case} SOURCE ${size_case}.adb)
    ada_test_fixture(source_path "${size_case}.adb")
    ada_test_fixture(expected_path "${size_case}.expected")
    add_test(NAME runtime.${size_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/SizeStorage.c
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${size_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(runtime.${size_case}_modes determinism.${size_case} PROPERTIES TIMEOUT 60)
endforeach()
set_tests_properties(error.sizestorageerrors PROPERTIES TIMEOUT 60)

add_executable(type_storage_test TypeStorageTest.cpp ${PROJECT_SOURCE_DIR}/adac/Type.cpp)
target_compile_features(type_storage_test PRIVATE cxx_std_17)
add_test(NAME semantics.type_storage COMMAND type_storage_test)
set_tests_properties(semantics.type_storage PROPERTIES TIMEOUT 20)

add_determinism_test(checkedarithmetic SOURCE numerics/checkedarithmetic.adb)
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
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/numerics/checkedarithmetic.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/numerics/checkedarithmetic.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/checkedarithmetic-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(runtime.checkedarithmetic_modes determinism.checkedarithmetic PROPERTIES TIMEOUT 60)

add_ada_test(checkedtasking SOURCES numerics/checkedtasking.adb)
add_determinism_test(checkedtasking SOURCE numerics/checkedtasking.adb)
add_test(NAME runtime.checkedtasking_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DFORBIDDEN_SYMBOL=__ada_integer_operation
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/numerics/checkedtasking.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/numerics/checkedtasking.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/checkedtasking-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(ada.checkedtasking runtime.checkedtasking_modes
    determinism.checkedtasking PROPERTIES TIMEOUT 60)

add_ada_test(namedoperators SOURCES subprograms/namedoperators.adb)
add_determinism_test(namedoperators SOURCE subprograms/namedoperators.adb)
add_test(NAME runtime.namedoperators_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/subprograms/namedoperators.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/subprograms/namedoperators.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/namedoperators-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(ada.namedoperators determinism.namedoperators
    runtime.namedoperators_modes PROPERTIES TIMEOUT 60)
add_ada_error_test(namedoperatorerrors SOURCES subprograms/namedoperatorerrors.adb)
add_ada_error_test(operatorvisibilityerrors SOURCES subprograms/operatorvisibilityerrors.adb)
add_ada_error_test(operatorchildvisibilityerrors SOURCES subprograms/operatorchildvisibilityerrors.adb)
add_ada_error_test(arrayindexerrors SOURCES arrays/arrayindexerrors.adb)
add_ada_error_test(enclosingnameerrors SOURCES subprograms/enclosingnameerrors.adb)
add_ada_error_test(caseselectorerrors SOURCES control_flow/caseselectorerrors.adb)
add_ada_error_test(realnumbererrors SOURCES numerics/realnumbererrors.adb)
add_ada_error_test(supportedrejectionerrors SOURCES syntax/supportedrejectionerrors.adb)
set_tests_properties(error.namedoperatorerrors PROPERTIES TIMEOUT 60)

add_executable(abort_deferral_test AbortDeferralTest.c)
target_link_libraries(abort_deferral_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.abort_deferral COMMAND abort_deferral_test)
set_tests_properties(runtime.abort_deferral PROPERTIES TIMEOUT 30)
add_determinism_test(abortdeferral SOURCE tasks/abortdeferral.adb)
add_test(NAME runtime.abortdeferral_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/tasks/abortdeferral.adb
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/AbortDeferralProbe.c
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/tasks/abortdeferral.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/abortdeferral-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(determinism.abortdeferral runtime.abortdeferral_modes PROPERTIES TIMEOUT 60)

add_executable(shared_collections_test SharedCollectionsTest.c)
target_link_libraries(shared_collections_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.shared_collections COMMAND shared_collections_test)
set_tests_properties(runtime.shared_collections PROPERTIES TIMEOUT 60)
foreach(collection_case sharedcollections sharedtaskcollections)
    add_ada_test(${collection_case} SOURCES ${collection_case}.adb)
    add_determinism_test(${collection_case} SOURCE ${collection_case}.adb)
    ada_test_fixture(source_path "${collection_case}.adb")
    ada_test_fixture(expected_path "${collection_case}.expected")
    add_test(NAME runtime.${collection_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${collection_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${collection_case} determinism.${collection_case}
        runtime.${collection_case}_modes PROPERTIES TIMEOUT 60)
endforeach()

add_executable(scalar_bounds_test ScalarBoundsTest.c)
target_link_libraries(scalar_bounds_test PRIVATE adart)
add_test(NAME runtime.scalar_bounds COMMAND scalar_bounds_test)
set_tests_properties(runtime.scalar_bounds PROPERTIES TIMEOUT 30)
add_ada_error_test(scalardynamicerrors SOURCES types/scalardynamicerrors.adb)
foreach(dynamic_case exceptionlibrary exceptionlibrarytasking streammemory streamrecords streamclasswide streamsizes pragmaunitbundle pragmasuppressed pragmaunsuppressed pragmanoreturn pragmapolicies pragmaignore pragmalibrary_main genericcontractlibrary_main genericcontractnested genericcontractreplay genericcontractregions genericlimitedcleanup genericlimitedtagged genericlimitedobjects genericlimitedarrays genericlimitedlibrary_main genericaccesslifetime genericaccessobjectlibrary_main genericaccessobjects genericanonymousobjects genericaccesscallbacks genericpackagepartial genericpackagepartialtypes genericpackagepartialoverloads genericpartialpackagelibrary_main genericpackages genericpackagecontracts genericpackagecallables genericpackagelibrary_main genericarrayconstraints genericarraycomponents genericarrayconstraintlibrary_main genericliterals genericliterallibrary_main genericattributes genericattributenumeric genericattributelibrary_main genericnull genericnullcomposites genericnulllibrary_main genericabstract genericabstractinterfaces genericabstractoperators genericabstractlibrary_main genericclasswide genericclasswideinterfaces genericclasswidelibrary_main genericsynchronized genericsynchronizednested genericsynchronizedlibrary_main genericdiscriminantviews genericdiscriminants genericdefinite genericdiscriminantlibrary_main genericinterfacelibrary_main genericinterfaceformals genericinterfacecategories genericinterfacenested genericderiveddiscriminants genericderivedlibrary_main genericderived genericderivedtagged genericderivednested genericaccess genericaccessmatching genericprivatearrays importedcallbacks inheritedcallbacks inheritedcallbacklibrary callbackextensiontasking nullexclusions nullexclusiontasking anonymousarrayobjects anonymousaccess anonymousaccessrecords anonymousaccessarrays anonymouscoextensions anonymousprotected anonymousdispatch anonymousaccesstasking generalaccess generalaccessarrays generalaccesschecks generalaccesstasking recordcontrolled controlledvariants controlledvariantlimited scalardynamic scalarstreams scalartasking
    runtimearraylibrary runtimearraylibrarytasking runtimearraylibrarylifetime runtimearraylibraryworkers
    runtimecomponents runtimeallocators runtimecomponentstreams runtimecomponentcleanup runtimecomponenttasking runtimecomponentchecks
    boxaggregates boxcleanup boxchecks boxtasking defaultdiscriminants defaultdiscriminantstasking defaultdiscriminantslibrary
    runtimediscriminants discriminantcomponents discriminantcleanup discriminanttasking controlledadjustment operatorvisibility arrayindexlegality enclosingnames caseselector realnumbers supportedrejections abstractsubprograms abstractinheritance controllingaccessresults overridingnull classwideconstraints taggedvariantgroups taggedviews taggedruntime taggeddependent privatedelayed taggedinstances taggedcontrolled interfaces interfacediamonds interfaceownership interfacecategories interfaceprofiles interfacegenerics)
    add_ada_test(${dynamic_case} SOURCES ${dynamic_case}.adb)
    add_determinism_test(${dynamic_case} SOURCE ${dynamic_case}.adb)
    ada_test_fixture(source_path "${dynamic_case}.adb")
    ada_test_fixture(expected_path "${dynamic_case}.expected")
    add_test(NAME runtime.${dynamic_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${dynamic_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${dynamic_case} determinism.${dynamic_case}
        runtime.${dynamic_case}_modes PROPERTIES TIMEOUT 60)
endforeach()

add_ada_error_test(runtimearraylibraryerrors SOURCES arrays/runtimearraylibraryerrors.ads)

add_ada_test(runtimearraylibraryfailure SOURCES controlled/runtimearraylibraryfailure.adb
    EXIT_CODE 1 STDERR controlled/runtimearraylibraryfailure.stderr)
add_determinism_test(runtimearraylibraryfailure SOURCE controlled/runtimearraylibraryfailure.adb)

foreach(array_failure bounds shape overflow)
    add_ada_test(runtimearraylibrary${array_failure} SOURCES runtimearraylibrary${array_failure}.adb
        EXIT_CODE 1 STDERR runtimearraylibrary${array_failure}.stderr)
    add_determinism_test(runtimearraylibrary${array_failure} SOURCE runtimearraylibrary${array_failure}.adb)
endforeach()

add_executable(runtime_layout_test RuntimeLayoutTest.c)
target_link_libraries(runtime_layout_test PRIVATE adart)
add_test(NAME runtime.dynamic_layout COMMAND runtime_layout_test)
set_tests_properties(runtime.dynamic_layout PROPERTIES TIMEOUT 30)

add_ada_error_test(runtimecomponenterrors SOURCES arrays/runtimecomponenterrors.adb)
add_ada_test(runtimecomponentlate SOURCES arrays/runtimecomponentlate.ads arrays/runtimecomponentlate_main.adb)
add_determinism_test(runtimecomponentlate SOURCE arrays/runtimecomponentlate_main.adb)

add_ada_error_test(boxpositionalerrors SOURCES arrays/boxpositionalerrors.adb)

add_ada_error_test(defaultdiscriminanterrors SOURCES types/defaultdiscriminanterrors.adb)

add_ada_error_test(discriminantcomponenterrors SOURCES types/discriminantcomponenterrors.adb)

set_tests_properties(error.discriminantcomponenterrors PROPERTIES TIMEOUT 10)

foreach(record_case recordrepresentation recorddependent)
    add_determinism_test(${record_case} SOURCE ${record_case}.adb)
    ada_test_fixture(source_path "${record_case}.adb")
    ada_test_fixture(expected_path "${record_case}.expected")
    add_test(NAME runtime.${record_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/SizeStorage.c
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${record_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(runtime.${record_case}_modes PROPERTIES TIMEOUT 60)
endforeach()

add_ada_error_test(recordrepresentationerrors SOURCES representation/recordrepresentationerrors.adb)

add_ada_error_test(generalaccesssyntaxerrors SOURCES access/generalaccesssyntaxerrors.adb)

add_executable(access_levels_test AccessLevelsTest.c)
target_link_libraries(access_levels_test PRIVATE adart)
add_test(NAME runtime.access_levels COMMAND access_levels_test)
set_tests_properties(runtime.access_levels PROPERTIES TIMEOUT 30)

add_ada_error_test(anonymousaccesserrors SOURCES access/anonymousaccesserrors.adb)
add_ada_error_test(anonymousaccessmodeerrors SOURCES access/anonymousaccessmodeerrors.adb)

add_ada_error_test(anonymousarrayerrors SOURCES access/anonymousarrayerrors.adb)
add_ada_error_test(anonymousarrayconstanterrors SOURCES access/anonymousarrayconstanterrors.ads)

add_ada_error_test(nullexclusionerrors SOURCES access/nullexclusionerrors.adb)
add_ada_error_test(nullexclusiongenericerrors SOURCES access/nullexclusiongenericerrors.adb)

add_ada_error_test(nullexclusionsyntaxerrors SOURCES access/nullexclusionsyntaxerrors.adb)

add_ada_error_test(callbackextensionerrors SOURCES access/callbackextensionerrors.adb)

add_determinism_test(importedcallbackabi SOURCE access/importedcallbackabi.adb)
add_test(NAME runtime.importedcallbackabi_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/access/importedcallbackabi.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/access/importedcallbackabi.expected
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/ImportedCallbacks.c
        -DWORKDIR=${ADA_TEST_WORK_DIR}/importedcallbackabi-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(runtime.importedcallbackabi_modes PROPERTIES TIMEOUT 60)

# Phase 6e: labels, transfer legality and master/finalization cleanup.
add_ada_test(gotostatements SOURCES control_flow/gotostatements.adb)
add_ada_test(gotocleanup SOURCES control_flow/gotocleanup.adb)
add_ada_error_test(gotoerrors SOURCES control_flow/gotoerrors.adb)
add_ada_error_test(gotoparsererrors SOURCES control_flow/gotoparsererrors.adb)

foreach(goto_case gotostatements gotocleanup)
    add_determinism_test(${goto_case} SOURCE ${goto_case}.adb)
    ada_test_fixture(source_path "${goto_case}.adb")
    ada_test_fixture(expected_path "${goto_case}.expected")
    add_test(NAME runtime.${goto_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${goto_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${goto_case} runtime.${goto_case}_modes PROPERTIES TIMEOUT 60)
endforeach()

# Phase 6e: package and generic renamed views, including library compatibility.
foreach(renaming_case unitrenaming renaminglibrary)
    add_ada_test(${renaming_case} SOURCES ${renaming_case}.adb)
    add_determinism_test(${renaming_case} SOURCE ${renaming_case}.adb)
    ada_test_fixture(source_path "${renaming_case}.adb")
    ada_test_fixture(expected_path "${renaming_case}.expected")
    add_test(NAME runtime.${renaming_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${renaming_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${renaming_case} runtime.${renaming_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_ada_error_test(unitrenamingerrors SOURCES units/unitrenamingerrors.adb)
add_ada_error_test(renamingvisibilityerrors SOURCES units/renamingvisibilityerrors.adb)
add_ada_error_test(renamingparsererrors SOURCES units/renamingparsererrors.adb)

add_test(NAME semantics.unit_renaming
    COMMAND ${CMAKE_COMMAND}
        -DADAC=$<TARGET_FILE:adac>
        -DSOURCES=${CMAKE_CURRENT_SOURCE_DIR}/ada/units
        -DWORKDIR=${ADA_TEST_WORK_DIR}/unit-renaming-errors
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckUnitRenaming.cmake)
set_tests_properties(semantics.unit_renaming PROPERTIES TIMEOUT 30)

# File objects expose limited private views in all four I/O packages.
add_ada_error_test(iofileprivacyerrors SOURCES io/iofileprivacyerrors.adb)

# Phase 6e: tagged prefixed calls through ordinary call lowering.
foreach(prefixed_case prefixedcalls prefixedcleanup)
    add_ada_test(${prefixed_case} SOURCES ${prefixed_case}.adb)
    add_determinism_test(${prefixed_case} SOURCE ${prefixed_case}.adb)
    ada_test_fixture(source_path "${prefixed_case}.adb")
    ada_test_fixture(expected_path "${prefixed_case}.expected")
    add_test(NAME runtime.${prefixed_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${prefixed_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${prefixed_case} runtime.${prefixed_case}_modes PROPERTIES TIMEOUT 60)
endforeach()
add_ada_error_test(prefixederrors SOURCES tagged/prefixederrors.adb)
add_ada_error_test(prefixedviewerrors SOURCES tagged/prefixedviewerrors.adb)

# Phase 6e: limited/private contexts and incomplete type views.
foreach(context_case limitedwith privatewith taggedincomplete)
    add_ada_test(${context_case} SOURCES ${context_case}.adb)
    add_determinism_test(${context_case} SOURCE ${context_case}.adb)
    ada_test_fixture(source_path "${context_case}.adb")
    ada_test_fixture(expected_path "${context_case}.expected")
    add_test(NAME runtime.${context_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${context_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${context_case} runtime.${context_case}_modes PROPERTIES TIMEOUT 60)
endforeach()

add_test(NAME semantics.context_views
    COMMAND ${CMAKE_COMMAND}
        -DADAC=$<TARGET_FILE:adac>
        -DSOURCES=${CMAKE_CURRENT_SOURCE_DIR}/ada/units
        -DWORKDIR=${ADA_TEST_WORK_DIR}/context-views
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckContextViews.cmake)
set_tests_properties(semantics.context_views PROPERTIES TIMEOUT 60)

add_test(NAME loading.context_dependencies
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckContextDependencies.cmake)
set_tests_properties(loading.context_dependencies PROPERTIES TIMEOUT 90)

# RM 13.10: unchecked object access retains ordinary object-view rules.
foreach(unchecked_case uncheckedaccess uncheckedaccessir)
    add_ada_test(${unchecked_case} SOURCES ${unchecked_case}.adb)
    add_determinism_test(${unchecked_case} SOURCE ${unchecked_case}.adb)
    ada_test_fixture(source_path "${unchecked_case}.adb")
    ada_test_fixture(expected_path "${unchecked_case}.expected")
    add_test(NAME runtime.${unchecked_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${unchecked_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${unchecked_case} runtime.${unchecked_case}_modes PROPERTIES TIMEOUT 60)
endforeach()

add_ada_error_test(uncheckedaccesserrors SOURCES access/uncheckedaccesserrors.adb)
add_ada_error_test(uncheckedsharederrors SOURCES access/uncheckedsharederrors.adb)
add_test(NAME semantics.unchecked_access
    COMMAND ${CMAKE_COMMAND}
        -DADAC=$<TARGET_FILE:adac>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/access/uncheckedaccessir.adb
        -DWORKDIR=${ADA_TEST_WORK_DIR}/unchecked-access-ir
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckUncheckedAccess.cmake)
set_tests_properties(semantics.unchecked_access PROPERTIES TIMEOUT 30)

# Subprogram Access of explicit callback dereferences, preserving closures.
add_ada_test(callbackdereference SOURCES access/callbackdereference.adb)
add_determinism_test(callbackdereference SOURCE access/callbackdereference.adb)
add_test(NAME runtime.callbackdereference_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/access/callbackdereference.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/access/callbackdereference.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/callbackdereference-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(ada.callbackdereference runtime.callbackdereference_modes PROPERTIES TIMEOUT 60)
add_ada_error_test(callbackdereferenceerrors SOURCES access/callbackdereferenceerrors.adb)

# Phase 6e: subprogram/package subunits, lexical scope and parent object inputs.
add_ada_test(subunits SOURCES units/subunits.adb)
add_determinism_test(subunits SOURCE units/subunits.adb)
add_test(NAME runtime.subunits_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/units/subunits.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/units/subunits.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/subunits-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(ada.subunits runtime.subunits_modes PROPERTIES TIMEOUT 90)

add_test(NAME semantics.subunits
    COMMAND ${CMAKE_COMMAND}
        -DADAC=$<TARGET_FILE:adac>
        -DADA=$<TARGET_FILE:ada>
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckSubunits.cmake)
set_tests_properties(semantics.subunits PROPERTIES TIMEOUT 90)

# Phase 6e: ordinary extended returns and general limited result construction.
foreach(return_case extendedreturns limitedresults returncleanup)
    add_ada_test(${return_case} SOURCES ${return_case}.adb)
    add_determinism_test(${return_case} SOURCE ${return_case}.adb)
    ada_test_fixture(source_path "${return_case}.adb")
    ada_test_fixture(expected_path "${return_case}.expected")
    add_test(NAME runtime.${return_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${return_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${return_case} runtime.${return_case}_modes PROPERTIES TIMEOUT 90)
endforeach()

add_test(NAME semantics.returns
    COMMAND ${CMAKE_COMMAND}
        -DADAC=$<TARGET_FILE:adac>
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckReturnSemantics.cmake)
set_tests_properties(semantics.returns PROPERTIES TIMEOUT 60)

# Ada 2005 abstract operations: declaration, use and overriding legality.
add_ada_error_test(abstractdeclarationerrors SOURCES tagged/abstractdeclarationerrors.adb)
add_ada_error_test(abstractcallerrors SOURCES tagged/abstractcallerrors.adb)
add_ada_error_test(abstractuseerrors SOURCES tagged/abstractuseerrors.adb)
add_ada_error_test(abstractoverrideerrors SOURCES tagged/abstractoverrideerrors.adb)

# Ada 2005 overriding, null-procedure and controlling access-result legality.
add_ada_error_test(overridinglegalityerrors SOURCES tagged/overridinglegalityerrors.adb)
add_ada_error_test(nullprocedureerrors SOURCES tagged/nullprocedureerrors.adb)
add_ada_error_test(nullgenericerrors SOURCES generics/nullgenericerrors.adb)
add_ada_error_test(protectedindicatorerrors SOURCES tasks/protectedindicatorerrors.adb)
add_ada_error_test(protectednullerrors SOURCES tasks/protectednullerrors.adb)
add_ada_error_test(accessresultlegalityerrors SOURCES access/accessresultlegalityerrors.adb)
add_ada_error_test(accessresultcontexterrors SOURCES access/access_result_base.ads access/access_result_base.adb access/accessresultcontexterrors.adb)
add_ada_error_test(protectedoverridingerrors SOURCES tasks/protectedoverridingerrors.adb)
add_ada_error_test(nullgenericcompletionerrors SOURCES generics/nullgenericcompletionerrors.adb)
add_ada_error_test(genericattributeerrors SOURCES generics/genericattributeerrors.adb)
add_ada_error_test(genericattributeaccesserrors SOURCES generics/genericattributeaccesserrors.adb)
add_ada_error_test(genericattributestaticerrors SOURCES generics/genericattributestaticerrors.adb)
add_ada_error_test(genericnullmatchingerrors SOURCES generics/genericnullmatchingerrors.adb)
add_ada_error_test(genericnullaccesserrors SOURCES generics/genericnullaccesserrors.adb)
add_ada_error_test(genericnullcontracterrors SOURCES generics/genericnullcontracterrors.adb)
add_ada_error_test(nullformaldefaulterrors SOURCES generics/nullformaldefaulterrors.adb)
add_ada_error_test(accessparameterlegalityerrors SOURCES access/accessparameterlegalityerrors.adb)

add_executable(interface_runtime_test InterfaceRuntimeTest.c)
target_link_libraries(interface_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.interface_maps COMMAND interface_runtime_test)
set_tests_properties(runtime.interface_maps PROPERTIES TIMEOUT 30)

# Phase 6g: formal access contracts are checked even without instances.
add_ada_error_test(genericaccesserrors SOURCES generics/genericaccesserrors.adb)
add_ada_error_test(genericaccesscontracterrors SOURCES generics/genericaccesscontracterrors.adb)
add_ada_error_test(genericprivateerrors SOURCES generics/genericprivateerrors.adb)
add_ada_error_test(genericaccesssyntaxerrors SOURCES generics/genericaccesssyntaxerrors.adb)

# Phase 6g: formal derived categories and inherited contracts.
add_ada_error_test(genericderivederrors SOURCES generics/genericderivederrors.adb)
add_ada_error_test(genericderivedsyntaxerrors SOURCES generics/genericderivedsyntaxerrors.adb)
add_ada_error_test(genericderivedancestorerrors SOURCES generics/genericderivedancestorerrors.adb)
add_ada_error_test(genericinterfaceerrors SOURCES generics/genericinterfaceerrors.adb)
add_ada_error_test(genericinterfaceprogenitorerrors SOURCES generics/genericinterfaceprogenitorerrors.adb)
add_ada_error_test(genericinterfaceobligationerrors SOURCES generics/genericinterfaceobligationerrors.adb)
add_ada_error_test(genericinterfacesyntaxerrors SOURCES generics/genericinterfacesyntaxerrors.adb)

add_ada_error_test(genericdiscriminanterrors SOURCES generics/genericdiscriminanterrors.adb)
add_ada_error_test(genericdefiniteerrors SOURCES generics/genericdefiniteerrors.adb)
add_ada_error_test(genericdiscriminantdefaulterrors SOURCES generics/genericdiscriminantdefaulterrors.adb)
add_ada_error_test(genericdiscriminantsyntaxerrors SOURCES generics/genericdiscriminantsyntaxerrors.adb)
add_ada_error_test(genericderiveddiscriminanterrors SOURCES generics/genericderiveddiscriminanterrors.adb)

add_ada_error_test(genericabstracterrors SOURCES generics/genericabstracterrors.adb)
add_ada_error_test(genericabstractcallerrors SOURCES generics/genericabstractcallerrors.adb)
add_ada_error_test(genericabstractaccesserrors SOURCES generics/genericabstractaccesserrors.adb)
add_ada_error_test(genericabstractsyntaxerrors SOURCES generics/genericabstractsyntaxerrors.adb)
add_ada_error_test(genericclasswideerrors SOURCES generics/genericclasswideerrors.adb)
add_ada_error_test(genericsynchronizederrors SOURCES generics/genericsynchronizederrors.adb)
add_ada_error_test(genericliteralerrors SOURCES generics/genericliteralerrors.adb)
add_ada_error_test(genericarrayconstrainterrors SOURCES generics/genericarrayconstrainterrors.adb)
add_ada_error_test(genericpackageerrors SOURCES generics/genericpackageerrors.adb)
add_ada_error_test(genericpackagecontracterrors SOURCES generics/genericpackagecontracterrors.adb)
add_ada_error_test(genericpackageboxerrors SOURCES generics/genericpackageboxerrors.adb)
add_ada_error_test(genericpackagepartialerrors SOURCES generics/genericpackagepartialerrors.adb)
add_ada_error_test(genericpackagepartialsyntaxerrors SOURCES generics/genericpackagepartialsyntaxerrors.adb)

add_ada_error_test(genericaccessobjecterrors SOURCES generics/genericaccessobjecterrors.adb)

add_ada_error_test(genericlimitederrors SOURCES generics/genericlimitederrors.adb)

add_ada_error_test(genericcontracterrors SOURCES generics/genericcontracterrors.adb)

add_test(NAME semantics.pragmapolicies
    COMMAND ${CMAKE_COMMAND}
        -DADAC=$<TARGET_FILE:adac>
        -DSOURCES=${CMAKE_CURRENT_SOURCE_DIR}/ada/elaboration
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckPragmaPolicies.cmake)

add_ada_error_test(pragmaargumenterrors SOURCES elaboration/pragmaargumenterrors.adb)
add_ada_error_test(pragmanoreturnerrors SOURCES elaboration/pragmanoreturnerrors.adb)
add_ada_error_test(pragmaplacementerrors SOURCES elaboration/pragmaplacementerrors.adb)
add_ada_error_test(pragmaignoredtypeerrors SOURCES elaboration/pragmaignoredtypeerrors.adb)
add_ada_error_test(pragmaimportaccesserrors SOURCES elaboration/pragmaimportaccesserrors.adb)
add_ada_error_test(pragmacontrollederrors SOURCES elaboration/pragmacontrollederrors.adb)
add_test(NAME runtime.pragmaimport_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/PragmaHooks.c
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/elaboration/pragmaimport.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/elaboration/pragmaimport.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/pragmaimport-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(runtime.pragmaimport_modes PROPERTIES TIMEOUT 60)

# Phase 7: predefined I/O, string mappings and controlled Ada collections.
foreach(collection_case iofilelifetime textinputgrammar textiolibrary iolibrarytasking textstringio
    stringmappingtasking stringmappingcallbacks
    unboundedstrings basicvectors vectoraggregates vectorlifetime librarycollectiontasking)
    add_ada_test(${collection_case} SOURCES ${collection_case}.adb)
    add_determinism_test(${collection_case} SOURCE ${collection_case}.adb)
    ada_test_fixture(source_path "${collection_case}.adb")
    ada_test_fixture(expected_path "${collection_case}.expected")
    add_test(NAME runtime.${collection_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${collection_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${collection_case} runtime.${collection_case}_modes
        PROPERTIES TIMEOUT 90)
endforeach()
add_ada_error_test(librarycollectionerrors SOURCES containers/librarycollectionerrors.adb)
add_ada_error_test(stringmappingerrors SOURCES strings/stringmappingerrors.adb)

# Phase 7: Ada 2005 container families and sorting utilities.
foreach(container_case containerfamilies containercursors containersorting containersets
        containerlifetime containertasking containergenerics containerdiscriminants containerstorage)
    add_ada_test(${container_case} SOURCES ${container_case}.adb)
    add_determinism_test(${container_case} SOURCE ${container_case}.adb)
    ada_test_fixture(source_path "${container_case}.adb")
    ada_test_fixture(expected_path "${container_case}.expected")
    add_test(NAME runtime.${container_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DCOMMAND_TIMEOUT=60
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${container_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${container_case} runtime.${container_case}_modes
        determinism.${container_case} PROPERTIES TIMEOUT 180)
endforeach()

add_ada_error_test(containererrors SOURCES containers/containererrors.adb)

# Phase 7 host services and their Calendar prerequisite.
set(host_cases calendarlibrary calendarchildren calendarchecks calendartasking)
if(UNIX OR MINGW)
    list(APPEND host_cases environmentvariables directorieslibrary hostlibrarytasking)
endif()
foreach(host_case IN LISTS host_cases)
    add_ada_test(${host_case} SOURCES ${host_case}.adb)
    add_determinism_test(${host_case} SOURCE ${host_case}.adb)
    ada_test_fixture(source_path "${host_case}.adb")
    ada_test_fixture(expected_path "${host_case}.expected")
    add_test(NAME runtime.${host_case}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=${QBE_EXECUTABLE}
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${host_case}-modes
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${host_case} runtime.${host_case}_modes
        determinism.${host_case} PROPERTIES TIMEOUT 120)
endforeach()

add_ada_error_test(hostlibraryerrors SOURCES library/hostlibraryerrors.adb)

if(UNIX)
    add_executable(host_library_runtime_test HostLibraryRuntimeTest.c)
    target_link_libraries(host_library_runtime_test PRIVATE adart Threads::Threads)
    add_test(NAME runtime.host_library COMMAND host_library_runtime_test)
    set_tests_properties(runtime.host_library PROPERTIES TIMEOUT 30)
endif()

# String Get: shifted/sliced bounds, terminators, null reads and task use.
add_ada_test(textgetstrings SOURCES io/textgetstrings.adb)
add_determinism_test(textgetstrings SOURCE io/textgetstrings.adb)
add_test(NAME runtime.textgetstrings_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/io/textgetstrings.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/io/textgetstrings.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/textgetstrings-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(ada.textgetstrings determinism.textgetstrings
    runtime.textgetstrings_modes PROPERTIES TIMEOUT 90)
add_ada_test(reportfilenames SOURCES ../acats/support/report.ads
    ../acats/support/report.adb driver/reportfilenames.adb)
add_executable(string_get_runtime_test StringGetRuntimeTest.c)
target_link_libraries(string_get_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.string_get COMMAND string_get_runtime_test)
set_tests_properties(runtime.string_get PROPERTIES TIMEOUT 30)

if(WIN32)
    add_executable(windows_host_library_test WindowsHostLibraryTest.c)
    target_link_libraries(windows_host_library_test PRIVATE adart Threads::Threads)
    add_test(NAME runtime.windows_host_library COMMAND windows_host_library_test)
    set_tests_properties(runtime.windows_host_library PROPERTIES TIMEOUT 60)
endif()

add_executable(windows_stack_probe_test WindowsStackProbeTest.cpp ../ada/WindowsStack.cpp)
add_test(NAME driver.windows_stack_probes COMMAND windows_stack_probe_test)
add_ada_test(windowsstackframes SOURCES driver/windowsstackframes.adb)
add_determinism_test(windowsstackframes SOURCE driver/windowsstackframes.adb)
set_tests_properties(ada.windowsstackframes determinism.windowsstackframes PROPERTIES TIMEOUT 60)

add_test(NAME runtime.windowsstackframes_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=${QBE_EXECUTABLE}
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/driver/windowsstackframes.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/driver/windowsstackframes.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/windowsstackframes-modes
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(runtime.windowsstackframes_modes PROPERTIES TIMEOUT 90)

add_determinism_test(streamcontainers SOURCE io/streamcontainers.adb)

add_executable(stream_runtime_test StreamRuntimeTest.c)
target_link_libraries(stream_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.stream_dispatch COMMAND stream_runtime_test)
set_tests_properties(runtime.stream_dispatch PROPERTIES TIMEOUT 30)

add_executable(exception_runtime_test ExceptionRuntimeTest.c)
target_link_libraries(exception_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.exception_library COMMAND exception_runtime_test)
set_tests_properties(runtime.exception_library PROPERTIES TIMEOUT 30)

add_test(NAME runtime.exception_partitions
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DSOURCES=${CMAKE_CURRENT_SOURCE_DIR}
        -DWORKDIR=${ADA_TEST_WORK_DIR}/exception-partitions
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckExceptionPartitions.cmake)
set_tests_properties(runtime.exception_partitions PROPERTIES TIMEOUT 60)

add_executable(text_io_library_test TextIoLibraryTest.c)
target_link_libraries(text_io_library_test PRIVATE adart)
add_test(NAME runtime.text_io_library COMMAND text_io_library_test)
set_tests_properties(runtime.text_io_library PROPERTIES TIMEOUT 15)

add_executable(calendar_runtime_test CalendarRuntimeTest.c)
target_link_libraries(calendar_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.calendar_library COMMAND calendar_runtime_test)
set_tests_properties(runtime.calendar_library PROPERTIES TIMEOUT 30)

# Ada 2005 package categorization and initialization contracts.
add_ada_test(categorization SOURCES elaboration/categorization.adb)
add_determinism_test(categorization SOURCE elaboration/categorization.adb)
foreach(category_error categorizationerrors preelaborableerrors preelaborableactualerrors categorizationplacementerrors)
    add_ada_error_test(${category_error} SOURCES ${category_error}.adb)
endforeach()
add_ada_error_test(preelaborablegenericerrors SOURCES elaboration/preelaborablegenericerrors.adb)
add_test(NAME semantics.categorization
    COMMAND ${CMAKE_COMMAND}
        -DADAC=$<TARGET_FILE:adac>
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckCategorization.cmake)
set_tests_properties(semantics.categorization PROPERTIES TIMEOUT 60)
add_test(NAME runtime.categorization_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=$<TARGET_FILE:qbe>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/elaboration/categorization.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/elaboration/categorization.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/categorization-modes
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
set_tests_properties(runtime.categorization_modes PROPERTIES TIMEOUT 60)

# Unicode source and the first wide-library foundation increment.
add_ada_test(widefoundation SOURCES strings/widefoundation.adb)
add_determinism_test(widefoundation SOURCE strings/widefoundation.adb)
add_ada_error_test(wideambiguityerrors SOURCES strings/wideambiguityerrors.adb)
add_test(NAME runtime.widefoundation_modes
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DADAC=$<TARGET_FILE:adac>
        -DQBE=$<TARGET_FILE:qbe>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/strings/widefoundation.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/strings/widefoundation.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/widefoundation-modes
        -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
add_executable(wide_runtime_test WideRuntimeTest.c)
target_link_libraries(wide_runtime_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.wide_characters COMMAND wide_runtime_test)
set_tests_properties(runtime.wide_characters PROPERTIES TIMEOUT 30)
add_ada_test(unicodeunitbundle SOURCES strings/unicodeunitbundle.adb)
add_determinism_test(unicodeunitbundle SOURCE strings/unicodeunitbundle.adb)
add_test(NAME semantics.unicode_source
    COMMAND ${CMAKE_COMMAND}
        -DADAC=$<TARGET_FILE:adac>
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckUnicodeSource.cmake)
set_tests_properties(semantics.unicode_source PROPERTIES TIMEOUT 30)

# Ada 2005 A.11 wide text: native arrays over the shared file runtime.
foreach(name wideiolibrary widewideiolibrary)
    add_ada_test(${name} SOURCES ${name}.adb)
    add_determinism_test(${name} SOURCE ${name}.adb)
    ada_test_fixture(source_path "${name}.adb")
    ada_test_fixture(expected_path "${name}.expected")
    add_test(NAME runtime.${name}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=$<TARGET_FILE:qbe>
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${name}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(runtime.${name}_modes PROPERTIES TIMEOUT 120)
endforeach()

add_executable(wide_text_io_test WideTextIoTest.c)
target_link_libraries(wide_text_io_test PRIVATE adart Threads::Threads)
add_test(NAME runtime.wide_text_io COMMAND wide_text_io_test)
set_tests_properties(runtime.wide_text_io PROPERTIES TIMEOUT 30)

# Ada 2005 A.4.7/A.4.8 native wide string families and A.11 adapters.
foreach(width wide widewide)
    foreach(family stringmaps fixedstrings fixedstringchecks boundedstrings
            unboundedstrings stringmappingcallbacks stringmappingtasking textstringio stringlimits)
        set(name ${width}${family})
        add_ada_test(${name} SOURCES ${name}.adb)
        add_determinism_test(${name} SOURCE ${name}.adb)
        ada_test_fixture(source_path "${name}.adb")
        ada_test_fixture(expected_path "${name}.expected")
        add_test(NAME runtime.${name}_modes
            COMMAND ${CMAKE_COMMAND}
                -DADA=$<TARGET_FILE:ada>
                -DADAC=$<TARGET_FILE:adac>
                -DQBE=$<TARGET_FILE:qbe>
                -DCC=${CMAKE_C_COMPILER}
                -DRUNTIME=$<TARGET_FILE:adart>
                -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
                -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
                -DSOURCE=${source_path}
                -DEXPECTED=${expected_path}
                -DWORKDIR=${ADA_TEST_WORK_DIR}/${name}-modes
                -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
        set_tests_properties(ada.${name} determinism.${name} runtime.${name}_modes
            PROPERTIES TIMEOUT 180)
    endforeach()
    add_ada_error_test(${width}stringmappingerrors SOURCES ${width}stringmappingerrors.adb)
    add_ada_error_test(${width}stringcategoryerrors SOURCES ${width}stringcategoryerrors.ads)
endforeach()

# Final wide-library audit: intrinsic profiles, native hashes and their callers.
foreach(name wideattributes wideoverloads widehashes widewidehashes stringhashes)
    add_ada_test(${name} SOURCES ${name}.adb)
    add_determinism_test(${name} SOURCE ${name}.adb)
    ada_test_fixture(source_path "${name}.adb")
    ada_test_fixture(expected_path "${name}.expected")
    add_test(NAME runtime.${name}_modes
        COMMAND ${CMAKE_COMMAND}
            -DADA=$<TARGET_FILE:ada>
            -DADAC=$<TARGET_FILE:adac>
            -DQBE=$<TARGET_FILE:qbe>
            -DCC=${CMAKE_C_COMPILER}
            -DRUNTIME=$<TARGET_FILE:adart>
            -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
            -DSTACK_PROBE=$<TARGET_FILE:windows_stack_probe_test>
            -DSOURCE=${source_path}
            -DEXPECTED=${expected_path}
            -DWORKDIR=${ADA_TEST_WORK_DIR}/${name}-modes
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTasking.cmake)
    set_tests_properties(ada.${name} determinism.${name} runtime.${name}_modes
        PROPERTIES TIMEOUT 180)
endforeach()
add_ada_error_test(wideprofileerrors SOURCES wideprofileerrors.adb)
add_ada_error_test(widehashcategoryerrors SOURCES widehashcategoryerrors.ads)
# A similarly named alias to another target does not get the predefined exception.
add_ada_error_test(widehashparenterrors SOURCES strings/hashparenterrors/ada-strings-wide_fixed-wide_hash.ads)

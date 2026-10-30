# Explicit selection and expectations: never silently discover new cases.
function(add_acats_test name)
    cmake_parse_arguments(ACATS "" "REJECTION;TEST_NAME" "SOURCES" ${ARGN})
    string(REPLACE ";" "|" sources "${ACATS_SOURCES}")
    add_test(NAME acats.${name}
        COMMAND ${CMAKE_COMMAND}
            -DNAME=${name}
            -DADA=$<TARGET_FILE:ada>
            -DROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/work
            -DREJECTION=${ACATS_REJECTION}
            -DSOURCE_LIST=${sources}
            -DEXPECTED_TEST_NAME=${ACATS_TEST_NAME}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunAcats.cmake
    )
    set_tests_properties(acats.${name} PROPERTIES TIMEOUT 60 LABELS "acats")
endfunction()

# Bit-packed three-bit Size clauses remain outside the byte-addressable subset.
add_acats_test(cd2a22a REJECTION "unsupported or unknown pragma 'PACK'")
add_acats_test(cd3014a)
add_acats_test(cd3014d)
add_acats_test(c354002)
add_acats_test(c35a08b)
add_acats_test(cxf2002)
add_acats_test(cxf2003)
add_acats_test(c52005a)
add_acats_test(c53007a)
add_acats_test(c49022a)
add_acats_test(c45523a)
add_acats_test(c45347a)
# Transfers from exception handlers to enclosing statement sequences.
add_acats_test(c59002a)

add_test(NAME acats.harness
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DROOT=${CMAKE_CURRENT_SOURCE_DIR}
        -DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/harness
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckHarness.cmake
)
set_tests_properties(acats.harness PROPERTIES TIMEOUT 90 LABELS "acats")

add_acats_test(c45504a)
add_acats_test(c45503a)
add_acats_test(c45611a)

# Collection cleanup across goto and the root compatibility generic renaming.
add_acats_test(c761002)

add_acats_test(c35507a)

add_acats_test(c36203a)

add_acats_test(c48005b)

# Dynamic others constraints shared with boxed aggregate construction.
add_acats_test(c433001)

# Unconstrained initialized constants and mutable defaulted variant records.
add_acats_test(c37206a)
add_acats_test(c37306a)

# Runtime discriminant constraint elaboration and default range checks.
add_acats_test(c37211a)
add_acats_test(c37108b)

# Controlled copy and anonymous variant object lifetime.
add_acats_test(c760007)
add_acats_test(c761012)

# Byte-addressable record placement and inherited storage attributes.
add_acats_test(cd4051a)
add_acats_test(cd1c03h)
add_acats_test(cd4031a REJECTION "bit-packed record components are not yet supported")

# Limited current instances/private extensions and array index Range prefixes.
add_acats_test(c3a0013)

# Anonymous access parameter conversion, forwarding, dereference and renaming.
add_acats_test(c460001)
add_acats_test(c460002)
add_acats_test(c3a2002)

# Null exclusions, including generic named/anonymous access objects.
add_acats_test(c3a0030)
add_acats_test(c3a0020)
add_acats_test(c3a0025)

# Callback structures and inheritance through private extensions.
add_acats_test(c3a0006)
add_acats_test(c3a0009)

# Implicit statement declarations and transfers out of nested compound statements.
add_acats_test(c51004a)
add_acats_test(c59002b)

# Package alias chains, use clauses and current instances.
add_acats_test(c85011a)

# Ada 2005 prefixed calls: values/access prefixes and access-parameter operations.
add_acats_test(c413001)
add_acats_test(c413002)
add_acats_test(c413003)
add_acats_test(c413004)

# Limited dependencies still include their packages in the executable partition.
add_acats_test(ca200031 TEST_NAME CA20003 SOURCES ca200030.ada ca200031.ada)
add_acats_test(ca110232 SOURCES ca110230.ada ca110231.ada ca110232.ada)
add_acats_test(ca120012 SOURCES ca120010.ada ca120011.ada ca120012.ada)
add_acats_test(ca12002)

# Contextual Access/Unchecked_Access overload resolution and controlling access.
add_acats_test(c3a2004)

# Separate subprogram and package bodies, including generic subunits.
add_acats_test(ca1006a)
add_acats_test(ca2009a)
add_acats_test(ca2009d)

# Ada 2005 extended returns, including limited generic results and transfers.
add_acats_test(c650003)

# The backfill snapshot records failures without calling them conformance
# passes. Only its independently selected, observed PASS cases join this label.
find_package(Python3 3.9 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    set(baseline_root "${CMAKE_CURRENT_SOURCE_DIR}/baseline")
    add_test(NAME baseline.acats_metadata
        COMMAND ${Python3_EXECUTABLE} -B "${baseline_root}/run.py" --validate)
    add_test(NAME baseline.acats_runner
        COMMAND ${Python3_EXECUTABLE} -B "${baseline_root}/test_runner.py")
    set_tests_properties(baseline.acats_metadata baseline.acats_runner
        PROPERTIES TIMEOUT 30 LABELS "baseline")

    # Raw legality observations only; these are not graded B-test passes.
    foreach(case_id b394a01 b394a02 b394a03 b394a04 b394a05 bc60001)
        add_test(NAME baseline.acats_${case_id}
            COMMAND ${Python3_EXECUTABLE} -B "${baseline_root}/run.py" --check
                --case ${case_id} --ada $<TARGET_FILE:ada> --adac $<TARGET_FILE:adac>
                --work "${CMAKE_CURRENT_BINARY_DIR}/baseline-probes")
        set_tests_properties(baseline.acats_${case_id}
            PROPERTIES TIMEOUT 60 LABELS "baseline")
    endforeach()

    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${baseline_root}/manifest.json" "${baseline_root}/expected.json")
    file(READ "${baseline_root}/manifest.json" baseline_selection)
    file(READ "${baseline_root}/expected.json" baseline_expected)
    string(JSON baseline_count LENGTH "${baseline_selection}" cases)
    math(EXPR baseline_last "${baseline_count} - 1")
    foreach(index RANGE ${baseline_last})
        string(JSON case_id GET "${baseline_selection}" cases ${index} id)
        string(JSON outcome GET "${baseline_expected}" cases ${case_id} outcome)
        if(outcome STREQUAL "PASS")
            add_test(NAME acats.backfill_${case_id}
                COMMAND ${Python3_EXECUTABLE} -B "${baseline_root}/run.py" --check
                    --case ${case_id} --ada $<TARGET_FILE:ada> --adac $<TARGET_FILE:adac>
                    --work "${CMAKE_CURRENT_BINARY_DIR}/baseline-passes")
            set_tests_properties(acats.backfill_${case_id}
                PROPERTIES TIMEOUT 60 LABELS "acats;backfill")
        endif()
    endforeach()
    add_custom_target(acats_baseline
        COMMAND ${Python3_EXECUTABLE} -B "${baseline_root}/run.py" --check
            --ada $<TARGET_FILE:ada> --adac $<TARGET_FILE:adac>
            --work "${CMAKE_CURRENT_BINARY_DIR}/baseline-observations"
        DEPENDS ada adac
        COMMENT "Checking raw ACATS baseline stability; recorded failures are not passes"
        USES_TERMINAL)
else()
    message(STATUS "Python 3.9+ unavailable: ACATS backfill probes are not registered")
endif()

# Phase 6g: formal private/access types, pointer containers and callbacks.
add_acats_test(cc54001)
add_acats_test(cc54002)
add_acats_test(cc54003)

# Generic tagged/access formals, accessibility and shared upstream foundation.
add_acats_test(c3a2a01 SOURCES f3a2a00.ada c3a2a01.ada)
add_acats_test(cc54004)

# Known private discriminants and the nonstatic formal Definite attribute.
add_acats_test(cc1204a)
add_acats_test(cc3240a)
add_acats_test(cc51b03 SOURCES fc51b00.ada cc51b03.ada)

# Ada 2005 abstract formal subprograms and dispatch through box defaults.
add_acats_test(cc60001)

# Attribute function defaults; grouped formal object syntax remains separate.
add_acats_test(cc1302a)
add_acats_test(cc3601a REJECTION "expected ':' in generic formal object")

# Formal package views, complete contracts, callbacks and formal-part use clauses.
add_acats_test(cc70002)
add_acats_test(cc70003)
add_acats_test(cc70a02 SOURCES fc70a00.ada cc70a02.ada)
add_acats_test(cc70b01 SOURCES fc70b00.ada cc70b01.ada)

# Nonreturning ordinary and generic procedures, including normal fall-through.
add_acats_test(c651001)

# Unchanged unbounded-string cases, including Maps.Constants.
add_acats_test(cxa4010)
add_acats_test(cxa4011)
add_acats_test(cxa4030)
add_acats_test(cxa4031)
add_acats_test(cxa4032)

# Unchanged Ada 2005 Amendment cases from official ACATS 3.1DD.
foreach(container_number 001 002 003 004 005 006)
    add_acats_test(cxai${container_number})
endforeach()
foreach(container_number 01 02 03 04 05 06)
    add_acats_test(cxaia${container_number}
        SOURCES fxaia00.ada cxaia${container_number}.ada)
endforeach()

# Phase 7 host environment and the base Calendar prerequisite.
if(UNIX OR MINGW)
    add_acats_test(cxah001)
    add_acats_test(cxah002)
    add_acats_test(cxag001)
endif()
add_acats_test(c96006a)
add_acats_test(c96007a)

# Stream array aliasing and elementary Stream_Size clauses.
add_acats_test(cdd1001)
add_acats_test(cdd2b01 SOURCES fdd2b00.ada cdd2b01.ada)

# Exception identity, saving/reraising, and null-occurrence contracts.
add_acats_test(cb41001)
add_acats_test(cb41003)
add_acats_test(cb41004)

# Text_IO input grammar and file/line operations.
add_acats_test(ce3704a)
add_acats_test(ce3804a)
add_acats_test(ce3904a)
add_acats_test(ce3402a)

# Calendar civil composition/decomposition and midnight normalization.
add_acats_test(c96008a)
add_acats_test(c96008b)

add_acats_test(ca21001)

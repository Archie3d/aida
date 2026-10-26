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

# Limited current instances/private extensions are supported; range syntax remains Phase 8.
add_acats_test(c3a0013 REJECTION "expected ')' after array index list")

# Anonymous access parameter conversion, forwarding, dereference and renaming.
add_acats_test(c460001)
add_acats_test(c460002)
add_acats_test(c3a2002)

# Null exclusions; generic access objects remain Phase 6g.
add_acats_test(c3a0030)
add_acats_test(c3a0020 REJECTION "access generic formal objects are not yet supported")
add_acats_test(c3a0025 REJECTION "access generic formal objects are not yet supported")

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

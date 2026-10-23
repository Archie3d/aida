# Explicit selection and expectations: never silently discover new cases.
function(add_acats_test name)
    cmake_parse_arguments(ACATS "" "REJECTION" "" ${ARGN})
    add_test(NAME acats.${name}
        COMMAND ${CMAKE_COMMAND}
            -DNAME=${name}
            -DADA=$<TARGET_FILE:ada>
            -DROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/work
            -DREJECTION=${ACATS_REJECTION}
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
# Phase 6e: valid Ada rejected because goto/labels are not implemented.
add_acats_test(c59002a REJECTION "goto statements are not yet supported")

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

# Collection/deallocation coverage retained unchanged; transfer cases await P6e.
add_acats_test(c761002 REJECTION "goto statements are not yet supported")

add_acats_test(c35507a)

add_acats_test(c36203a)

add_acats_test(c48005b)

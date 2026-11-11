add_test(NAME ada.importedfloatabi
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/subprograms/importedfloatabi.adb
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/ImportedFloatAbi.c
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/subprograms/importedfloatabi.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/importedfloatabi
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunImportedFloatAbi.cmake
)

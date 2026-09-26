set(IMPORTED_FLOAT_SEPARATE_LIBM OFF)
if(UNIX AND NOT APPLE)
    set(IMPORTED_FLOAT_SEPARATE_LIBM ON)
endif()
add_test(NAME ada.importedfloatabi
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSEPARATE_LIBM=${IMPORTED_FLOAT_SEPARATE_LIBM}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/importedfloatabi.adb
        -DHELPERS=${CMAKE_CURRENT_SOURCE_DIR}/ImportedFloatAbi.c
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/importedfloatabi.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/importedfloatabi
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunImportedFloatAbi.cmake
)

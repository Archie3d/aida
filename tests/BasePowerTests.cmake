# Exercise both driver linking and explicit assembly linking with libm.
add_test(NAME ada.basepower_driver
    COMMAND ${CMAKE_COMMAND}
        -DNAME=basepower_driver
        -DADA=$<TARGET_FILE:ada>
        -DSOURCES=${CMAKE_CURRENT_SOURCE_DIR}/ada/basepower.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/basepower.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunAdaTest.cmake
)
set(BASEPOWER_SEPARATE_LIBM OFF)
if(UNIX AND NOT APPLE)
    set(BASEPOWER_SEPARATE_LIBM ON)
endif()
add_test(NAME ada.basepower
    COMMAND ${CMAKE_COMMAND}
        -DADA=$<TARGET_FILE:ada>
        -DCC=${CMAKE_C_COMPILER}
        -DRUNTIME=$<TARGET_FILE:adart>
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/ada/basepower.adb
        -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/ada/basepower.expected
        -DWORKDIR=${ADA_TEST_WORK_DIR}/basepower
        -DSEPARATE_LIBM=${BASEPOWER_SEPARATE_LIBM}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunBasePower.cmake
)
add_ada_error_test(basepowererrors SOURCES basepowererrors.adb)

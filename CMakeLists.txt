cmake_minimum_required(VERSION 3.20)

project(ada_compiler)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(cmake/Sanitizers.cmake)
include(cmake/TestLimits.cmake)

# Where an installed compiler keeps the parts that are not programs.  The names
# are the ones GNAT uses: the environment is source to be read, and the run time
# is a library to be linked.
set(ADA_INSTALL_LIBRARY ${CMAKE_INSTALL_LIBDIR}/ada)
set(ADA_INSTALL_INCLUDE ${ADA_INSTALL_LIBRARY}/adainclude)
set(ADA_INSTALL_ARCHIVE ${ADA_INSTALL_LIBRARY}/adalib)

set(QBE_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/qbe)
set(QBE_EXECUTABLE ${QBE_BUILD_DIR}/qbe${CMAKE_EXECUTABLE_SUFFIX})

file(GLOB_RECURSE QBE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/qbe/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/qbe/*.h
)

find_program(QBE_MAKE_EXECUTABLE NAMES make gmake REQUIRED)

add_custom_command(
    OUTPUT "${QBE_BUILD_DIR}/qbe.stamp"
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory "${QBE_BUILD_DIR}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/qbe"
        "${QBE_BUILD_DIR}"
    COMMAND
        "${QBE_MAKE_EXECUTABLE}" -C "${QBE_BUILD_DIR}" qbe
    COMMAND
        "${CMAKE_COMMAND}" -E touch "${QBE_BUILD_DIR}/qbe.stamp"
    DEPENDS
        ${QBE_SOURCES}
        "${CMAKE_CURRENT_SOURCE_DIR}/qbe/Makefile"
    BYPRODUCTS
        "${QBE_EXECUTABLE}"
    COMMENT "Building qbe in the CMake build tree"
    VERBATIM
)

add_custom_target(qbe_build ALL
    DEPENDS "${QBE_BUILD_DIR}/qbe.stamp"
)

add_executable(qbe IMPORTED GLOBAL)
set_target_properties(qbe PROPERTIES
    IMPORTED_LOCATION "${QBE_EXECUTABLE}"
)
add_dependencies(qbe qbe_build)

set(AIDA_REAL_QBE ${QBE_EXECUTABLE})
if(AIDA_SANITIZER STREQUAL "thread")
    if(NOT UNIX OR NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Generated Ada TSan instrumentation requires Clang on a POSIX host")
    endif()
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    set(AIDA_TSAN_TOOLS "${CMAKE_CURRENT_SOURCE_DIR}/tools")
    set(AIDA_TSAN_BACKEND "${AIDA_REAL_QBE}")
    foreach(path AIDA_TSAN_TOOLS AIDA_TSAN_BACKEND)
        string(REPLACE "\\" "\\\\" ${path} "${${path}}")
        string(REPLACE "\"" "\\\"" ${path} "${${path}}")
    endforeach()
    file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/tools/tsan_ir.py" AIDA_TSAN_PASS_DIGEST)
    file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/tools/qbe_tsan.py" AIDA_TSAN_ADAPTER_DIGEST)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS tools/tsan_ir.py tools/qbe_tsan.py)
    set(QBE_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/qbe-tsan")
    configure_file(cmake/qbe-tsan.in "${QBE_EXECUTABLE}" @ONLY)
    file(CHMOD "${QBE_EXECUTABLE}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    set_target_properties(qbe PROPERTIES IMPORTED_LOCATION "${QBE_EXECUTABLE}")
endif()

add_subdirectory(runtime)
add_subdirectory(adac)
add_subdirectory(ada)

enable_testing()
add_subdirectory(tests)

# qbe is an imported target, being built by its own Makefile, so the program it
# leaves behind is installed as a file rather than as a target.
install(PROGRAMS "${AIDA_REAL_QBE}"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/runtime/ada/
    DESTINATION ${ADA_INSTALL_INCLUDE}
    FILES_MATCHING
        PATTERN "*.ads"
        PATTERN "*.adb"
)

set(TARGET adart)

# The C run time every compiled program is linked against.  It is a library
# rather than a pair of sources handed to the C compiler each time, so that an
# installed compiler carries it the way it carries anything else.
add_library(${TARGET} STATIC)

if(UNIX AND NOT APPLE)
    target_link_libraries(${TARGET} PUBLIC m)
endif()

target_sources(${TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/adart.c
        ${CMAKE_CURRENT_SOURCE_DIR}/adaio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/adafixed.c
        ${CMAKE_CURRENT_SOURCE_DIR}/adanumerics.c
)

target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS ${TARGET}
    ARCHIVE DESTINATION ${ADA_INSTALL_ARCHIVE}
)

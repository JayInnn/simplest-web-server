
MESSAGE("test compile, path: "  ${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_DIRECTORIES(
    ${ROOT_CMAKE_PATH}/third/googletest-release-1.12.1/googletest/include
    ${ROOT_CMAKE_PATH}/src/pool
    ${ROOT_CMAKE_PATH}/src/logger
)

LINK_DIRECTORIES(
    ${ROOT_CMAKE_PATH}/lib
)

FIND_PACKAGE(Threads)

FILE(GLOB_RECURSE POOL_TEST_SRC_LIST "unit_test/pool/*.cc")
FILE(GLOB_RECURSE LOGGER_TEST_SRC_LIST "unit_test/logger/*.cc")

ADD_EXECUTABLE(test_bin ${POOL_TEST_SRC_LIST} ${LOGGER_TEST_SRC_LIST})

TARGET_LINK_LIBRARIES(test_bin gtest gtest_main libsrc.a ${CMAKE_THREAD_LIBS_INIT})

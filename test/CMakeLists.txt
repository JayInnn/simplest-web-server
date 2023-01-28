
MESSAGE("test compile, path: "  ${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_DIRECTORIES(
    ${ROOT_CMAKE_PATH}/third/googletest-release-1.12.1/googletest/include
    ${ROOT_CMAKE_PATH}/src/pool
)

LINK_DIRECTORIES(
    ${ROOT_CMAKE_PATH}/lib
)

FIND_PACKAGE(Threads)

FILE(GLOB_RECURSE THREADPOOL_TEST_SRC_LIST "unit_test/threadpool/*.cc")

ADD_EXECUTABLE(test_bin ${THREADPOOL_TEST_SRC_LIST})

TARGET_LINK_LIBRARIES(test_bin gtest gtest_main libsrc.a ${CMAKE_THREAD_LIBS_INIT})

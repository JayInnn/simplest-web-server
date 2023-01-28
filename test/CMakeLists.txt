
message("pwd: "  ${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/googletest-release-1.12.1/googletest/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/pool
)

LINK_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
)

find_package(Threads)

FILE(GLOB_RECURSE THREADPOOL_TEST_SRC_LIST "unit_test/threadpool/*.cc")

ADD_EXECUTABLE(test_sult ${THREADPOOL_TEST_SRC_LIST})
TARGET_LINK_LIBRARIES(test_sult gtest gtest_main libsrc.a)
TARGET_LINK_LIBRARIES(test_sult ${CMAKE_THREAD_LIBS_INIT})

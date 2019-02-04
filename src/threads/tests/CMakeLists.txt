find_package(GTest)
find_package(Threads)

add_executable(${PROJECT_NAME}_threads_tests async_handle.cc)

target_link_libraries(${PROJECT_NAME}_threads_tests ${PROJECT_NAME}_threads ${GTEST_BOTH_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

gtest_discover_tests(${PROJECT_NAME}_threads_tests)

find_package(GTest)
find_package(Threads)

add_executable(${PROJECT_NAME}_base_tests 
  config.cc
  lru_cache_map.cc
  request.cc
  static_list.cc
  static_list_multi.cc
  static_list_multi.h
  static_list_multi_1.cc
  static_list_multi_2.cc
  statistics.cc
  timer.cc
  xml.cc)
target_link_libraries(${PROJECT_NAME}_base_tests ${PROJECT_NAME}_base ${GTEST_BOTH_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(${PROJECT_NAME}_base_tests SYSTEM PRIVATE ${GTEST_INCLUDE_DIR})

gtest_discover_tests(${PROJECT_NAME}_base_tests)

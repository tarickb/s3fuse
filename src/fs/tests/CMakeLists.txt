find_package(GTest)
find_package(Threads)

add_executable(${PROJECT_NAME}_fs_tests
  callback_xattr.cc
  mime_types.cc
  static_xattr.cc)

target_link_libraries(${PROJECT_NAME}_fs_tests ${PROJECT_NAME}_fs ${PROJECT_NAME}_base ${PROJECT_NAME}_crypto ${GTEST_BOTH_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(${PROJECT_NAME}_fs_tests SYSTEM PRIVATE ${GTEST_INCLUDE_DIR})

gtest_discover_tests(${PROJECT_NAME}_fs_tests)

add_executable(${PROJECT_NAME}_fs_tests_decrypt_file decrypt_file.cc)
target_link_libraries(${PROJECT_NAME}_fs_tests_decrypt_file ${PROJECT_NAME}_crypto)

add_executable(${PROJECT_NAME}_fs_tests_encrypt_file encrypt_file.cc)
target_link_libraries(${PROJECT_NAME}_fs_tests_encrypt_file ${PROJECT_NAME}_crypto)

add_executable(${PROJECT_NAME}_fs_tests_get_mime_type get_mime_type.cc)
target_link_libraries(${PROJECT_NAME}_fs_tests_get_mime_type ${PROJECT_NAME}_fs ${PROJECT_NAME}_base)

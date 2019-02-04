add_executable(${PROJECT_NAME}_fs_tests_callback_xattr callback_xattr.cc)
target_link_libraries(${PROJECT_NAME}_fs_tests_callback_xattr ${PROJECT_NAME}_fs ${PROJECT_NAME}_base ${PROJECT_NAME}_crypto)

add_executable(${PROJECT_NAME}_fs_tests_decrypt_file decrypt_file.cc)
target_link_libraries(${PROJECT_NAME}_fs_tests_decrypt_file ${PROJECT_NAME}_crypto)

add_executable(${PROJECT_NAME}_fs_tests_encrypt_file encrypt_file.cc)
target_link_libraries(${PROJECT_NAME}_fs_tests_encrypt_file ${PROJECT_NAME}_crypto)

add_executable(${PROJECT_NAME}_fs_tests_get_mime_type get_mime_type.cc)
target_link_libraries(${PROJECT_NAME}_fs_tests_get_mime_type ${PROJECT_NAME}_fs ${PROJECT_NAME}_base)

add_executable(${PROJECT_NAME}_fs_tests_static_xattr static_xattr.cc)
target_link_libraries(${PROJECT_NAME}_fs_tests_static_xattr ${PROJECT_NAME}_fs ${PROJECT_NAME}_crypto)

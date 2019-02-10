find_package(GTest)
find_package(Threads)

add_executable(${PROJECT_NAME}_crypto_passwords passwords.cc)
target_link_libraries(${PROJECT_NAME}_crypto_passwords ${PROJECT_NAME}_crypto)

add_executable(${PROJECT_NAME}_crypto_tests
  aes_cbc_256.cc
  aes_cbc_256_kat.cc
  aes_cbc_256_random.cc
  aes_ctr_256.cc
  aes_ctr_256_kat.cc
  aes_ctr_256_random_par.cc
  aes_ctr_256_random_seq.cc
  encoders.cc
  md5_kat.cc
  md5_random.cc
  pbkdf2_sha1_kat.cc
  random.h
  sha256_kat.cc
  symmetric_key.cc)
target_link_libraries(${PROJECT_NAME}_crypto_tests ${PROJECT_NAME}_crypto ${GTEST_BOTH_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(${PROJECT_NAME}_crypto_tests SYSTEM PRIVATE ${GTEST_INCLUDE_DIR})

gtest_discover_tests(${PROJECT_NAME}_crypto_tests)

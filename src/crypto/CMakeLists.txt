set(crypto_LIBS)

find_package(OpenSSL REQUIRED)
list(APPEND crypto_LIBS ${OPENSSL_CRYPTO_LIBRARY})

if(APPLE)
  find_library(CORE_LIB CoreFoundation)
  find_library(SECURITY_LIB Security)
  list(APPEND crypto_LIBS ${CORE_LIB} ${SECURITY_LIB})
endif()

set(crypto_SOURCES
  aes_cbc_256.cc
  aes_cbc_256.h
  aes_ctr_256.cc
  aes_ctr_256.h
  base64.cc
  base64.h
  buffer.h
  cipher.h
  encoder.h
  hash.h
  hash_list.h
  hex.cc
  hex.h
  hex_with_quotes.h
  hmac_sha1.cc
  hmac_sha1.h
  keychain.cc
  keychain.h
  md5.cc
  md5.h
  passwords.cc
  passwords.h
  pbkdf2_sha1.cc
  pbkdf2_sha1.h
  private_file.cc
  private_file.h
  sha256.cc
  sha256.h
  symmetric_key.h)

if(EnableTests)
  add_subdirectory(tests)
endif()

add_library(${PROJECT_NAME}_crypto ${crypto_SOURCES})
target_link_libraries(${PROJECT_NAME}_crypto ${crypto_LIBS})
add_dependencies(${PROJECT_NAME}_crypto ${PROJECT_NAME}_base)
target_include_directories(${PROJECT_NAME}_crypto SYSTEM PUBLIC ${OPENSSL_INCLUDE_DIR})

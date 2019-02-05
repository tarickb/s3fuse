find_package(CURL REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(OpenSSL)
find_package(GnuTLS)

add_definitions(
  -DSYSCONFDIR=\"${CMAKE_INSTALL_SYSCONFDIR}\")

if(GNUTLS_FOUND)
  add_definitions(-DHAVE_GNUTLS)
endif()

if(OPENSSL_FOUND)
  add_definitions(-DHAVE_OPENSSL)
endif()

set(base_COPY_FROM_CONFIG
  config.cc
  config.h)

set(base_SOURCES
  curl_easy_handle.cc
  curl_easy_handle.h
  logger.cc
  logger.h
  lru_cache_map.h
  paths.cc
  paths.h
  request.cc
  request.h
  request_hook.h
  static_list.h
  statistics.cc
  statistics.h
  timer.h
  xml.cc
  xml.h)

foreach(f IN LISTS base_COPY_FROM_CONFIG)
  set(of ${CMAKE_CURRENT_BINARY_DIR}/${f})
  set(if ${CMAKE_BINARY_DIR}/config/${f}.in)

  add_custom_command(
    OUTPUT ${of}
    COMMAND cp "${if}" "${of}"
    DEPENDS ${if}
    COMMENT "Copying ${if} to ${of}"
    VERBATIM)

  list(APPEND base_SOURCES ${of})
endforeach()

if(EnableTests)
  add_subdirectory(tests)
endif()

add_library(${PROJECT_NAME}_base ${base_SOURCES})
target_link_libraries(${PROJECT_NAME}_base ${CURL_LIBRARIES} ${LIBXML2_LIBRARIES} ${OPENSSL_LIBRARIES} ${GNUTLS_LIBRARIES})
target_include_directories(${PROJECT_NAME}_base SYSTEM PUBLIC ${LIBXML2_INCLUDE_DIR} ${CURL_INCLUDE_DIRS})

add_dependencies(${PROJECT_NAME}_base ${PROJECT_NAME}_config)

set(fs_SOURCES
  bucket_volume_key.cc
  bucket_volume_key.h
  cache.cc
  cache.h
  callback_xattr.cc
  callback_xattr.h
  directory.cc
  directory.h
  encrypted_file.cc
  encrypted_file.h
  encryption.cc
  encryption.h
  file.cc
  file.h
  glacier.cc
  glacier.h
  list_reader.cc
  list_reader.h
  metadata.cc
  metadata.h
  mime_types.cc
  mime_types.h
  object.cc
  object.h
  special.cc
  special.h
  static_xattr.cc
  static_xattr.h
  symlink.cc
  symlink.h
  xattr.h)

if(EnableTests)
  add_subdirectory(tests)
endif()

add_library(${PROJECT_NAME}_fs ${fs_SOURCES})
target_link_libraries(${PROJECT_NAME}_fs ${PROJECT_NAME}_crypto)

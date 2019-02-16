set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Werror")

find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED "fuse >= 2.7.3")
if(DEFINED FUSE_LIBRARY_DIRS AND NOT ${FUSE_LIBRARY_DIRS} EQUAL "")
  set(fuse_lib "-L${FUSE_LIBRARY_DIRS}" ${FUSE_LIBRARIES})
else()
  set(fuse_lib ${FUSE_LIBRARIES})
endif()

add_definitions(
  -DPACKAGE_NAME=\"${PROJECT_NAME}\"
  -DPACKAGE_VERSION_WITH_REV=\"${PROJECT_VERSION}${GIT_VERSION_SUFFIX}\"
  -D_FILE_OFFSET_BITS=64)

if(EnableMacOSBundle)
  add_definitions(-DOSX_BUNDLE)
endif()

include_directories(
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR}/src)

add_subdirectory(base)
add_subdirectory(crypto)
add_subdirectory(fs)
add_subdirectory(services)
add_subdirectory(threads)

# The order here matters. Libraries have to appear before their dependencies.
set(libs
  ${PROJECT_NAME}_fs
  ${PROJECT_NAME}_threads
  ${PROJECT_NAME}_services
  ${PROJECT_NAME}_crypto
  ${PROJECT_NAME}_base)

if(EnableTests)
  add_subdirectory(tests)
endif()

add_executable(${PROJECT_NAME} main.cc operations.cc operations.h)
target_link_libraries(${PROJECT_NAME} ${libs} ${fuse_lib})
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${FUSE_INCLUDE_DIRS})
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

if(EnableGcs)
  add_executable(${PROJECT_NAME}_gs_get_token gs_get_token.cc)
  target_link_libraries(${PROJECT_NAME}_gs_get_token ${libs})
  install(TARGETS ${PROJECT_NAME}_gs_get_token RUNTIME DESTINATION bin)

  add_executable(${PROJECT_NAME}_gs_operations gs_operations.cc)
  target_link_libraries(${PROJECT_NAME}_gs_operations ${libs})
  install(TARGETS ${PROJECT_NAME}_gs_operations RUNTIME DESTINATION bin)
endif()

add_executable(${PROJECT_NAME}_sha256_sum sha256_sum.cc)
target_link_libraries(${PROJECT_NAME}_sha256_sum ${PROJECT_NAME}_crypto)
install(TARGETS ${PROJECT_NAME}_sha256_sum RUNTIME DESTINATION bin)

add_executable(${PROJECT_NAME}_vol_key vol_key.cc)
target_link_libraries(${PROJECT_NAME}_vol_key ${libs})
install(TARGETS ${PROJECT_NAME}_vol_key RUNTIME DESTINATION bin)

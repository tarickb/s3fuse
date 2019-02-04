set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Werror")

find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED "fuse >= 2.7.3")

add_definitions(
  -DPACKAGE_NAME=\"${PROJECT_NAME}\"
  -DPACKAGE_VERSION_WITH_REV=\"${PROJECT_VERSION}${GIT_VERSION_SUFFIX}\"
  -D_FILE_OFFSET_BITS=64)

if(EnableMacOSBundle)
  add_definitions(-DOSX_BUNDLE)
endif()

include_directories(
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR}/src
  ${FUSE_INCLUDE_DIRS})

add_subdirectory(base)
add_subdirectory(crypto)
add_subdirectory(fs)
add_subdirectory(services)
add_subdirectory(threads)

set(service_libs)

if(EnableAwsS3)
  list(APPEND service_libs ${PROJECT_NAME}_services_aws)
endif()

if(EnableFvs)
  list(APPEND service_libs ${PROJECT_NAME}_services_fvs)
endif()

if(EnableGcs)
  list(APPEND service_libs ${PROJECT_NAME}_services_gs)
endif()

set(libs
  ${PROJECT_NAME}_fs
  ${service_libs}
  ${PROJECT_NAME}_threads
  ${PROJECT_NAME}_services
  ${PROJECT_NAME}_crypto
  ${PROJECT_NAME}_base)

if(EnableTests)
  add_subdirectory(tests)
endif()

add_library(${PROJECT_NAME}_init init.h init.cc)

add_executable(${PROJECT_NAME} main.cc operations.cc operations.h)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_init ${libs} ${FUSE_LIBRARIES})

add_executable(${PROJECT_NAME}_gs_get_token gs_get_token.cc)
target_link_libraries(${PROJECT_NAME}_gs_get_token ${libs})

add_executable(${PROJECT_NAME}_sha256_sum sha256_sum.cc)
target_link_libraries(${PROJECT_NAME}_sha256_sum ${libs})

add_executable(${PROJECT_NAME}_vol_key vol_key.cc)
target_link_libraries(${PROJECT_NAME}_vol_key ${PROJECT_NAME}_init ${libs})

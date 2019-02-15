set(services_SOURCES
  file_transfer.cc
  file_transfer.h
  impl.h
  service.cc
  service.h
  utils.h
  utils.cc)

set(service_libs)

if(EnableAwsS3)
  add_subdirectory(aws)
  list(APPEND service_libs ${PROJECT_NAME}_services_aws)
endif()

if(EnableFvs)
  add_subdirectory(fvs)
  list(APPEND service_libs ${PROJECT_NAME}_services_fvs)
endif()

if(EnableGcs)
  add_subdirectory(gs)
  list(APPEND service_libs ${PROJECT_NAME}_services_gs)
endif()

add_library(${PROJECT_NAME}_services ${services_SOURCES})
target_link_libraries(${PROJECT_NAME}_services ${service_libs} ${PROJECT_NAME}_crypto ${PROJECT_NAME}_base)

set(services_SOURCES
  file_transfer.cc
  file_transfer.h
  impl.cc
  impl.h
  service.cc
  service.h)

if(EnableAwsS3)
  add_subdirectory(aws)
endif()

if(EnableFvs)
  add_subdirectory(fvs)
endif()

if(EnableGcs)
  add_subdirectory(gs)
endif()

add_library(${PROJECT_NAME}_services ${services_SOURCES})

set(aws_SOURCES
  file_transfer.cc
  file_transfer.h
  impl.cc
  impl.h)

add_library(${PROJECT_NAME}_services_aws  ${aws_SOURCES})
target_link_libraries(${PROJECT_NAME}_services_aws ${PROJECT_NAME}_services ${PROJECT_NAME}_crypto)

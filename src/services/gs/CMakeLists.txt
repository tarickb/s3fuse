find_package(jsoncpp REQUIRED)

set(gs_SOURCES
  file_transfer.cc
  file_transfer.h
  impl.cc
  impl.h
  versioning.cc
  versioning.h)

add_library(${PROJECT_NAME}_services_gs ${gs_SOURCES})
target_include_directories(${PROJECT_NAME}_services_gs SYSTEM PRIVATE jsoncpp_lib)
target_link_libraries(${PROJECT_NAME}_services_gs ${PROJECT_NAME}_services ${PROJECT_NAME}_threads jsoncpp_lib)

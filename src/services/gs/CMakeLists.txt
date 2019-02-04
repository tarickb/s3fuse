set(gs_SOURCES
  file_transfer.cc
  file_transfer.h
  impl.cc
  impl.h)

add_library(${PROJECT_NAME}_services_gs ${gs_SOURCES})


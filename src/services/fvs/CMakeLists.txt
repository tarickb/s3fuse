set(fvs_SOURCES
  impl.cc
  impl.h)

add_library(${PROJECT_NAME}_services_fvs ${fvs_SOURCES})
target_link_libraries(${PROJECT_NAME}_services_fvs ${PROJECT_NAME}_services ${PROJECT_NAME}_crypto ${PROJECT_NAME}_base)


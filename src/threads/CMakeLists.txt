find_package(Threads)

set(threads_SOURCES
  async_handle.h
  parallel_work_queue.h
  pool.cc
  pool.h
  request_worker.cc
  request_worker.h
  work_item.cc
  work_item.h
  work_item_queue.h
  worker.cc
  worker.h)

if(EnableTests)
  add_subdirectory(tests)
endif()

add_library(${PROJECT_NAME}_threads ${threads_SOURCES})
target_link_libraries(${PROJECT_NAME}_threads ${CMAKE_THREAD_LIBS_INIT})
add_dependencies(${PROJECT_NAME}_threads ${PROJECT_NAME}_base)

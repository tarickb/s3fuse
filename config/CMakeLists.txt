enable_language(C)

get_directory_property(defines COMPILE_DEFINITIONS )
set(cpp_flags)
foreach(d ${defines})
  string(APPEND cpp_flags "-D${d} ")
endforeach()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config.h.in
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generator.config.sh config.inc generator.config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h.in
  DEPENDS config.inc generator.config.h.in generator.config.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building config.h.in"
  VERBATIM)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config.cc.in
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generator.config.sh config.inc generator.config.cc.in ${CMAKE_CURRENT_BINARY_DIR}/config.cc.in
  DEPENDS config.inc generator.config.cc.in generator.config.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building config.cc.in"
  VERBATIM)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/s3fuse.conf
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generator.s3fuse.conf.sh "${CMAKE_C_COMPILER}" "${cpp_flags}" config.inc "${PROJECT_NAME}" "${PROJECT_VERSION}" "${CMAKE_INSTALL_SYSCONFDIR}" generator.s3fuse.conf.awk ${CMAKE_CURRENT_BINARY_DIR}/s3fuse.conf
  DEPENDS config.inc generator.s3fuse.conf.awk generator.s3fuse.conf.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building s3fuse.conf"
  VERBATIM)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/s3fuse.conf.5
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generator.s3fuse.conf.sh "${CMAKE_C_COMPILER}" "${cpp_flags}" config.inc "${PROJECT_NAME}" "${PROJECT_VERSION}" "${CMAKE_INSTALL_SYSCONFDIR}" generator.s3fuse.conf.5.awk ${CMAKE_CURRENT_BINARY_DIR}/s3fuse.conf.5
  DEPENDS config.inc generator.s3fuse.conf.5.awk generator.s3fuse.conf.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building s3fuse.conf.5"
  VERBATIM)

add_custom_target(
  ${PROJECT_NAME}_config ALL DEPENDS 
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config.h.in
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config.cc.in
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/s3fuse.conf
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/s3fuse.conf.5)

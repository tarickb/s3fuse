enable_language(C)

get_directory_property(defines COMPILE_DEFINITIONS )
set(cpp_flags)
foreach(d ${defines})
  string(APPEND cpp_flags "-D${d} ")
endforeach()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config.h.in
  COMMAND ./generate_config_cc_h.sh config.inc config.h.template ${CMAKE_CURRENT_BINARY_DIR}/config.h.in
  DEPENDS config.inc config.h.template generate_config_cc_h.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building config.h.in"
  VERBATIM)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config.cc.in
  COMMAND ./generate_config_cc_h.sh config.inc config.cc.template ${CMAKE_CURRENT_BINARY_DIR}/config.cc.in
  DEPENDS config.inc config.cc.template generate_config_cc_h.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building config.cc.in"
  VERBATIM)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.conf
  COMMAND ./generate_config_file_man.sh "${CMAKE_C_COMPILER}" "${cpp_flags}" config.inc "${PROJECT_NAME}" "${PROJECT_VERSION}" "${CMAKE_INSTALL_SYSCONFDIR}" config_file.awk ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.conf
  DEPENDS config.inc config_file.awk generate_config_file_man.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building ${PROJECT_NAME}.conf"
  VERBATIM)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.conf.5
  COMMAND ./generate_config_file_man.sh "${CMAKE_C_COMPILER}" "${cpp_flags}" config.inc "${PROJECT_NAME}" "${PROJECT_VERSION}" "${CMAKE_INSTALL_SYSCONFDIR}" config_file_man.awk ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.conf.5
  DEPENDS config.inc config_file_man.awk generate_config_file_man.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building ${PROJECT_NAME}.conf.5"
  VERBATIM)

add_custom_target(
  ${PROJECT_NAME}_config ALL DEPENDS 
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config.h.in
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config.cc.in
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.conf
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.conf.5)

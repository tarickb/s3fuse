set(man_1_SED_SOURCES
  1
  gs_get_token.1
  gs_operations.1
  sha256_sum.1
  vol_key.1)

set(man_1_FILES)
set(man_5_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.conf.5)

set(man_FILES)

string(TIMESTAMP TODAY "%Y-%m-%d")
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)

set(SED_OPTS
  -e "s/__PACKAGE_NAME__/${PROJECT_NAME}/g"
  -e "s/__PACKAGE_NAME_UPPER__/${PROJECT_NAME_UPPER}/g"
  -e "s/__PACKAGE_VERSION__/${PROJECT_VERSION}/g"
  -e "s/__TODAY__/${TODAY}/g")

foreach(m IN LISTS man_1_SED_SOURCES)
  if(m MATCHES "\\.")
    set(mf ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${m})
  else()
    set(mf ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.${m})
  endif()
  set(ms ${CMAKE_CURRENT_SOURCE_DIR}/${m}.template)

  add_custom_command(
    OUTPUT ${mf}
    COMMAND sed ${SED_OPTS} ${ms} > ${mf}
    DEPENDS ${ms}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building sed-manpage ${mf}"
    VERBATIM)

  list(APPEND man_1_FILES ${mf})
endforeach()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.conf.5
  COMMAND cp "${CMAKE_BINARY_DIR}/config/${PROJECT_NAME}.conf.5" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.conf.5"
  DEPENDS ${CMAKE_BINARY_DIR}/config/${PROJECT_NAME}.conf.5
  COMMENT "Copying ${PROJECT_NAME}.conf.5"
  VERBATIM)

set(man_PDF_FILES)
list(APPEND man_FILES "${man_1_FILES}")
list(APPEND man_FILES "${man_5_FILES}")

foreach(f IN LISTS man_FILES)
  set(of ${f}.pdf)

  add_custom_command(
    OUTPUT "${of}"
    COMMAND groff -mandoc "${f}" -t -T pdf > "${of}"
    DEPENDS "${f}"
    COMMENT "Generating ${of}"
    VERBATIM)

  list(APPEND man_PDF_FILES "${of}")
endforeach()

add_custom_target(${PROJECT_NAME}_man ALL DEPENDS ${man_FILES})
add_dependencies(${PROJECT_NAME}_man ${PROJECT_NAME}_config)

add_custom_target(${PROJECT_NAME}_man_pdf DEPENDS ${man_PDF_FILES})
add_dependencies(${PROJECT_NAME}_man_pdf ${PROJECT_NAME}_man)

install(FILES ${man_1_FILES} DESTINATION share/man/man1)
install(FILES ${man_5_FILES} DESTINATION share/man/man5)

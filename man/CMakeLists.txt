set(man_SED_SOURCES
  s3fuse.1
  s3fuse_gs_get_token.1
  s3fuse_sha256_sum.1
  s3fuse_vol_key.1)

set(man_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/s3fuse.conf.5)

string(TIMESTAMP TODAY "%Y-%m-%d")
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)

set(SED_OPTS
  -e "s/__PACKAGE_NAME__/${PROJECT_NAME}/g"
  -e "s/__PACKAGE_NAME_UPPER__/${PROJECT_NAME_UPPER}/g"
  -e "s/__PACKAGE_VERSION__/${PROJECT_VERSION}/g"
  -e "s/__TODAY__/${TODAY}/g")

foreach(m IN LISTS man_SED_SOURCES)
  set(mf ${CMAKE_CURRENT_BINARY_DIR}/${m})
  set(ms ${CMAKE_CURRENT_SOURCE_DIR}/${m}.in)

  add_custom_command(
    OUTPUT ${mf}
    COMMAND sed ${SED_OPTS} ${ms} > ${mf}
    DEPENDS ${ms}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building sed-manpage ${mf}"
    VERBATIM)

  list(APPEND man_FILES ${mf})
endforeach()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/s3fuse.conf.5
  COMMAND cp "${CMAKE_BINARY_DIR}/config/s3fuse.conf.5" "${CMAKE_CURRENT_BINARY_DIR}/s3fuse.conf.5"
  DEPENDS ${CMAKE_BINARY_DIR}/config/s3fuse.conf.5
  COMMENT "Copying s3fuse.conf.5"
  VERBATIM)

set(man_PDF_FILES)

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

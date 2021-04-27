## Copyright 2021 Sandia Coporation
## Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
## the U.S. Government retains certain rights in this software.
##
## This source code is released under the New BSD License.
#

SET(ICET_GL3_SRCS
  gl3_draw.c
  gl3_image.c
  gl3_state.c
  )

SET(ICET_GL3_HEADERS
  ../include/IceTGL3.h
  ../include/IceTDevGL3Image.h
  )

ICET_ADD_LIBRARY(IceTGL3 ${ICET_GL3_SRCS} ${ICET_GL3_HEADERS})

SET_SOURCE_FILES_PROPERTIES(${ICET_GL3_HEADERS}
  PROPERTIES HEADER_FILE_ONLY TRUE
  )

TARGET_LINK_LIBRARIES(IceTGL3
  IceTCore
  ${ICET_OPENGL_LIBRARIES}
  )

IF(NOT ICET_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES ${ICET_SOURCE_DIR}/src/include/IceTGL3.h
    DESTINATION ${ICET_INSTALL_INCLUDE_DIR})
ENDIF(NOT ICET_INSTALL_NO_DEVELOPMENT)

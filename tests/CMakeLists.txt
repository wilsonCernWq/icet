## Copyright 2003 Sandia Coporation
## Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
## the U.S. Government retains certain rights in this software.
##
## This source code is released under the New BSD License.
#

IF (ICET_USE_OPENGL)
  SET(ICET_OPENGL_WINDOW_LIBRARY GLUT
    CACHE STRING
    "The name of the library the IceT tests will use to create windows for
OpenGL contexts. Supported values are GLUT and GLFW."
    )
  SET_PROPERTY(CACHE ICET_OPENGL_WINDOW_LIBRARY
    PROPERTY STRINGS GLUT GLFW
    )
  MARK_AS_ADVANCED(ICET_OPENGL_WINDOW_LIBRARY)
  IF (ICET_OPENGL_WINDOW_LIBRARY STREQUAL GLUT)
    FIND_PACKAGE(GLUT)
    IF (GLUT_FOUND)
      INCLUDE_DIRECTORIES(${GLUT_INCLUDE_DIR})
      SET(ICET_TESTS_USE_OPENGL 1)
      SET(ICET_TESTS_USE_GLUT 1)
    ELSE (GLUT_FOUND)
      MESSAGE(WARNING "Cannot find GLUT library. OpenGL tests disabled.")
    ENDIF (GLUT_FOUND)
  ELSEIF (ICET_OPENGL_WINDOW_LIBRARY STREQUAL GLFW)
    FIND_PACKAGE(GLFW)
    IF (GLFW_FOUND)
      INCLUDE_DIRECTORIES(${GLFW_INCLUDE_DIR})
      SET(ICET_TESTS_USE_OPENGL 1)
      SET(ICET_TESTS_USE_GLFW 1)
    ELSE (GLFW_FOUND)
      MESSAGE(WARNING "Cannot find GLFW library. OpenGL tests disabled.")
    ENDIF (GLFW_FOUND)
  ELSE ()
    MESSAGE(SEND_ERROR "Invalid value for ICET_OPENGL_WINDOW_LIBRARY: ${ICET_OPENGL_WINDOW_LIBRARY}")
  ENDIF ()
ENDIF (ICET_USE_OPENGL)

IF (NOT ICET_TESTS_USE_OPENGL)
  MESSAGE(STATUS "Most tests require ICET_USE_OPENGL and GLUT or GLFW. Those tests are disabled.")
ENDIF (NOT ICET_TESTS_USE_OPENGL)

SET(MyTests
  BackgroundCorrect.c
  CompressionSize.c
  Interlace.c
  MaxImageSplit.c
  OddImageSizes.c
  OddProcessCounts.c
  RadixkUnitTests.c
  RenderEmpty.c
  SimpleTiming.c
  SparseImageCopy.c
  )

IF (ICET_TESTS_USE_OPENGL)
  SET(MyTests ${MyTests}
    BlankTiles.c
    BoundsBehindViewer.c
    DisplayNoDraw.c
    RandomTransform.c
    SimpleExample.c
    )
ENDIF (ICET_TESTS_USE_OPENGL)

SET(UTIL_SRCS init.c ppm.c)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/test-config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/test-config.h
  )

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

CREATE_TEST_SOURCELIST(Tests icetTests_mpi.c ${MyTests}
  EXTRA_INCLUDE mpi_comm.h
  FUNCTION init_mpi_comm)

ADD_EXECUTABLE(icetTests_mpi ${Tests} ${UTIL_SRCS})
TARGET_LINK_LIBRARIES(icetTests_mpi
  IceTCore
  IceTMPI
  )
IF (ICET_TESTS_USE_OPENGL)
  TARGET_LINK_LIBRARIES(icetTests_mpi IceTGL)
  IF (ICET_TESTS_USE_GLUT)
    TARGET_LINK_LIBRARIES(icetTests_mpi ${GLUT_LIBRARIES})
  ENDIF (ICET_TESTS_USE_GLUT)
  IF (ICET_TESTS_USE_GLFW)
    TARGET_LINK_LIBRARIES(icetTests_mpi ${GLFW_LIBRARY})
  ENDIF (ICET_TESTS_USE_GLFW)
ENDIF (ICET_TESTS_USE_OPENGL)

IF (ICET_MPIRUN_EXE)
  SET(PRE_TEST_FLAGS ${ICET_MPIRUN_EXE} ${ICET_MPI_NUMPROC_FLAG} ${ICET_MPI_MAX_NUMPROCS} ${ICET_MPI_PREFLAGS})
  SET(POST_TEST_FLAGS ${ICET_MPI_POSTFLAGS})
ENDIF (ICET_MPIRUN_EXE)

SET(ICET_TEST_FLAGS "" CACHE STRING "Arguments given to the test program.  These are given between the test executable and the name of the test.  Run the test executable with the -h flag to see what is available.")
MARK_AS_ADVANCED(ICET_TEST_FLAGS)
SEPARATE_ARGUMENTS(ICET_TEST_FLAGS)

FOREACH (test ${MyTests})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  ADD_TEST(NAME IceT${TName}
    COMMAND
    ${PRE_TEST_FLAGS}
    $<TARGET_FILE:icetTests_mpi> ${ICET_TEST_FLAGS} ${TName}
    ${POST_TEST_FLAGS})
  IF (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 2.1)
    SET_TESTS_PROPERTIES(IceT${TName}
      PROPERTIES FAIL_REGULAR_EXPRESSION
      ":ERROR:;TEST NOT RUN;TEST NOT PASSED;TEST FAILED"
      )
    SET_TESTS_PROPERTIES(IceT${TName}
      PROPERTIES PASS_REGULAR_EXPRESSION "Test Passed"
      )
  ENDIF (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 2.1)
ENDFOREACH(test)

IF (WIN32)
  ADD_CUSTOM_TARGET(logclean erase log.???? *.ppm)
ELSE (WIN32)
  ADD_CUSTOM_TARGET(logclean rm -f log.???? *.ppm core.*)
ENDIF (WIN32)

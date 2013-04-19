# - Find 0mq
# Find the zeromq includes and library
#
#  LIBEVENT_INCLUDE_DIR - Where to find libevent include sub-directory.
#  LIBEVENT_LIBRARIES   - List of libraries when using libevent.
#  LIBEVENT_FOUND       - True if libevent found.

IF (LIBEVENT_INCLUDE_DIR)
  # Already in cache, be silent.
  SET(LIBEVENT_FIND_QUIETLY TRUE)
ENDIF (LIBEVENT_INCLUDE_DIR)

FIND_PATH(LIBEVENT_INCLUDE_DIR event2/event.h)

FIND_LIBRARY(LIBEVENT_LIBRARY NAMES event )

# Handle the QUIETLY and REQUIRED arguments and set LIBEVENT_FOUND to
# TRUE if all listed variables are TRUE.
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  LIBEVENT DEFAULT_MSG
  LIBEVENT_LIBRARY LIBEVENT_INCLUDE_DIR 
)

IF(LIBEVENT_FOUND)
  SET( LIBEVENT_LIBRARIES ${LIBEVENT_LIBRARY} )
ELSE(LIBEVENT_FOUND)
  SET( LIBEVENT_LIBRARIES )
ENDIF(LIBEVENT_FOUND)



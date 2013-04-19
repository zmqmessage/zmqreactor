project(ZMQREACTOR)

# projects library name
set(TARGET_NAME zmqreactor)

set(ZMQREACTOR_SOURCE_FILES
  Base.cpp
  Dynamic.cpp
  LibEvent.cpp
  )

# projects include directory
include_directories(
  ${ZMQREACTOR_INCLUDE_DIR}
  ${ZEROMQ_INCLUDE_DIR}
  ${LIBEVENT_INCLUDE_DIR}
  )

# link projects object files
# into an executable named
# ${TARGET_NAME}
add_library(${TARGET_NAME}
  SHARED
  ${ZMQREACTOR_SOURCE_FILES}
  )

# link ${TARGET_NAME} target with the
# following libraries
target_link_libraries(${TARGET_NAME}
  ${ZEROMQ_LIBRARIES}
  ${LIBEVENT_LIBRARIES}
  )

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)

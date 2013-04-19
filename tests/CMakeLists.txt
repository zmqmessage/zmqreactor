include_directories(
  ${ZMQMESSAGE_INCLUDE_DIR}
  ${LIBEVENT_INCLUDE_DIR}
  ${ZMQREACTOR_INCLUDE_DIR}
)

add_executable(ReactorsTest
  ReactorsTest.cpp
)

target_link_libraries(ReactorsTest
 pthread
 zmqreactor
)

add_test(ReactorsTest
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ReactorsTest)


add_executable(PushTest
  PushTest.cpp
)

target_link_libraries(PushTest
 pthread
 zmqreactor
)
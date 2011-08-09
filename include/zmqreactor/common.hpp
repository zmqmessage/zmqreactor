/**
 * @file common.hpp
 * @author askryabin
 *
 */

#ifndef ZMQREACTOR_COMMON_HPP_
#define ZMQREACTOR_COMMON_HPP_

namespace ZmqReactor
{
  /**
   * @brief Argument passed to event handlers from reactors.
   */
  struct Arg
  {
    zmq::socket_t* socket;
    int fd;
    short events;
  };

  /**
   * @brief Poll result.
   * Returns from reactor's 'run' functions and operator().
   */
  enum PollResult
  {
    ERROR, NONE_MATCHED, CANCELLED, OK
  };

  /**
   * @brief Get string name for specific poll result.
   */
  inline const char*
  poll_result_str(PollResult res)
  {
    static const char* POLL_RESULT_STRS[] =
      {"ERROR", "NONE_MATCHED", "CANCELLED", "OK"};
    return POLL_RESULT_STRS[res];
  }
}

#endif /* ZMQREACTOR_COMMON_HPP_ */

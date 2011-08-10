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
    /**
     * @brief Pointer to original ZMQ socket given when handlers have been configured
     *
     * Set to 0 if handler is set to file descriptor
     */
    zmq::socket_t* socket;
    /**
     * @brief Original file descriptor given when handlers have been configured
     *
     * Set to 0 if handler is set to ZMQ socket
     */
    int fd;
    /**
     * @brief Bitwise mask of triggered events:
     *
     * Possible constants: ZMQ_POLLIN, ZMQ_POLLOUT or ZMQ_POLLERR
     */
    short events;
  };

  /**
   * @brief Poll result.
   * Returns from reactor's 'run' functions and operator().
   */
  enum PollResult
  {
    /**
     * ZMQ poll error occured, no events triggered
     */
    ERROR,
    /**
     * No events are matched, no handlers called, timeout elapsed
     */
    NONE_MATCHED,
    /**
     * Processing has been canceled by some handlers (it has returned false)
     */
    CANCELLED,
    /**
     * Operation finished normally: events matched, handlers called
     */
    OK
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

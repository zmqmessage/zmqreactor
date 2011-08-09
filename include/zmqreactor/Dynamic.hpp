/**
 * @file Dynamic.hpp
 * @author askryabin
 *
 */

#ifndef ZMQREACTOR_DYNAMIC_HPP_
#define ZMQREACTOR_DYNAMIC_HPP_

#include "zmqreactor/details/Base.hpp"

#include <vector>
#include <tr1/functional>

namespace ZmqReactor
{
  /**
   * @brief Dynamic reactor. Allows dynamic adding of handlers.
   *
   * Should be created directly.
   * Overhead: stores polymorphic functions in vector of tr1 function objects.
   * They may use dynamic allocations for copies of large functors.
   * No virtual calls are performed.
   */
  class Dynamic : protected Private::ReactorBase
  {
  private:

    typedef std::tr1::function<bool (const ZmqReactor::Arg&)> HandlerFun;
    typedef std::vector<HandlerFun> HandlersVec;

    HandlersVec handlers_;

  public:

    /**
     * Add poll handler for zmq socket.
     * @tparam FunT functor with signature: bool (Arg);
     * Returns true to continue polling, false to break.
     * @param socket bound socket
     * @param events zmq events mask to handle, for example ZMQ_POLLIN
     * @param fun functor. Must be copyable.
     */
    template <typename FunT>
    void
    add_handler(zmq::socket_t& socket, short events, const FunT& fun)
    {
      add_socket(socket, events);
      handlers_.push_back(HandlerFun(fun));
    }

    /**
     * Add poll handler for some file descriptor.
     * Used for non-zmq pollable actions.
     * @tparam FunT functor with signature: bool (Arg);
     * Returns true to continue polling, false to break.
     * @param fd unix file descriptor
     * @param events zmq events mask to handle, for example ZMQ_POLLIN
     * @param fun functor. Must be copyable.
     */
    template <typename FunT>
    void
    add_handler(int fd, short events, const FunT& fun)
    {
      add_fd(fd, events);
      handlers_.push_back(HandlerFun(fun));
    }

    /**
     * Overload for events = ZMQ_POLLIN
     */
    template <typename FunT>
    inline void
    add_handler(zmq::socket_t& socket, const FunT& fun) {
      add_handler(socket, ZMQ_POLLIN, fun);
    }

    /**
     * Overload for events = ZMQ_POLLIN
     */
    template <typename FunT>
    inline void
    add_handler(int fd, const FunT& fun) {
      add_handler(fd, ZMQ_POLLIN, fun);
    }

    inline size_t
    num_handlers() const {
      return handlers_.size();
    }

    /**
     * Removes all handlers starting from idx.
     * i.e. if idx is 2:
     * handlers before: [0, 1, 2, 3]
     * handlers after: [0, 1]
     */
    void
    remove_handlers_from(int idx)
    {
      remove_from(idx);
      handlers_.resize(idx);
    }

    /**
     * Perform one poll operation.
     */
    PollResult
    operator()(long timeout = -1);

    /**
     * Perform poll operations
     * until some handler cancels processing (by returning false),
     * timeout expires or some zmq poll error occurs.
     */
    PollResult
    run(long timeout = -1);
  };
}

#endif /* ZMQREACTOR_DYNAMIC_HPP_ */

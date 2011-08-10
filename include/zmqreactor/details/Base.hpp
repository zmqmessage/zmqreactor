/**
 * @file Base.hpp
 * @author askryabin
 *
 */

#ifndef ZMQREACTOR_BASE_HPP_
#define ZMQREACTOR_BASE_HPP_

#include <vector>
#include <zmq.hpp>

#include <zmqreactor/common.hpp>
#include <zmqreactor/details/NonCopyable.hpp>

/**
 * @namespace ZmqReactor
 * @brief Main namespace: both \ref StaticReactorBase "Static"
 * and Dynamic reactors reside here.
 */
namespace ZmqReactor
{
  /**
   * @namespace Private
   * @brief Implementation details are located here, nothing to look at
   */
  namespace Private
  {
    /**
     * Base class for static and dynamic reactors.
     * Contains common implementation and data.
     */
    class ReactorBase : private NonCopyable
    {
    protected:
      ReactorBase() {}

      typedef std::vector<zmq::pollitem_t> PollItemsVec;

      PollItemsVec poll_items_;

      std::vector<zmq::socket_t*> sockets_;

      /**
       * Perform zmq poll once.
       * @return number of events matched.
       * -1 on poll error
       * 0 if timeout expired and no events matched.
       */
      int
      do_poll(long timeout);

      template <typename FunT>
      inline bool
      call_handler(FunT& fun, int item_num)
      {
        Arg arg = {
          sockets_[item_num],
          poll_items_[item_num].fd,
          poll_items_[item_num].revents
        };
        return fun(arg);
      }

      void
      add_socket(zmq::socket_t& socket, short events);

      void
      add_fd(int fd, short events);

      void
      remove_from(int idx);

      inline bool
      event_matches(PollItemsVec::const_reference item) const
      {
        return (item.revents & item.events);
      }

    };
  }
}

#endif /* ZMQREACTOR_BASE_HPP_ */
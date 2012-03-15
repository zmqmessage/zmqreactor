/**
 * @file Base.hpp
 * @author askryabin
 * Definition of base class for Static and Dynamic reactor: ReactorBase
 */

#ifndef ZMQREACTOR_BASE_HPP_
#define ZMQREACTOR_BASE_HPP_

#include <vector>
#include "sys/time.h"

#include <zmq.hpp>

#include <zmqreactor/common.hpp>
#include <zmqreactor/details/NonCopyable.hpp>

/**
 * @namespace ZmqReactor
 * @brief Main namespace: both \ref StaticReactorBase "Static"
 * and \ref Dynamic reactors reside here.
 */
namespace ZmqReactor
{
  /**
   * @namespace ZmqReactor::Private
   * @brief Implementation details are located here.
   */
  namespace Private
  {
    /**
     * Base class for static and dynamic reactors.
     * Contains common implementation and data.
     */
    class ReactorBase : private NonCopyable
    {
    private:
      const char* last_error_;

    public:

      /**
       * If poll operation finished with PollResult::ERROR status,
       * last error is saved and may be obtained.
       */
      inline
      const char*
      last_error() const
      {
        return last_error_;
      }

      /**
       * Replace old socket pointer to new value in all configured handlers.
       * Use it if you reopened a socket
       * (deleted old instance and created new)
       * and want all configured handlers to remain valid.
       * @return number of actual replacements made
       */
      size_t
      replace_socket(zmq::socket_t* old_ptr, zmq::socket_t* new_ptr);

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

      class Timer
      {
      private:
        long remaining_;
        struct timeval last_ev_;

      public:

        explicit
        Timer(long timeout) :
          remaining_(timeout)
        {
          if (remaining_ > 0)
          {
            ::gettimeofday(&last_ev_, 0);
          }
        }

        void
        tick()
        {
          if (remaining_ > 0)
          {
            struct timeval end;
            ::gettimeofday(&end, 0);
            struct timeval elapsed;
            timersub(&end, &last_ev_, &elapsed);
            remaining_ -= (elapsed.tv_sec * 1000000 + elapsed.tv_usec);
            if (remaining_ <= 0)
            {
              remaining_ = 0;
            }
            last_ev_ = end;
          }
        }

        long
        remaining() const
        {
          return remaining_;
        }
      };
    };
  }
}

#endif /* ZMQREACTOR_BASE_HPP_ */

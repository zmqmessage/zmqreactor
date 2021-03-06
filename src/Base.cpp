/**
 * @file ZmqWrap.cpp
 * @author askryabin
 *
 */

#include "zmqreactor/details/Base.hpp"
#include "zmqreactor/Static.hpp"

#include <time.h>
#include <sys/time.h>

namespace ZmqReactor
{
  namespace Private
  {
    /**
     * Get timeout (in microseconds) as a distance from the two timevals.
     * (0 if 'from' is after 'to')
     */
    static inline
    long timeout_usec(const struct timeval& from, const struct timeval& to)
    {
      struct timeval res;
      if (timercmp(&from, &to, > ))
      {
        return 0;
      }
      timersub(&to, &from, &res);

      return res.tv_sec * 1000000 + res.tv_usec;
    }

    size_t
    ReactorBase::replace_socket(
      zmq::socket_t* old_ptr, zmq::socket_t* new_ptr)
    {
      size_t rep = 0;
      void* old_content_ptr = static_cast<void*>(*old_ptr);

      for (size_t i = 0; i < sockets_.size(); ++i)
      {
        if (sockets_[i] == old_ptr && poll_items_[i].socket == old_content_ptr)
        {
          sockets_[i] = new_ptr;
          poll_items_[i].socket = static_cast<void*>(*new_ptr);
          ++rep;
        }
      }
      return rep;
    }

    int
    ReactorBase::do_poll(long timeout)
    {
      // 0MQ poll workaround for proper handling of zmq::poll timeout
      timeval start, stop;
      int res = -1;
      for(int nstep = 0; ; ++nstep)
      {
        if(timeout > 0)
        {
          ::gettimeofday(&start, 0);
        }

        try
        {
          res = zmq::poll(&poll_items_[0], poll_items_.size(), timeout);
        }
        catch (const zmq::error_t& e)
        {
          last_error_ = e.what();
          return -1;
        }

        if (res != 0 || timeout == -1 || (nstep == 0 && timeout == 0))
        {
          break;
        }

        ::gettimeofday(&stop, 0);
        long usecs_elapsed = timeout_usec(start, stop);

        if (usecs_elapsed >= timeout)
        {
          break;
        }

        timeout -= usecs_elapsed;
      }
      return res;
    }

    void
    ReactorBase::add_socket(zmq::socket_t& socket, short events)
    {
      zmq::pollitem_t item;
      item.socket = static_cast<void*>(socket);
      item.fd = 0;
      item.events = events;
      item.revents = 0;

      poll_items_.push_back(item);
      sockets_.push_back(&socket);
    }

    void
    ReactorBase::replace_socket(int idx, zmq::socket_t& socket, short events)
    {
      poll_items_[idx].socket = static_cast<void*>(socket);
      poll_items_[idx].fd = 0;
      poll_items_[idx].events = events;
      poll_items_[idx].revents = 0;
      sockets_[idx] = &socket;
    }

    int
    ReactorBase::index_of(zmq::socket_t& socket) const
    {
      SocketsVec::const_iterator it =
        std::find(sockets_.begin(), sockets_.end(), &socket);
      if (it == sockets_.end())
      {
        return -1;
      }
      return std::distance(sockets_.begin(), it);
    }

    void
    ReactorBase::remove_from(int idx)
    {
      poll_items_.resize(idx);
      sockets_.resize(idx);
    }

    void
    ReactorBase::add_fd(int fd, short events)
    {
      zmq::pollitem_t item;
      item.socket = 0;
      item.fd = fd;
      item.events = events;
      item.revents = 0;

      poll_items_.push_back(item);
      sockets_.push_back(0);
    }
  }
}//NS

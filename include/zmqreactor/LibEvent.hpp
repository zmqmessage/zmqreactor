/**
 * @file LibEvent.hpp
 * @author askryabin
 *
 */

#ifndef ZMQREACTOR_LIBEVENT_HPP_
#define ZMQREACTOR_LIBEVENT_HPP_

#include "zmqreactor/common.hpp"
#include "zmqreactor/LibEvent.fwd.hpp"
#include "zmqreactor/details/NonCopyable.hpp"
#include "zmqreactor/details/LinkedQueue.hpp"

#include <tr1/functional>

#include <event2/event.h>
#include <event2/event_struct.h>

namespace ZmqReactor
{
  struct LibEventBase::HandlerInfo :
    public Private::LinkedBase<LibEventBase::HandlerInfo>
  {
    typedef std::tr1::function<bool (ZmqReactor::Arg)> Fun;

    Arg arg_; //socket, fd, events
    LibEvent* reactor_;
    Fun fun_;

    short expected_events_; //in reactor terms

    struct event event_;
    bool enabled_;

    enum Status
    {
      WAITING, TRIGGERED
    };

    Status status_;

    template <typename Fun>
    inline
    HandlerInfo(LibEvent* reactor, const Fun& fun, short expected_events) :
      reactor_(reactor), fun_(fun),
      expected_events_(expected_events), enabled_(true), status_(WAITING)
    {}

    inline
    bool
    is_zmq() const
    {
      return arg_.socket != 0;
    }

    /**
     * Edge triggered
     */
    inline
    bool
    is_et() const
    {
      return is_zmq();
    }
  };

  /**
   * LibEvent-based reactor
   */
  class LibEvent : public LibEventBase, private Private::NonCopyable
  {
  private:
    event_base* base_;

    typedef Private::LinkedQueue<HandlerInfo> HandlerQueue;

    HandlerQueue waiting_handlers_;
    HandlerQueue triggered_handlers_;
    HandlerQueue disabled_handlers_;

    HandlerInfo* now_handled_;

    class AllHandlersIter;

    PollResult poll_result_; //modified from callbacks

    struct event event_immediate_;

    /**
     * Enum struct, never created
     */
    struct HasEvents
    {
      enum Value
      {
        YES, NO, UNKNOWN
      };

    private:
      HasEvents();
    };

  private:

    inline
    static
    short
    events_to_libev(short events, bool always_read = false, bool et = false);

    inline
    static
    short
    events_to_reactor(short events);

    inline
    static
    short
    zmq_to_reactor(uint32_t events);

    inline
    HandlerQueue&
    get_queue(HandlerInfo* hi);

    void
    update_immediate_timeout();

    void
    do_add_handler(HandlerInfo* hi, short libev_events);

    void
    do_remove_handler(HandlerInfo* hi);

    void
    do_activate(HandlerInfo* hi);

    void
    do_deactivate(HandlerInfo* hi);

    PollResult
    do_run(int mode, long timeout);

    HasEvents::Value
    has_actual_events(HandlerInfo* hi) const;

    HasEvents::Value
    handle_event(
      HandlerInfo* hi, HasEvents::Value has_ev, bool update_immediate);

    int
    fd_by_sock(zmq::socket_t& sock) const;

    size_t
    do_replace_descriptor(
      zmq::socket_t* old_ptr, int old_fd, zmq::socket_t* new_ptr, int new_fd);

  public:

    LibEvent();

    ~LibEvent();

    inline
    event_base*
    get_base()
    {
      return base_;
    }

    static
    void
    event_callback(int fd, short event, void *arg);

    static
    void
    immediate_callback(int fd, short event, void *arg);

    template <typename FunT>
    HandlerDesc
    add_handler(zmq::socket_t& socket, short events, const FunT& fun);

    /**
     * @overload
     * Overload for events = Poll:IN
     */
    template <typename FunT>
    inline
    HandlerDesc
    add_handler(zmq::socket_t& socket, const FunT& fun)
    {
      return add_handler(socket, Poll::IN, fun);
    }

    template <typename FunT>
    HandlerDesc
    add_handler(int fd, short events, const FunT& fun);

    /**
     * @overload
     * Overload for events = Poll::IN
     */
    template <typename FunT>
    inline
    HandlerDesc
    add_handler(int fd, const FunT& fun)
    {
      return add_handler(fd, Poll::IN, fun);
    }

    template <typename FunT>
    HandlerDesc
    add_timeout(const timeval& tv, const FunT& fun, bool persistent = false);

    template <typename FunT>
    HandlerDesc
    add_timeout(long sec, const FunT& fun, bool persistent = false);

    inline
    void
    remove_handler(HandlerDesc& hd)
    {
      do_remove_handler(hd.hi_);
      hd.hi_ = 0;
    }

    inline
    void
    disable_handler(HandlerDesc& hd)
    {
      if (hd.hi_ && hd.hi_->enabled_)
      {
        do_deactivate(hd.hi_);
        hd.hi_->enabled_ = false;
        disabled_handlers_.enqueue(hd.hi_);
      }
    }

    inline
    void
    enable_handler(HandlerDesc& hd)
    {
      if (hd.hi_ && !hd.hi_->enabled_)
      {
        hd.hi_->enabled_ = true;
        disabled_handlers_.dequeue(hd.hi_);
        do_activate(hd.hi_);
      }
    }

    inline
    bool
    enabled(HandlerDesc& hd)
    {
      return (hd.hi_ && hd.hi_->enabled_);
    }

    /**
     * Force check if actual events are pending for the handler and
     * if so, update its status to TRIGGERED and schedule immediate timeout.
     * Call it after some operation was performed on socket
     * outside its callback.
     */
    bool
    force_check_events(const HandlerDesc& hd);

    inline
    HandlerDesc
    now_handled() const
    {
      return HandlerDesc(now_handled_);
    }

    /**
     * All HandlerDesc handles are preserved
     */
    inline
    size_t
    replace_socket(zmq::socket_t* old_ptr, zmq::socket_t* new_ptr)
    {
      return do_replace_descriptor(old_ptr, 0, new_ptr, fd_by_sock(*new_ptr));
    }

    /**
     * @brief Perform one poll operation.
     *
     * @param timeout timeout in microseconds. No timeout by default
     */
    inline
    PollResult
    operator()(long timeout = -1)
    {
      return do_run(EVLOOP_ONCE, timeout);
    }

    inline
    PollResult
    run(long timeout = -1)
    {
      return do_run(0, timeout);
    }

    inline
    const char*
    last_error() const
    {
      return "libevent error";
    }
  };

  /////////////////////// implementation /////////////////////

  LibEvent::HandlerQueue&
  LibEvent::get_queue(HandlerInfo* hi)
  {
    switch (hi->status_)
    {
    case HandlerInfo::TRIGGERED:
      return triggered_handlers_;
    case HandlerInfo::WAITING:
    default:
      return waiting_handlers_;
    }
  }

  short
  LibEvent::events_to_libev(short events, bool always_read, bool et)
  {
    return
      ((events & Poll::IN) ? EV_READ : 0) |
      ((events & Poll::OUT) ? (always_read ? EV_READ : EV_WRITE) : 0) |
      EV_PERSIST |
      (et ? EV_ET : 0);
  }

  short
  LibEvent::events_to_reactor(short events)
  {
    return
      ((events & EV_READ) ? Poll::IN : 0) |
      ((events & EV_WRITE) ? Poll::OUT : 0);
  }

  short
  LibEvent::zmq_to_reactor(uint32_t events)
  {
    return
      ((events & ZMQ_POLLIN) ? Poll::IN : 0) |
      ((events & ZMQ_POLLOUT) ? Poll::OUT : 0) |
      ((events & ZMQ_POLLERR) ? Poll::ERR : 0);
  }

  template <typename FunT>
  LibEvent::HandlerDesc
  LibEvent::add_handler(zmq::socket_t& socket, short events, const FunT& fun)
  {
    const int fd = fd_by_sock(socket);

    HandlerInfo* hi = new HandlerInfo(this, fun, events);
    hi->arg_.fd = fd;
    hi->arg_.socket = &socket;

    do_add_handler(hi, events_to_libev(events, true, true));
    return HandlerDesc(hi);
  }

  template <typename FunT>
  LibEvent::HandlerDesc
  LibEvent::add_handler(int fd, short events, const FunT& fun)
  {
    HandlerInfo* hi = new HandlerInfo(this, fun, events);

    hi->arg_.fd = fd;
    hi->arg_.socket = 0;
    do_add_handler(hi, events_to_libev(events));
    return HandlerDesc(hi);
  }

  template <typename FunT>
  LibEvent::HandlerDesc
  LibEvent::add_timeout(
    const timeval& tv, const FunT& fun, bool persistent)
  {
    HandlerInfo* hi = new HandlerInfo(this, fun, 0);

    hi->arg_.fd = 0;
    hi->arg_.socket = 0;

    ::event_assign(
      &hi->event_, base_, 0, persistent ? EV_PERSIST : 0,
      &LibEvent::event_callback, hi);
    ::event_add(&hi->event_, &tv);

    waiting_handlers_.enqueue(hi);
    return HandlerDesc(hi);
  }

  template <typename FunT>
  LibEvent::HandlerDesc
  LibEvent::add_timeout(long sec, const FunT& fun, bool persistent)
  {
    timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = 0;
    return add_timeout(tv, fun, persistent);
  }
}

#endif /* ZMQREACTOR_LIBEVENT_HPP_ */

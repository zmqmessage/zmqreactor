/**
 * @file LibEvent.cpp
 * @author askryabin
 *
 */

#include "zmqreactor/LibEvent.hpp"

#include <iostream>

namespace ZmqReactor
{
  class LibEvent::AllHandlersIter
  {
  private:
    LibEvent* reactor_;
    HandlerQueue* cur_queue_;
    HandlerInfo* cur_hi_;

    HandlerQueue*
    first_queue();

    void
    next_queue();

    AllHandlersIter(LibEvent* reactor, bool end) :
      reactor_(reactor), cur_queue_(end ? 0 : first_queue()),
      cur_hi_(end ? 0 : cur_queue_->head())
    {}

  public:
    static
    AllHandlersIter
    begin(LibEvent* r)
    {
      return AllHandlersIter(r, false);
    }

    static
    AllHandlersIter
    end(LibEvent* r)
    {
      return AllHandlersIter(r, true);
    }

    HandlerInfo&
    operator* ()
    {
      return *cur_hi_;
    }

    HandlerInfo*
    operator-> ()
    {
      return cur_hi_;
    }

    HandlerInfo*
    get()
    {
      return cur_hi_;
    }

    AllHandlersIter&
    operator++ ();

    AllHandlersIter
    operator++ (int)
    {
      AllHandlersIter it = *this;
      ++(*this);
      return it;
    }

    friend
    bool
    operator== (AllHandlersIter& it1, AllHandlersIter& it2)
    {
      return it1.cur_hi_ == it2.cur_hi_;
    }

    friend
    bool
    operator!= (AllHandlersIter& it1, AllHandlersIter& it2)
    {
      return it1.cur_hi_ != it2.cur_hi_;
    }
  };

  LibEvent::HandlerQueue*
  LibEvent::AllHandlersIter::first_queue()
  {
    return &reactor_->waiting_handlers_;
  }

  void
  LibEvent::AllHandlersIter::next_queue()
  {
    if (cur_queue_ == first_queue())
    {
      cur_queue_ = &reactor_->triggered_handlers_;
    }
    else
    {
      cur_queue_ = 0; //end
    }
  }

  LibEvent::AllHandlersIter&
  LibEvent::AllHandlersIter::operator++ ()
  {
    if (!cur_queue_ || !cur_hi_)
    {
      return *this;
    }
    cur_hi_ = cur_queue_->next(cur_hi_);
    if (!cur_hi_)
    {
      next_queue();
      if (cur_queue_)
      {
        cur_hi_ = cur_queue_->head();
      }
    }
    return *this;
  }

  LibEvent::LibEvent() :
    base_(::event_base_new()),
    now_handled_(0),
    poll_result_(OK)
  {
    //EV_PERSIST does not work with 0 timeouts
    ::event_assign(
       &event_immediate_, base_, 0, EV_TIMEOUT,
       &LibEvent::immediate_callback, this);
  }

  LibEvent::~LibEvent()
  {
//std::cout << time(0) << ">" <<"====== ~Reactor ========= \n";
    for (AllHandlersIter it = AllHandlersIter::begin(this),
      e = AllHandlersIter::end(this); it != e; )
    {
      do_remove_handler((it++).get());
    }
    ::event_base_free(base_);
  }

  LibEvent::HasEvents::Value
  LibEvent::has_actual_events(HandlerInfo* hi) const
  {
    if (hi->is_zmq())
    {
      uint32_t actual_events;
      size_t sz = sizeof(actual_events);
      hi->arg_.socket->getsockopt(ZMQ_EVENTS, &actual_events, &sz);
      return ((hi->expected_events_ & zmq_to_reactor(actual_events)) != 0) ?
        HasEvents::YES : HasEvents::NO;
    }
    return HasEvents::UNKNOWN;
  }

  int
  LibEvent::fd_by_sock(zmq::socket_t& sock) const
  {
    int fd;
    size_t sz = sizeof(int);
    sock.getsockopt(ZMQ_FD, &fd, &sz);
    return fd;
  }

  void
  LibEvent::immediate_callback(int fd, short event, void *arg)
  {
    LibEvent* reactor = static_cast<LibEvent*>(arg);
//std::cout << time(0) << ">" <<time(0) << ": >>> Reactor: in immediate_callback, fd=" << fd << "\n";
    for (HandlerInfo* hi = reactor->triggered_handlers_.head(),
      * next_hi = hi; hi; hi = next_hi)
    {
      hi->arg_.events = hi->expected_events_;

      next_hi = reactor->triggered_handlers_.next(hi);

      reactor->handle_event(hi, HasEvents::YES, false);
    }
//std::cout << time(0) << ">" <<"<<< Reactor: in immediate_callback, handled\n";

    reactor->update_immediate_timeout();
  }

  LibEvent::HasEvents::Value
  LibEvent::handle_event(
    HandlerInfo* hi, HasEvents::Value has_ev, bool update_immediate)
  {
//std::cout << time(0) << ">" <<"Reactor: handle_event, has_ev=" << has_ev << ", update_immediate=" << update_immediate << "\n";
    //perform callback?
    now_handled_ = hi;

    if (has_ev != HasEvents::NO)
    {
      const bool should_continue = hi->fun_(hi->arg_);
      if (!should_continue)
      {
        ::event_base_loopbreak(base_);
        poll_result_ = CANCELLED;
      }
      else
      {
        poll_result_ = OK;
      }
      if (!now_handled_) //handler has just been removed in callback
      {
        //timeouts are up to date
        return HasEvents::NO;
      }
      has_ev = has_actual_events(hi); //again
    }

    if (has_ev == HasEvents::NO)
    {
      if (hi->status_ == HandlerInfo::TRIGGERED)
      {
//std::cout << time(0) << ">" <<"Reactor: handle_event: triggered to waiting\n";
        triggered_handlers_.dequeue(hi);
        hi->status_ = HandlerInfo::WAITING;
        waiting_handlers_.enqueue(hi);
        if (update_immediate)
        {
          update_immediate_timeout();
        }
      }
    }
    else if (has_ev == HasEvents::YES)
    {
      if (hi->status_ == HandlerInfo::WAITING)
      {
//std::cout << time(0) << ">" <<"Reactor: handle_event: waiting to triggered\n";
        waiting_handlers_.dequeue(hi);
        hi->status_ = HandlerInfo::TRIGGERED;
        triggered_handlers_.enqueue(hi);
        if (update_immediate)
        {
          update_immediate_timeout();
        }
      }
    }
//std::cout << time(0) << ">" <<"Reactor: handle_event return " << has_ev << "\n";
    now_handled_ = 0;
    return has_ev;
  }

  void
  LibEvent::event_callback(int fd, short event, void *arg)
  {
    HandlerInfo* hi = static_cast<HandlerInfo*>(arg);
    assert(fd == hi->arg_.fd);
//std::cout << time(0) << ">" <<time(0) << ": ----> Reactor: in event_callback, fd=" << fd << "\n";
    hi->arg_.events = events_to_reactor(event);

    hi->reactor_->handle_event(
      hi, hi->reactor_->has_actual_events(hi), true);
//std::cout << time(0) << ">" <<"----< Reactor: exit event_callback, fd=" << fd << "\n";
  }

  void
  LibEvent::update_immediate_timeout()
  {
//std::cout << time(0) << ">" <<">> Reactor: in update_immediate_timeout...\n";
    if (triggered_handlers_.head())
    {
//std::cout << time(0) << ">" <<"Reactor: scheduling timeout\n";

      timeval tv = {0, 0};
      ::event_add(&event_immediate_, &tv);
    }
//std::cout << time(0) << ">" <<"<< Reactor: after update_immediate_timeout...\n";
  }

  void
  LibEvent::do_add_handler(HandlerInfo* hi, short libev_events)
  {
    ::event_assign(
      &hi->event_, base_, hi->arg_.fd, libev_events,
      &LibEvent::event_callback, hi);
    ::event_add(&hi->event_, 0);

    HasEvents::Value has_ev = has_actual_events(hi);
//std::cout << time(0) << ">" <<"<< Reactor: do_add_handler, sock=" << hi->arg_.socket << ", fd=" << hi->arg_.fd << ", has_ev=" << has_ev << "\n";
    switch (has_ev)
    {
    case HasEvents::YES:
      hi->status_ = HandlerInfo::TRIGGERED;
      triggered_handlers_.enqueue(hi);
      update_immediate_timeout();
      break;
    default:
      hi->status_ = HandlerInfo::WAITING;
      waiting_handlers_.enqueue(hi);
      break;
    }
  }

  void
  LibEvent::do_remove_handler(HandlerInfo* hi)
  {
    if (hi)
    {
      ::event_del(&hi->event_);
      get_queue(hi).dequeue(hi);
      delete hi;
      if (now_handled_ == hi)
      {
        now_handled_ = 0;
      }
      update_immediate_timeout();
    }
  }

  bool
  LibEvent::force_check_events(const HandlerDesc& hd)
  {
    if (hd.hi_ && hd.hi_->status_ == HandlerInfo::WAITING &&
      has_actual_events(hd.hi_) == HasEvents::YES)
    {
      hd.hi_->status_ = HandlerInfo::TRIGGERED;
      waiting_handlers_.dequeue(hd.hi_);
      triggered_handlers_.enqueue(hd.hi_);
      update_immediate_timeout();
      return true;
    }
    return false;
  }

  size_t
  LibEvent::do_replace_descriptor(
    zmq::socket_t* old_ptr, int old_fd, zmq::socket_t* new_ptr, int new_fd)
  {
    size_t replaced = 0;

    for (AllHandlersIter it = AllHandlersIter::begin(this),
      e = AllHandlersIter::end(this); it != e; )
    {
      HandlerInfo* hi = (it++).get();

      if (
        (old_ptr && hi->arg_.socket == old_ptr) ||
        (old_fd && hi->arg_.fd == old_fd))
      {
        ::event_del(&hi->event_);
        hi->arg_.socket = new_ptr;
        hi->arg_.fd = new_fd;
        get_queue(hi).dequeue(hi);
        do_add_handler(hi, ::event_get_events(&hi->event_));
        ++replaced;
      }
    }
    return replaced;
  }

  PollResult
  LibEvent::do_run(int mode, long timeout)
  {
    if (timeout == 0)
    {
      ::event_base_loopexit(base_, NULL);
    }
    else if (timeout > 0)
    {
      timeval tv;
      tv.tv_sec = timeout;
      tv.tv_usec = 0;
      ::event_base_loopexit(base_, &tv);
    }

    poll_result_ = NONE_MATCHED;

    int ret = ::event_base_loop(base_, mode);
    if (ret == -1)
    {
      poll_result_ = ERROR;
    }
    if (ret == 1)
    {
      poll_result_ = NONE_MATCHED;
    }
    return poll_result_;
  }
}


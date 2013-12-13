/**
 * @file LibEvent.fwd.hpp
 * @author askryabin
 *
 */

#ifndef ZMQREACTOR_LIBEVENT_FWD_HPP_
#define ZMQREACTOR_LIBEVENT_FWD_HPP_

namespace ZmqReactor
{
  class LibEvent;

  class LibEventBase
  {
  protected:
    struct HandlerInfo;

  public:
    class HandlerDesc
    {
    private:
      HandlerInfo* hi_;
      friend class LibEvent;

      explicit
      HandlerDesc(HandlerInfo* hi) : hi_(hi) {}

    public:
      HandlerDesc() : hi_(0) {}

      inline
      bool
      empty() const
      {
        return !hi_;
      }
    };
  };
}

#endif /* ZMQREACTOR_LIBEVENT_FWD_HPP_ */

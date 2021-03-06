/**
 * @file Dynamic.cpp
 * @author askryabin
 *
 */

#include "zmqreactor/Dynamic.hpp"

namespace ZmqReactor
{
  PollResult
  Dynamic::operator()(long timeout)
  {
    int ret = do_poll(timeout);

    if (ret == -1)
    {
      return ERROR;
    }
    if (ret == 0)
    {
      return NONE_MATCHED;
    }

    PollItemsVec::iterator items_it = poll_items_.begin();
    PollItemsVec::iterator items_end = poll_items_.end();
    for (int n = 0; items_it != items_end; ++items_it, ++n)
    {
      if (event_matches(*items_it))
      {
        bool should_continue = call_handler(handlers_[n], n);
        if (!should_continue)
        {
          return CANCELLED;
        }
      }
    }
    return OK;
  }

  PollResult
  Dynamic::run(long timeout, int max_events)
  {
    PollResult res = NONE_MATCHED;
    Timer timer(timeout);
    for (int i = 0; i < max_events || max_events == -1; ++i)
    {
      res = this->operator()(timer.remaining());
      if (res != OK && res != NONE_MATCHED)
      {
        break;
      }
      timer.tick();
      if (timeout >=0 && timer.remaining() <= 0)
      {
        break;
      }
    }
    return res;
  }

}

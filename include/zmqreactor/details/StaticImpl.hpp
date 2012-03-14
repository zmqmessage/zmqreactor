/**
 * @file StaticImpl.hpp
 * @author askryabin
 *
 */

#ifndef ZMQREACTOR_STATICIMPL_HPP_
#define ZMQREACTOR_STATICIMPL_HPP_

////////////////////////////////////////////////////
///     definition of templates
////////////////////////////////////////////////////

namespace ZmqReactor
{
  namespace Private
  {
    template <typename FunTupleT, int Size>
    PollResult
    StaticReactor<FunTupleT, Size>::run(long timeout)
    {
      //check all socks
      PollResult res;
      Timer timer(timeout);
      while (true)
      {
        res = this->poll(timer.remaining());
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

    template <typename FunTupleT, int Size>
    template <typename ReactorT, int TermSize, int Num>
    PollResult
    StaticReactor<FunTupleT, Size>
    ::Caller<ReactorT, TermSize, Num>::call(self& r)
    {
      if (r.event_matches(r.poll_items_[Num]))
      {
        bool should_continue = r.call_handler(
            std::tr1::get<Num>(r.fun_tuple_), Num
        );
        if (!should_continue)
          return CANCELLED;
      }
      return Caller<ReactorT, TermSize, Num+1>::call(r);
    }

    template <typename FunTupleT, int Size>
    PollResult
    StaticReactor<FunTupleT, Size>::poll(long timeout)
    {
      int ret = do_poll(timeout);

      if (ret == -1) return ERROR;
      if (ret == 0) return NONE_MATCHED;

      return Caller<self, Size, 0>::call(*this);
    }
  }
}

#endif /* ZMQREACTOR_STATICIMPL_HPP_ */

/**
 * @file Static.hpp
 * @author askryabin
 * @brief Interface of Static reactor, and static reactors make functions
 */

#ifndef ZMQREACTOR_STATIC_HPP_
#define ZMQREACTOR_STATIC_HPP_

#include "zmqreactor/details/Base.hpp"

#include <tr1/tuple>
#include <memory>

namespace ZmqReactor
{
  class StaticReactorBase;

  namespace Private
  {
    /**
     * Proxy function, friend of StaticReactorBase
     */
    inline void
    add_socket(StaticReactorBase* r, zmq::socket_t& socket, short events);
  }


  /**
   * @brief Static reactor base class.
   *
   * Auto pointer to it is returned from \ref make_static functions.
   * It knows all handler types in compile time,
   * so it doesn't allow dynamic adding of handlers.
   * It has no overhead of virtual function calls.
   * Concrete static reactors must be created through \ref make_static functions.
   */
  class StaticReactorBase : protected Private::ReactorBase
  {
  public:
    /**
     * @brief Perform poll operations.
     *
     * Perform polls until either some handler cancels processing
     * (by returning false), timeout expires or some zmq poll error occurs.
     * @param timeout timeout in microseconds. No timeout by default
     */
    virtual PollResult
    run(long timeout = -1) = 0;

    /**
     * @brief Perform one poll operation.
     * @param timeout timeout in microseconds. No timeout by default
     */
    inline
    PollResult
    operator()(long timeout = -1)
    {
      return poll(timeout);
    }

    virtual ~StaticReactorBase() {}

  protected:
    virtual PollResult
    poll(long timeout = -1) = 0;

  private:
    friend void
    Private::add_socket(
        StaticReactorBase* r, zmq::socket_t& socket, short events
    );
  };

  namespace Private
  {
    inline void
    add_socket(
      StaticReactorBase* r, zmq::socket_t& socket, short events)
    {
      r->add_socket(socket, events);
    }

    /**
     * @brief Concrete static reactor parameterized with tuple of handlers.
     *
     * should be created indirectly, by calling \ref make_static functions
     */
    template <typename FunTupleT, int Size>
    class StaticReactor : public StaticReactorBase
    {
    private:
      typedef StaticReactor<FunTupleT, Size> self;

      FunTupleT fun_tuple_;

    public:

      StaticReactor(const FunTupleT& t) : fun_tuple_(t) {}

      virtual PollResult
      run(long timeout = -1);

    private:

      template <typename ReactorT, int TermSize, int Num>
      friend struct Caller;

      /**
       * @brief Functor Calls the handler at specified offset Num.
       * Non-terminal specialization.
       */
      template <typename ReactorT, int TermSize, int Num>
      struct Caller
      {
        inline static PollResult
        call(self& r);
      };

      /**
       * @brief Functor Calls the handler at specified offset Num.
       * Terminal specialization, does nothing.
       */
      template <typename ReactorT, int TermSize>
      struct Caller<ReactorT, TermSize, TermSize>
      {
        inline static PollResult
        call(self& r)
        {
          return OK;
        }
      };

    protected:

      virtual PollResult
      poll(long timeout = -1);
    };
  }

  /**
   * @brief Auto-pointer to static reactor.
   */
  typedef std::auto_ptr<StaticReactorBase> StaticPtr;

  ///static reactor makers

  /**
   * Create static reactor for given sockets, handlers and ZMQ events.
   * Handler functor must be of signature bool (Arg)
   * @return auto-pointer to dynamically allocated concrete reactor.
   */
  template <typename Fun1>
  inline
  StaticPtr
  make_static(
      zmq::socket_t& s1, Fun1 fun1, short events1 = ZMQ_POLLIN)
  {
    typedef typename std::tr1::tuple<Fun1> tuple_t;
    StaticPtr p(new Private::StaticReactor<tuple_t, 1>(tuple_t(fun1)));
    Private::add_socket(p.get(), s1, events1);
    return p;
  }

  /**
   * @overload
   */
  template <typename Fun1, typename Fun2>
  inline
  StaticPtr
  make_static(
      zmq::socket_t& s1, Fun1 fun1, short events1,
      zmq::socket_t& s2, Fun2 fun2, short events2)
  {
    typedef typename std::tr1::tuple<Fun1, Fun2> tuple_t;
    StaticPtr p(
        new Private::StaticReactor<tuple_t, 2>(tuple_t(fun1, fun2))
    );
    Private::add_socket(p.get(), s1, events1);
    Private::add_socket(p.get(), s2, events2);
    return p;
  }

  /**
   * @overload
   */
  template <typename Fun1, typename Fun2>
  inline
  StaticPtr
  make_static(
      zmq::socket_t& s1, Fun1 fun1,
      zmq::socket_t& s2, Fun2 fun2)
  {
    return make_static(
        s1, fun1, (short)ZMQ_POLLIN, s2, fun2, (short)ZMQ_POLLIN
    );
  }

  /**
   * @overload
   */
  template <typename Fun1, typename Fun2, typename Fun3>
  inline
  StaticPtr
  make_static(
      zmq::socket_t& s1, Fun1 fun1, short events1,
      zmq::socket_t& s2, Fun2 fun2, short events2,
      zmq::socket_t& s3, Fun3 fun3, short events3)
  {
    typedef typename std::tr1::tuple<Fun1, Fun2, Fun3> tuple_t;
    StaticPtr p(
        new Private::StaticReactor<tuple_t, 3>(tuple_t(fun1, fun2, fun3))
    );
    Private::add_socket(p.get(), s1, events1);
    Private::add_socket(p.get(), s2, events2);
    Private::add_socket(p.get(), s3, events3);
    return p;
  }

  /**
   * @overload
   */
  template <typename Fun1, typename Fun2, typename Fun3>
  inline
  StaticPtr
  make_static(
      zmq::socket_t& s1, Fun1 fun1,
      zmq::socket_t& s2, Fun2 fun2,
      zmq::socket_t& s3, Fun3 fun3)
  {
    return make_static(
        s1, fun1, (short)ZMQ_POLLIN,
        s2, fun2, (short)ZMQ_POLLIN,
        s3, fun3, (short)ZMQ_POLLIN
    );
  }

  /**
   * @overload
   */
  template <typename Fun1, typename Fun2, typename Fun3, typename Fun4>
  inline
  StaticPtr
  make_static(
      zmq::socket_t& s1, Fun1 fun1, short events1,
      zmq::socket_t& s2, Fun2 fun2, short events2,
      zmq::socket_t& s3, Fun3 fun3, short events3,
      zmq::socket_t& s4, Fun4 fun4, short events4)
  {
    typedef typename std::tr1::tuple<Fun1, Fun2, Fun3, Fun4> tuple_t;
    StaticPtr p(
        new Private::StaticReactor<tuple_t, 4>(tuple_t(fun1, fun2, fun3, fun4))
    );
    Private::add_socket(p.get(), s1, events1);
    Private::add_socket(p.get(), s2, events2);
    Private::add_socket(p.get(), s3, events3);
    Private::add_socket(p.get(), s4, events4);
    return p;
  }

  /**
   * @overload
   */
  template <typename Fun1, typename Fun2, typename Fun3, typename Fun4>
  inline
  StaticPtr
  make_static(
      zmq::socket_t& s1, Fun1 fun1,
      zmq::socket_t& s2, Fun2 fun2,
      zmq::socket_t& s3, Fun3 fun3,
      zmq::socket_t& s4, Fun4 fun4)
  {
    return make_static(
        s1, fun1, (short)ZMQ_POLLIN,
        s2, fun2, (short)ZMQ_POLLIN,
        s3, fun3, (short)ZMQ_POLLIN,
        s4, fun4, (short)ZMQ_POLLIN
    );
  }

  /**
   * @overload
   */
  template <
    typename Fun1, typename Fun2, typename Fun3, typename Fun4, typename Fun5>
  inline
  StaticPtr
  make_static(
      zmq::socket_t& s1, Fun1 fun1, short events1,
      zmq::socket_t& s2, Fun2 fun2, short events2,
      zmq::socket_t& s3, Fun3 fun3, short events3,
      zmq::socket_t& s4, Fun4 fun4, short events4,
      zmq::socket_t& s5, Fun5 fun5, short events5)
  {
    typedef typename std::tr1::tuple<Fun1, Fun2, Fun3, Fun4, Fun5> tuple_t;
    StaticPtr p(
        new Private::StaticReactor<tuple_t, 5>(
          tuple_t(fun1, fun2, fun3, fun4, fun5))
    );
    Private::add_socket(p.get(), s1, events1);
    Private::add_socket(p.get(), s2, events2);
    Private::add_socket(p.get(), s3, events3);
    Private::add_socket(p.get(), s4, events4);
    Private::add_socket(p.get(), s5, events5);
    return p;
  }

  /**
   * @overload
   */
  template <
    typename Fun1, typename Fun2, typename Fun3, typename Fun4, typename Fun5>
  inline
  StaticPtr
  make_static(
      zmq::socket_t& s1, Fun1 fun1,
      zmq::socket_t& s2, Fun2 fun2,
      zmq::socket_t& s3, Fun3 fun3,
      zmq::socket_t& s4, Fun4 fun4,
      zmq::socket_t& s5, Fun5 fun5)
  {
    return make_static(
        s1, fun1, (short)ZMQ_POLLIN,
        s2, fun2, (short)ZMQ_POLLIN,
        s3, fun3, (short)ZMQ_POLLIN,
        s4, fun4, (short)ZMQ_POLLIN,
        s5, fun5, (short)ZMQ_POLLIN
    );
  }

  //TODO copy make_static functions for more arguments if needed
}

#include "zmqreactor/details/StaticImpl.hpp"

#endif /* ZMQREACTOR_STATIC_HPP_ */

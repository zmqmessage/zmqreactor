/**
 * @file NumberOperator.cpp
 * @author askryabin
 * Performing different operations on numbers in a separate thread.
 */

#include <cctype>
#include <cstdlib>
#include <cstdio>

#include <unistd.h>

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <numeric>
#include <tr1/functional>

#include "pthread.h"

#include "zmqreactor/Dynamic.hpp"
#include "zmqreactor/Static.hpp"

/**
\example NumberOperator.cpp

This example launches a separate thread which receives multipart ZMQ message
with numbers. Separate ZMQ endpoint is used for each operation.
Reactor dispatches incoming messages to different instantiations of the same
template handler. This handler is parameterized with functor that actually
performs operation on numbers vector (sort, reverse sort, produce squares, sum
or factorial). All the code dedicated to receiving and sending
messages is located in the generic handler and is separated from actual
functor performing operation.

Main thread just sends request and receives result of operation.

Numbers are entered by user.

Operator thread may use either Static or Dynamic reactor.
Reactor type is specified by passing one of command line parameters:
\li -s for static, default
\li -d for dynamic

Example session:
\code
$ ./NumberOperator -d
Using dynamic reactor...
Enter numbers (separated with space): 8 4 6 11
Entered: 8, 4, 6, 11,
Sorted: 4, 6, 8, 11,
Reverse sorted: 11, 8, 6, 4,
Squares: 64, 16, 36, 121,
Sum: 29,
Factorial: 2112,
\endcode

*/

enum Mode
{
  SORT = 0, //sort numbers
  REVSORT, //reverse sort numbers
  SQUARE, //calculate square of each numbers
  SUM, //calculate sum of numbers
  FACTORIAL, //calculate factorial of numbers
  TOTAL  //total number of modes
};


const char* endpoints[] = {
  "inproc://sort_endpoint",
  "inproc://rev_sort_endpoint",
  "inproc://squares_endpoint",
  "inproc://sum_endpoint",
  "inproc://factorial_endpoint"
};

zmq::context_t context(1);

typedef std::vector<int> Numbers;

typedef std::vector<zmq::socket_t*> SocketVec;

typedef void* (*thread_fn) (void *);

// -------------- numbers transformers ----------------

bool sort(Numbers& numbers, bool reverse = false)
{
  std::sort(numbers.begin(), numbers.end());
  if (reverse)
  {
    std::reverse(numbers.begin(), numbers.end());
  }
  return true;
}

bool squares(Numbers& numbers)
{
  std::transform(
    numbers.begin(), numbers.end(),
    numbers.begin(), numbers.begin(),
    std::multiplies<int>()
  );
  return true;
}

/*
 * Op is a template of binary operation applied to numbers
 */
template<template<class> class Op>
struct accumulate
{
  typedef bool result_type;

  int operator()(Numbers& numbers, Numbers::value_type init)
  {
    int res = std::accumulate(
      numbers.begin(), numbers.end(), init, Op<Numbers::value_type>());
    numbers.clear();
    numbers.push_back(res);
    return true;
  }
};

// ----------------- printer ---------------------

void
print_numbers(const Numbers& numbers, const char* prefix)
{
  std::cout << prefix << ": ";
  std::ostream_iterator<int> out_it(std::cout, ", ");
  std::copy(numbers.begin(), numbers.end(), out_it);
  std::cout << std::endl;
}

// --------------- ZMQ helpers ----------------------

bool has_more(zmq::socket_t& sock)
{
  int64_t more = 0;
  size_t more_size = sizeof(more);
  sock.getsockopt(ZMQ_RCVMORE, &more, &more_size);
  return (more != 0);
}

void
zmqmessage_free(void *data, void *hint)
{
  ::free(data);
}

void send(zmq::socket_t& sock, const Numbers& numbers)
{
  zmq::message_t msg;
  size_t sz = sizeof(Numbers::value_type);

  for (size_t i = 0; i < numbers.size(); ++i)
  {
    void *data = ::malloc(sz);
    assert(data);
    ::memcpy(data, &numbers[i], sz);

    msg.rebuild(data, sz, &zmqmessage_free);
    int flag = (i < numbers.size() - 1) ? ZMQ_SNDMORE : 0;
    sock.send(msg, flag);
  }
}

void receive(zmq::socket_t& sock, Numbers& numbers)
{
  numbers.clear();

  zmq::message_t msg;
  do
  {
    sock.recv(&msg);
    numbers.push_back(*static_cast<Numbers::value_type*>(msg.data()));
  } while (has_more(sock));
}

template<class T>
void
del_obj(T* obj)
{
  delete obj;
}

// ------------- Generic request Handler ---------------

/*
 * Single handler receives message parts, composes Numbers vector,
 * performs operation and sends result.
 * @tparam Fun operation to perform on numbers. void(Numbers&)
 */
template <typename Fun>
struct Handler
{
  typedef bool result_type;

  Fun fun;

  explicit Handler(const Fun& f) : fun(f)
  {}

  bool
  operator()(ZmqReactor::Arg arg)
  {
    Numbers numbers;
    receive(*arg.socket, numbers);

//    print_numbers(numbers, "\tReceived");

    fun(numbers);

//    print_numbers(numbers, "\tTransformed");

    send(*arg.socket, numbers);

    return true;
  }
};

template <typename Fun>
inline
Handler<Fun>
mkhandler(const Fun& f)
{
  return Handler<Fun>(f);
}

//  ----------- Operator threads ------------

void
connect_socks(SocketVec& socks)
{
  socks.reserve(TOTAL);
  for (int mode = 0; mode < TOTAL; mode++)
  {
    socks[mode] = new zmq::socket_t(context, ZMQ_REP);
    socks[mode]->connect(endpoints[mode]);
  }
}

void*
static_operator_thread(void* arg)
{
  SocketVec socks;
  connect_socks(socks);

  ZmqReactor::StaticPtr r = ZmqReactor::make_static(
    *socks[SORT], mkhandler(&sort),
    *socks[REVSORT], mkhandler(
      std::tr1::bind(&sort, std::tr1::placeholders::_1, true)),
    *socks[SQUARE], mkhandler(&squares),
    *socks[SUM], mkhandler(
      std::tr1::bind(accumulate<std::plus>(), std::tr1::placeholders::_1, 0)),
    *socks[FACTORIAL], mkhandler(
      std::tr1::bind(accumulate<std::multiplies>(),
        std::tr1::placeholders::_1, 1))
  );

  r->run();

  std::for_each(socks.begin(), socks.end(), &del_obj<zmq::socket_t>);
  return 0;
}

void*
dynamic_operator_thread(void* arg)
{
  SocketVec socks;
  connect_socks(socks);

  ZmqReactor::Dynamic r;

  r.add_handler(
    *socks[SORT], mkhandler(&sort));
  r.add_handler(
    *socks[REVSORT], mkhandler(
      std::tr1::bind(&sort, std::tr1::placeholders::_1, true)));
  r.add_handler(
    *socks[SQUARE], mkhandler(&squares));
  r.add_handler(
    *socks[SUM], mkhandler(
      std::tr1::bind(accumulate<std::plus>(), std::tr1::placeholders::_1, 0)));
  r.add_handler(
    *socks[FACTORIAL], mkhandler(
      std::tr1::bind(accumulate<std::multiplies>(),
        std::tr1::placeholders::_1, 1)));

  r.run();

  std::for_each(socks.begin(), socks.end(), &del_obj<zmq::socket_t>);
  return 0;
}

// --------------- Main thread ----------------------

void
read_numbers(Numbers& numbers)
{
  std::cout << "Enter numbers (separated with space): ";

  std::string str;
  std::getline(std::cin, str);

  for (char * e, * p = &*str.begin(); ; p = e)
  {
    long v = ::strtol(p, &e, 10);
    if (e == p)
      break;
    numbers.push_back(v);
  }
}

void
request(zmq::socket_t& sock, const Numbers& in, Numbers& out)
{
  send(sock, in);
  receive(sock, out);
}

void
usage()
{
  std::cerr << "Usage: NumberOperator [-s|-d]" << std::endl;
  std::cerr << "\t-s Use static reactor, default" << std::endl;
  std::cerr << "\t-d Use dynamic reactor" << std::endl;
}

void
parse_args(int argc, char* argv[], bool &is_dynamic)
{
  char c;
  while ((c = getopt (argc, argv, "sd")) != -1)
  {
    switch (c)
    {
    case 's':
      is_dynamic = false;
      break;
    case 'd':
      is_dynamic = true;
      break;
    case '?':
      if (isprint (optopt))
        fprintf (stderr, "Unknown option `-%c'.\n", optopt);
      else
        fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
      usage();
    default:
      abort ();
    }
  }

  std::cout <<
    "Using " << (is_dynamic ? "dynamic" : "static") << " reactor..." <<
    std::endl;
}

int
main(int argc, char* argv[])
{
  bool is_dynamic;
  parse_args(argc, argv, is_dynamic);

  Numbers numbers, result;

  SocketVec socks;
  socks.reserve(TOTAL);
  for (int mode = 0; mode < TOTAL; mode++)
  {
    socks[mode] = new zmq::socket_t(context, ZMQ_REQ);
    socks[mode]->bind(endpoints[mode]);
  }

  thread_fn operator_fn = is_dynamic ?
    &dynamic_operator_thread : &static_operator_thread;

  pthread_t t_server;
  pthread_create(&t_server, NULL, operator_fn, NULL);

  read_numbers(numbers);
  print_numbers(numbers, "Entered");

  request(*socks[SORT], numbers, result);
  print_numbers(result, "Sorted");

  request(*socks[REVSORT], numbers, result);
  print_numbers(result, "Reverse sorted");

  request(*socks[SQUARE], numbers, result);
  print_numbers(result, "Squares");

  request(*socks[SUM], numbers, result);
  print_numbers(result, "Sum");

  request(*socks[FACTORIAL], numbers, result);
  print_numbers(result, "Factorial");

  std::for_each(socks.begin(), socks.end(), &del_obj<zmq::socket_t>);
  return 0;
}


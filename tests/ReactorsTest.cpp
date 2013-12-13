/**
 * @file ReactorsTest.cpp
 * @author askryabin
 *
 * \test
 * \brief
 * Performs poll for different types of reactors 9static, dynamic)
 * and for raw zmq poll api.
 *
 * Reactors dispatch requests to following handlers types:
 * \li bound member function.
 * \li bound member function with additional (big) parameter,
 * which probably wouldn/t fit into internal buffer of tr1::function object,
 * so dynamic memory allocation should occur for Dynamic reactor.
 * \li raw function pointer
 *
 * To measure this overhead we may parameterize test
 * with number of iterations and to see the consumed time.
 * Ex:
 * \code
 * $ ./ReactorsTest 100000
 * \endcode
 * Number of iterations means the number of "event sets" dispatched,
 * each reactor is created just once.
 */

#include "stdlib.h"
#include "unistd.h"
#include "assert.h"

#include <iostream>
#include <algorithm>
#include <iterator>
#include <tr1/functional>
#include <memory>
#include <sstream>

#include <zmq.hpp>
#include <pthread.h>

#include "zmqreactor/Dynamic.hpp"
#include "zmqreactor/Static.hpp"
#include "zmqreactor/LibEvent.hpp"

#ifdef NDEBUG
# undef NDEBUG
#endif

static const char* req_end = "end";
static const useconds_t usleep_interval = 0;//1000000;

static long attempts = 100000;
static long sockets_num = 50; //for each client!

static const char* dev_names[] = {
    "inproc://zmqreactor_test_proc1",
    "inproc://zmqreactor_test_proc2",
    "inproc://zmqreactor_test_proc3"
};

struct SomeParam
{
  int a,b,c,d;
};

static SomeParam some_param = {1,1,1,1};

zmq::context_t context(1);

std::string
endpoint(int client_num, int socket_num)
{
  std::ostringstream oss;
  oss << dev_names[client_num - 1] << "_" << socket_num;
  return oss.str();
}

void* client_fun(void* param)
{
  int num = *static_cast<int*>(param);

  zmq::socket_t* sockets[sockets_num];

  try {
    for (int i = 0; i < sockets_num; ++i)
    {
      sockets[i] = new zmq::socket_t(context, ZMQ_REQ);
      std::string ep = endpoint(num, i);
//    std::cout << "Client " << num << ": Connecting to endpoint " << ep << std::endl;
      sockets[i]->connect(ep.c_str());
    }

//    std::cout << "Client " << num << ": connected.. will do " << attempts << "attempts" << std::endl;

    //  Do requests, waiting each time for a response
    for (long request_nbr = 0; request_nbr < attempts; request_nbr++)
    {
      zmq::socket_t* socket = sockets[::rand() % sockets_num];

      zmq::message_t request (6);
      memcpy ((void *) request.data (), "Hello", 6);
//      std::cout << "Client " << num << ": Sending request " << request_nbr << std::endl;
      socket->send (request);

      //  Get the reply.
      zmq::message_t reply;
      socket->recv (&reply);
//      std::cout << "Client " << num << ": Received reply " << request_nbr <<
//        ": " << (char*)reply.data() << std::endl;

      if (usleep_interval > 0)
        ::usleep(usleep_interval);
    }
  }
  catch (std::exception &e) {
    std::cerr << "client_fun " << num <<
      ": An error occurred: " << e.what() << std::endl;
  }

  for (int i = 0; i < sockets_num; ++i)
  {
    delete sockets[i];
  }

  return 0;
}

void* client_term_fun(void* param)
{
  std::string ep = endpoint(1, 0);

  zmq::socket_t socket (context, ZMQ_REQ);

//  std::cout << "client_term_fun: Connecting to endpoint " << ep << std::endl;
  socket.connect (ep.c_str());

  zmq::message_t request (strlen(req_end)+1);
  memcpy ((void *) request.data (), req_end, strlen(req_end)+1);
//  std::cout << "Terminating: Sending request: " << req_end << std::endl;
  socket.send (request);

  return 0;
}

// server stuff

void my_free(void *data, void *hint)
{
  ::free(data);
}

bool do_handle(ZmqReactor::Arg arg, int& num_handled, const char* name, const char* resp_str)
{
  zmq::message_t query;
  arg.socket->recv(&query);

  const char* raw_req = (const char*)query.data();
  std::string req = (raw_req[query.size()-1] == '\0') ? std::string(raw_req) : std::string(raw_req, query.size());

  ++num_handled;
//  std::cout << name << ": " << req << " size = " << query.size() << ", total: " << num_handled << std::endl;

  if (req == req_end)
  {
//    std::cout << name << ": return false" << std::endl;
    return false;
  }

  char* resp = (char*)::malloc(strlen(resp_str)+1);
  strcpy(resp, resp_str);

  zmq::message_t t(resp, strlen(resp), &my_free); //no \0

  arg.socket->send(t);
  return true;
}

class SomeStatefulCls
{
public:

  SomeStatefulCls() : num_handled_1(0), num_handled_2(0) {}

  bool handle_1(ZmqReactor::Arg arg)
  {
    return do_handle(arg, num_handled_1, "handler 1", resp_1);
  }


  bool handle_2(ZmqReactor::Arg arg, SomeParam param)
  {
    return do_handle(arg, num_handled_2, "handler 2", resp_2);
  }

  int num_handled_1;
  int num_handled_2;

  static const char* resp_1;
  static const char* resp_2;
};

const char* SomeStatefulCls::resp_1 = "resp 1";
const char* SomeStatefulCls::resp_2 = "resp 2";

static int handled_free = 0;

bool free_handler(ZmqReactor::Arg arg)
{
  return do_handle(arg, handled_free, "free handler", "from free");
}

int run_raw(SomeStatefulCls& cls, zmq::socket_t* socks[], int num)
{
  zmq_pollitem_t items[num];
  for (int i = 0; i < num; ++i) {
    items[i].socket = static_cast<void*>(*socks[i]);
    items[i].fd = 0;
    items[i].events = ZMQ_POLLIN;
    items[i].revents = 0;
  }

  while (true)
  {
    int ret = zmq::poll(&items[0], num, -1);

    if (ret == -1) return -1;
    if (ret == 0) continue;

    for (int i = 0; i < num; ++i)
    {
      if (items[i].revents & items[i].events)
      {
        ZmqReactor::Arg arg = { socks[i], items[i].revents };

        bool call_res = false;
        if (i < sockets_num)
        {
          call_res = cls.handle_1(arg);
        }
        else if (i < 2*sockets_num)
        {
          call_res = cls.handle_2(arg, some_param);
        }
        else
        {
          call_res = free_handler(arg);
        }

        if (!call_res)
        {
          return 0;
        }
      }
    }
  }
  return 1;
}

enum ServerRunMode
{
  DYNAMIC = 0, STATIC, LIBEVENT, RAW
};

static const char* MODES[] = {"DYNAMIC", "STATIC", "LIBEVENT", "RAW"};

struct ServerRunResult
{
  int handled_1, handled_2, handled_3;
  double elapsed;
  const ServerRunMode mode;

  explicit
  ServerRunResult(ServerRunMode m) :
    handled_1(0), handled_2(0), handled_3(0), elapsed(0.), mode(m)
  {}
};

void* server_fun(void* param)
{
  ServerRunResult& result = *static_cast<ServerRunResult*>(param);

  zmq::socket_t* sockets_1[sockets_num];
  zmq::socket_t* sockets_2[sockets_num];
  zmq::socket_t* sockets_3[sockets_num];

  for (int i = 0; i < sockets_num; ++i)
  {
    sockets_1[i] = new zmq::socket_t(context, ZMQ_REP);
    sockets_1[i]->bind(endpoint(1, i).c_str());
    sockets_2[i] = new zmq::socket_t(context, ZMQ_REP);
    sockets_2[i]->bind(endpoint(2, i).c_str());
    sockets_3[i] = new zmq::socket_t(context, ZMQ_REP);
    sockets_3[i]->bind(endpoint(3, i).c_str());
  }
//  std::cout << "Sockets bound to: " << dev_names[0] << " "<< dev_names[1] <<" " << dev_names[2] << std::endl;

  try
  {
    SomeStatefulCls cls;
    handled_free = 0;

    clock_t start, finish;
    start = clock();

    //dynamic
    switch (result.mode)
    {
    case DYNAMIC:
    {
      ZmqReactor::Dynamic r;
      for (int i = 0; i < sockets_num; ++i)
      {
        r.add_handler(*sockets_1[i], ZMQ_POLLIN, std::bind1st(std::mem_fun(&SomeStatefulCls::handle_1), &cls));
        r.add_handler(*sockets_2[i], std::tr1::bind(
            std::tr1::mem_fn(&SomeStatefulCls::handle_2), &cls,
            std::tr1::placeholders::_1, some_param
        ));
        //free fun
        r.add_handler(*sockets_3[i], std::ptr_fun(&free_handler));
      }
      r.run();
      break;
    }
    case STATIC:
    {
      assert(sockets_num == 1);
      ZmqReactor::StaticPtr pR = ZmqReactor::make_static(
          *sockets_1[0], std::bind1st(std::mem_fun(&SomeStatefulCls::handle_1), &cls),
          *sockets_2[0], std::tr1::bind(
            std::tr1::mem_fn(&SomeStatefulCls::handle_2), &cls,
            std::tr1::placeholders::_1, some_param
          ),
          *sockets_3[0], std::ptr_fun(&free_handler)
      );
      pR->run();
      break;
    }
    case LIBEVENT:
    {
      ZmqReactor::LibEvent r;
      for (int i = 0; i < sockets_num; ++i)
      {
        r.add_handler(*sockets_1[i], ZmqReactor::Poll::IN, std::bind1st(std::mem_fun(&SomeStatefulCls::handle_1), &cls));
        r.add_handler(*sockets_2[i], std::tr1::bind(
            std::tr1::mem_fn(&SomeStatefulCls::handle_2), &cls,
            std::tr1::placeholders::_1, some_param
        ));
        //free fun
        r.add_handler(*sockets_3[i], std::ptr_fun(&free_handler));
      }
      ZmqReactor::PollResult res = r.run();
      std::cout << "LIBEVENT server: reactor returned " << ZmqReactor::poll_result_str(res) << "\n";
      break;
    }
    case RAW:
    {
      zmq::socket_t* socks[sockets_num*3];
      size_t arr_sz = sockets_num * sizeof(zmq::socket_t*);
      memcpy(socks, sockets_1, arr_sz);
      memcpy(socks + sockets_num, sockets_2, arr_sz);
      memcpy(socks + 2*sockets_num, sockets_3, arr_sz);
      run_raw(cls, socks, sockets_num*3);
      break;
    }
    }

    finish = clock();
    double elapsed = static_cast<double>(finish - start)/CLOCKS_PER_SEC;
    std::cout <<
        "Total with mode " << MODES[result.mode] << ": "
        << "handled 1: " << cls.num_handled_1 << ", "
        << "handled 2: " << cls.num_handled_2 << ", "
        << "handled free: " << handled_free
        << "; elapsed: " << elapsed << std::endl;

    result.handled_1 = cls.num_handled_1;
    result.handled_2 = cls.num_handled_2;
    result.handled_3 = handled_free;
    result.elapsed = elapsed;

  }
  catch (std::exception &e) {
    std::cerr << "An error occurred: " << e.what() << std::endl;
  }

  for (int i = 0; i < sockets_num; ++i)
  {
    delete sockets_1[i];
    delete sockets_2[i];
    delete sockets_3[i];
  }
  return 0;
}

void test(ServerRunMode mode)
{
  //sometimes immediate reconnecting fails
  ::sleep(1);

  //server will wait to receive until the clients start

  ServerRunResult result(mode);

  pthread_t t_server, t1, t2, t3, t_term;
  int n1 = 1, n2 = 2, n3 = 3;

  pthread_create(&t_server, NULL, &server_fun, &result);

  ::sleep(1);

  pthread_create(&t1, NULL, &client_fun, &n1);
  pthread_create(&t2, NULL, &client_fun, &n2);
  pthread_create(&t3, NULL, &client_fun, &n3);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(t3, NULL);

  //send term
  pthread_create(&t_term, NULL, &client_term_fun, 0);
  pthread_join(t_term, NULL);

  pthread_join(t_server, NULL);

  //std::cout << "result.handled_1 == attempts+1 --> " << result.handled_1 << " == " << (attempts+1) << std::endl;
  //check result
  assert(result.handled_1 == attempts+1); //for 'end' request
  assert(result.handled_2 == attempts);
  assert(result.handled_3 == attempts);
//  assert(result.elapsed > 0.);
}

int main (int argc, const char* argv[])
{
  if (argc > 1)
  {
    attempts = atoi(argv[1]);
    std::cout << "Attempts: " << attempts << std::endl;
  }

  test(RAW); //spare launch
//  test(ctx, VIRTUAL);

  test(DYNAMIC);
//  test(STATIC);
  test(LIBEVENT);
//  test(RAW);
}

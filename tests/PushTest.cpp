/**
 * @file PushTest.cpp
 * @author askryabin
 *
 */

#include <zmq.hpp>
#include <pthread.h>
#include <iostream>

#include "zmqreactor/LibEvent.hpp"

static const char* endopoint = "inproc://inner_ctrl";
static const char* msg = "terminate";

zmq::context_t context(1);

bool worker_handler(ZmqReactor::Arg arg)
{
  std::cout << "worker_handler: receive...\n";
  zmq::message_t query;
  arg.socket->recv(&query);
  std::cout << "worker_handler: received " << static_cast<const char*>(query.data()) << "\n";
  return false;
}

void* worker_fun(void* param)
{
  try
  {
    zmq::socket_t socket (context, ZMQ_PULL);
    socket.connect(endopoint);
    std::cout << "worker_fun: connected.. " << std::endl;
    ZmqReactor::LibEvent r;
    r.add_handler(socket, std::ptr_fun(&worker_handler));

    sleep(2);

    ZmqReactor::PollResult res = r.run();
    std::cout << "worker_fun: polled: " << ZmqReactor::poll_result_str(res) << std::endl;
  }
  catch (std::exception &e) {
    std::cerr << "worker_fun: An error occurred: " << e.what() << std::endl;
  }
  return 0;
}

void* main_fun(void* param)
{
  try
  {
    zmq::socket_t socket (context, ZMQ_PUSH);
    socket.bind(endopoint);
    std::cout << "main_fun: bound.. " << std::endl;

    sleep(2);


    zmq::message_t request(strlen(msg)+1);
    memcpy ((void *) request.data (), msg, strlen(msg)+1); //null term
    std::cout << "main_fun: Sending request: " << msg << std::endl;
    socket.send(request);
    std::cout << "main_fun: sent" << std::endl;

    sleep(2);
  }
  catch (std::exception &e) {
    std::cerr << "main_fun: An error occurred: " << e.what() << std::endl;
  }
  return 0;
}

int main()
{
  pthread_t t_main, t_worker;

  pthread_create(&t_main, NULL, &main_fun, 0);

  sleep(1);

  pthread_create(&t_worker, NULL, &worker_fun, 0);

  pthread_join(t_worker, NULL);
  pthread_join(t_main, NULL);

  return 0;

}

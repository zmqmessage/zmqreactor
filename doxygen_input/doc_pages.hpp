/** \mainpage ZmqReactor C++ Library
Provides C++ implementation of
<a class="el" href="http://en.wikipedia.org/wiki/Reactor_pattern">Reactor pattern</a>
for <a class="el" href="http://www.zeromq.org/">&Oslash;MQ</a> library.

<h3>Main features:</h3>
<ul>
<li>
  Supports two different kinds of reactors:
  \ref ZmqReactor::StaticReactorBase "static" and \ref ZmqReactor::Dynamic "dynamic".
  <ul>
  <li>Static reactor is a little bit faster, as it's handlers are bound to sockets positions
  at creation time, and no runtime overhead for dispatching occurs.
  But all the functions must be defined when creating the reactor.</li>
  <li>Dynamic reactor is more flexible,
  it allows adding and removing handlers of any type in runtime,
  but it imposes runtime overhead of creation a <b>tr1::function</b> wrapper and adding it to vector.
  tr1::function may impose some little overhead both in memory usage and calling speed
  </li>
  </ul>
</li>
<li>
  Supports handlers of any type (function pointers, functor objects)
</li>
<li>
  Supports handlers either for ZMQ sockets and for native file descriptors (currently Dynamic reactor only)
</li>
<li>
  May perform one poll operation or infinite poll loop.
</li>
<li>
  Handlers may request poll loop termination by return value (just return false).
</li>
<li>
  Polling with timeout
  (<a class="el" href="http://www.zeromq.org/topics:zmq-poll-workaround">ZMQ workaround</a> is implemented)
</li>
</ul>

<div class="zm_toc">
<h3>Table of contents</h3>
<ul>
  <li>\ref zm_tutorial "Tutorial"</li>
  <li>Reference</li>
    <dd>
    - \ref ZmqReactor::StaticReactorBase "Static" reactor
    - \ref ZmqReactor::Dynamic "Dynamic" reactor
    - \ref ZmqReactor "All ZmqReactor namespace members"
    </dd>
  </li>
  <li><a class="el" href="examples.html">Examples</a></li>
  <li><a class="el" href="test.html">Tests list</a></li>
  <li>\ref zm_build "Build instructions"</li>
  <li>\ref zm_performance "Performance notes"</li>
  <li><a class="el" href="https://github.com/zmqreactor/zmqreactor/">GitHub project download page</a></li>
</ul>
</div>
 */

/** \page zm_build
<h2>Build instructions</h2>

After you have downloaded an archive or cloned a git repository, you may
build the shared library, examples and tests.

<h3>Build Requirements</h3>
<ul>
<li>Library is built with <a href="http://cmake.org/">CMake</a> cross-platform build tool, so you need it installed.
<li>You need <a href="http://zeromq.org">ZeroMQ</a> library in order to compile ZmqReactor library.
</li>
</ul>

<h3>Runtime requirements</h3>
<ul>
<li>You need <a href="http://zeromq.org">ZeroMQ</a> library at runtime too.
</li>
</ul>

<h3>Build Steps</h3>
Go to zmqreactor directory and do following:
\code
$ mkdir build
$ cd build
//configure CMAKE: This command generates makefiles.
$ cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..
//or just "cmake ..". /path/to/install by default is /usr/local
//or type "ccmake .." for more control, or when zeromq library is installed in non-standard places

//then just make and install:
$ make
$ make install
//or "sudo make install" depending on permissions
\endcode

If you want to generate offline documentation:
\code
$ make doc
\endcode

Documentation is generated at doc/html/index.html

Note, that you need <a class="el" href="http://www.doxygen.org/">Doxygen</a>
to be installed in order to generate documentation.
 */

/** \page zm_performance
<h2>Performance notes</h2>
<hr>
Nothing comes for free, and our library introduces some little overhead
over plain zeromq poll interface.

To measure this overhead we may use \ref tests/ReactorsTest.cpp "ReactorsTest"
with huge number of iterations and see the consumed time for each type of reactor.

We have made 10 launches with 1000000 iterations each and got the following summary (in seconds)

<table cellspacing=0 cellpadding=2>
<tr>
  <th>Launch</th><th>Dynamic</th><th>Static</th><th>Raw poll</th>
</tr>

<tr><th>1</th><td>38.54</td><td>30.52</td><td>26.38</td></tr>
<tr><th>2</th><td>38.64</td><td>38.64</td><td>27.59</td></tr>
<tr><th>3</th><td>31.33</td><td>37.24</td><td>32.45</td></tr>
<tr><th>4</th><td>35.74</td><td>29.54</td><td>29.03</td></tr>
<tr><th>5</th><td>34.78</td><td>36.19</td><td>35.81</td></tr>
<tr><th>6</th><td>32.09</td><td>33.63</td><td>33.38</td></tr>
<tr><th>7</th><td>38.55</td><td>39.93</td><td>39.88</td></tr>
<tr><th>8</th><td>35.65</td><td>41.00</td><td>40.98</td></tr>
<tr><th>9</th><td>40.45</td><td>37.75</td><td>33.65</td></tr>
<tr><th>10</th><td>39.11</td><td>38.14</td><td>31.84</td></tr>

<tr style='color: blue;'><th>average</th>
              <td>36.49</td><td>36.26</td><td>33.10</td></tr>
<tr style='color: red;'><th>min</th>
              <td>31.33</td><td>30.52</td><td>26.38</td></tr>

</table>

First of all, we need to say that this "application" does nothing
but sending and receiving small messages and dispatching poll events, so it should be considered rather synthetic,
and in real apps performance costs probably will be even more unnoticeable.

 */


/** \page zm_tutorial

<h2>Tutorial</h2>
<div class="zm_toc">
<ul>
  <li>\ref ref_def_handlers "Defining handlers"</li>
  <li>\ref ref_make_static "Creating static reactor"</li>
  <li>\ref ref_make_dynamic "Creating dynamic reactor"</li>
  <li>\ref ref_timeout "Poll with timeout"</li>
</ul>
</div>

<hr>

\anchor ref_def_handlers
<h3>Defining handlers</h3>
Your handlers may be anything that is callable on ZmqReactor::Arg and returns bool.
Returning true means, that polling will be continued.
Returning false means, that polling will be stopped and ZmqReactor::CANCELLED status is Returned
(see ZmqReactor::PollResult)
\code

bool handler(ZmqReactor::Arg arg)
{
  if (we_want_to_stop())
    return false;
  return true;
}

struct Handler
{
  bool operator() (ZmqReactor::Arg arg)
  {
    ...
  }
}

\endcode
Of course you may use any function adapter
(i.e. created with std::tr1::bind, boost::bind, etc.).

\anchor ref_make_static
<h3>Creating static reactor</h3>

Static reactor is created using ZmqReactor::make_static functions.
They return auto pointer to static reactor base class (ZmqReactor::StaticReactorBase).

\code

#include <zmqreactor/Static.hpp>

//called when writing is possible
bool send_data(ZmqMessage::Arg arg)
{
  zmq::socket_t* s = arg.socket;
  zmq::message_t msg;
  s->send(msg);
  return true;
}

bool my_handler(ZmqMessage::Arg arg, bool param)
{
  zmq::socket_t* s = arg.socket;
  zmq::message_t msg;
  s->recv(msg);
  //...
  return true;
}

int main()
{
  zmq::socket_t sock1, sock2;

  ZmqReactor::StaticPtr r = ZmqReactor::make_static(
    sock1, &send_data, ZMQ_POLLOUT,
    sock2, std::tr1::bind(&my_handler, std::tr1::placeholders::_1, true)), ZMQ_POLLIN
  );
  r->run();
  return 0;
}
\endcode

You can give up to 5 pairs of socket and handler.
If this limit is not enough you may create new make_static functions as
defined in \ref Static.hpp

\anchor ref_make_dynamic
<h3>Creating dynamic reactor</h3>

Dynamic reactor is created directly.
Dynamic reactor allows adding handlers for native file descriptors.

\code

#include <zmqreactor/Dynamic.hpp>

bool my_handler(ZmqMessage::Arg arg, bool param)
{
  zmq::socket_t* s = arg.socket;
  zmq::message_t msg;
  s->recv(msg);
  //...
  return true;
}

bool write_file(ZmqMessage::Arg arg)
{
  int fd = arg.fd;
  //... write to file ...
  return true;
}

int main()
{
  zmq::socket sock1;

  int fd = ::open("file.txt", O_WRONLY | O_CREAT | O_TRUNC);

  ZmqReactor::Dynamic r;

  r.add_handler(sock1, std::tr1::bind(&my_handler, std::tr1::placeholders::_1, true));
  r.add_handler(fd, &write_file, ZMQ_POLLOUT);

  r.run(); //polling until any handler returns false

  //we may remove handlers where they are not needed anymore

  r.remove_handlers_from(1); //&write_file is removed

  r.run(); //polling again...

  return 0;
}
\endcode

\anchor ref_timeout
<h3>Polling with timeout</h3>

For each reactor there are two ways to poll
<ul>
<li> <i>PollResult operator() (long timeout = -1)</i><br>
Perform exactly one poll operation, call all matched handlers, return.
</li>
<li> <i>PollResult run(long timeout = -1)</i><br>
Perform polls until either some handler cancels processing
(by returning false), timeout expires or some zmq poll error occurs.
</li>
</ul>

\code

void print_res(ZmqReactor::PollResult res, const char* last_error)
{
  switch (res)
  {
  case ZmqReactor::CANCELLED:
    printf("cancelled by some handler");
    break;
  case ZmqReactor::NONE_MATCHED:
    printf("timeout expired");
    break;
  case ZmqReactor::OK:
    printf("Polled, handlers called. (for one operation)");
    break;
  case ZmqReactor::ERROR:
    printf("Poll error occurred: %s", last_error);
    break;
  }
}

int main()
{
  ZmqReactor::Dynamic dr;
  r.add_handler(...);

  ZmqReactor::StaticPtr sr = ZmqReactor::make_static(...);

  ZmqReactor::PollResult res;

  //one poll operation
  res = dr(1000000); //1 second
  print_res(res, dr.last_error());

  res = (*sr)(1000000); //1 second
  print_res(res, sr->last_error());

  //many operations, until timeout expires or handler cancels further processing
  res = dr.run(1000000); //1 second
  print_res(res, dr.last_error());

  res = sr->run(1000000); //1 second
  print_res(res, sr->last_error());

  return 0;
}
\endcode


*/


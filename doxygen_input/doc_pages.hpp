/** \mainpage ZmqReactor C++ Library
Provides implementation of
<a class="el" href="http://en.wikipedia.org/wiki/Reactor_pattern">Reactor pattern</a> for ZMQ library.

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
  Support handler of any type (function pointers, functor objects)
</li>
<li>
  Polling with timeout
  (<a class="el" href="http://www.zeromq.org/topics:zmq-poll-workaround">ZMQ workaround</a> implemented)
</li>
</ul>

REMOVE ME:

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

That's all.

 */

/** \page zm_performance
<h2>Performance notes</h2>
<hr>
Nothing comes for free, and our library introduces some little overhead
over plain zeromq poll interface.

To measure this overhead we may use \ref tests/ReactorsTest.cpp "ReactorsTest"
with huge number of iterations and see the consumed time for each type of reactor.

We have made 10 launches with 1000000 iterations each and got the following summary

<table>
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
  <li>\ref ref_configuring "Configuring library to integarte smoothly into your application"</li>
  <li>\ref ref_linking_options "Linking options"</li>
  <li>\ref ref_receiving "Receiving messages"</li>
  <li>\ref ref_sending "Sending messages"</li>
</ul>
</div>

<hr>

\anchor ref_configuring
<h3>Configuring library</h3>
To integrate the library into your application you can (and encouraged to)
define a few macro constants before including library headers
(ZmqMessage.hpp and ZmqTools.hpp) anywhere in your application.
ZmqMessageFwd.hpp may be included wherever (i.e. before the definitions).
Though none of these definitions are mandatory.

These constants are:

<ul>

<li>
\code
ZMQMESSAGE_STRING_CLASS
\endcode
@copydoc ZMQMESSAGE_STRING_CLASS
</li>

<li>
\code
ZMQMESSAGE_LOG_STREAM
\endcode
@copydoc ZMQMESSAGE_LOG_STREAM
</li>

<li>
\code
ZMQMESSAGE_LOG_TERM
\endcode
@copydoc ZMQMESSAGE_LOG_TERM
</li>

<li>
\code
ZMQMESSAGE_EXCEPTION_MACRO
\endcode
@copydoc ZMQMESSAGE_EXCEPTION_MACRO
</li>

<li>
\code
ZMQMESSAGE_WRAP_ZMQ_ERROR
\endcode
@copydoc ZMQMESSAGE_WRAP_ZMQ_ERROR
</li>

</ul>

\anchor ref_linking_options
<h3>Linking options</h3>

<ol>
<li>
You may build ZmqMessage as shared library (see \ref zm_build "build instructions")
and link against it. But if you need extensive configuration, the following problem may arise:
library is built separately, with definite and basically default configuration
(\ref ZMQMESSAGE_LOG_STREAM, \ref ZMQMESSAGE_EXCEPTION_MACRO, \ref ZMQMESSAGE_WRAP_ZMQ_ERROR),
So if you need to override them in your application,
you really need to \ref zm_build "rebuild shared library" with appropriate definitions
(the same as you define in your application), otherwise you can get the link error as following:
\code
In function `__static_initialization_and_destruction_0':
/home/andrey_skryabin/projects/zmqmessage/include/zmqmessage/TypeCheck.hpp:32: undefined reference to `ZmqMessage::Private::TypeCheck<ZmqMessage::MessageFormatError, ZmqMessage::NoSuchPartError, zmq::error_t>::value'
collect2: ld returned 1 exit status
\endcode
That's because we check that generated types are the same.
Link with shared library if you don't need extensive configuration or if it's not too cumbersome
for you to rebuild shared library with the same configuration.
</li>
<li>
Do not link against shared library. In this case you MUST include implementation
code in ONE of your .cpp files in order to assemble you binary:
\code

//@file foo.cpp

//make definitions available for linking
#include "ZmqMessageImpl.hpp"

...

\endcode
Do not include "ZmqMessageImpl.hpp" on more than one .cpp file,
or you will get multiple definitions and thus link error.

Such approach is better than header-only, cause it reduces compilation time.

</li></ol>

\anchor ref_receiving
<h3>Receiving messages</h3>
To receive multipart message, you create instance of ZmqMessage::Incoming.
For XREQ and XREP socket types use \ref ZmqMessage::XRouting as template parameter,
for other socket types use \ref ZmqMessage::SimpleRouting.

\code
#include "ZmqMessage.hpp"

zmq::context_t ctx(1);
zmq::socket sock(ctx, ZMQ_PULL); //will use SimpleRouting
sock.connect("inproc://some-endpoint");

ZmqMessage::Incoming<ZmqMessage::SimpleRouting> incoming(sock);

//say, we know what message parts we receive. Here are their names:
const char* req_parts[] = {"id", "name", "blob"};

//true because it's a terminal message, no more parts allowed at end
incoming.receive(3, req_parts, true);

//Get 2nd part explicitly (assume ZMQMESSAGE_STRING_CLASS is std::string):
std::string name = ZmqMessage::get_string(incoming[1]);
//or more verbose:
std::string name_cpy = ZmqMessage::get<std::string>(incoming[1]);

//if we also have some MyString class:
MyString my_id = ZmqMessage::get<MyString>(incoming[0]);

//or we can extract data as variables or plain zmq messages.
zmq::message_t blob;
incoming >> my_id >> name >> blob;

//or we can iterate on message parts and use standard algorithms:
std::ostream_iterator<MyString> out_it(std::cout, ", ");
std::copy(
  incoming.begin<MyString>(), incoming.end<MyString>(), out_it);

\endcode

Of course, passing message part names is not necessary (see \ref ZmqMessage::Incoming::receive "receive" functions).

There are cases when implemented protocol implies a fixed number of message parts at the beginning of multipart message.
And the number of subsequent messages is either undefined or estimated based on contents of first parts
(i.e. first part may contain command name, and other parts may contain data dependent on command).
So you first may call \ref ZmqMessage::Incoming::receive "receive" function with @c false as last parameter
(not checking if message is terminal), do something with first parts,
and then call \ref ZmqMessage::Incoming::receive "receive" or
\ref ZmqMessage::Incoming::receive_all "receive_all" again to fetch the rest.

\code
const char* req_parts[] = {"command"};
incoming.receive(1, req_parts, false);

std::string cmd = ZmqMessage::get_string(incoming[0]);

if (cmd == "SET_PARAM")
{
  const char* tail_parts[] = {"parameter 1 value (text)", "parameter 2 value (binary)"};
  incoming.receive(2, tail_parts, true);

  //message with parameter 1 contains text data converted into unsigned 32 bit integer (ex. "678" -> 678)
  uint32_t param1 = ZmqMessage::get<uint32_t>(incoming[1]);

  //message with parameter 2 contains binary data (unsigned 32 bit integer)
  uint32_t param2 = ZmqMessage::get_bin<uint32_t>(incoming[2]);

  //...
}
else
{
  //otherwise we receive all remaining message parts
  incoming.receive_all();

  if (incoming.size() > 1)
  {
    std::string first;
    incoming >> ZmqMessage::Skip //command is already analyzed
      >> first; // same as first = ZmqMessage::get<std::string>(incoming[1]);
    //...
  }
}

\endcode

\anchor ref_sending
<h3>Sending messages</h3>

To send a multipart message, you create instance of ZmqMessage::Outgoing.
For sending to XREQ and XREP socket types use \ref ZmqMessage::XRouting as template parameter,
for other socket types use \ref ZmqMessage::SimpleRouting.

\code
#include "ZmqMessage.hpp"

zmq::context_t ctx(1);
zmq::socket sock(ctx, ZMQ_XREQ); //will use XRouting
sock.connect("inproc://some-endpoint");

//create Outgoing specifying sending options: use nonblocking send
//and drop messages if operation would block
ZmqMessage::Outgoing<ZmqMessage::XRouting> outgoing(
  sock, ZmqMessage::OutOptions::NONBLOCK | ZmqMessage::OutOptions::DROP_ON_BLOCK);

//suppose we have some MyString class:
MyString id("112233");

outgoing << id << "SET_VARIABLES";

//Number will be converted to string (written to stream), cause Outgoing is in Text mode.
outgoing << 567099;

char buffer[128];
::memset (buffer, 'z', 128); //fill buffer

outgoing << ZmqMessage::RawMessage(buffer, 128);

//send message with binary number and flush it;
int num = 9988;
outgoing << ZmqMessage::Binary << num << ZmqMessage::Flush;
\endcode

Flushing Outgoing message sends final (terminal) message part (which was inserted before flushing),
and no more insertions allowed after it.
If you do not flush Outgoing message manually, it will flush in destructor.

*/


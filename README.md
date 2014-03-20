Netlib
======

Netlib is a simple client-server networking library. It is written in C++11 and uses SFML's networking module.

Non-blocking sockets are used without any threads for simplicity and stability.

More documentation and examples will be added in the next few days/weeks.

Classes
-------
All of these classes exist in the "net" namespace. If you want to use one in your project, simply include its header file, and make sure to compile the source along with it.

Most of these classes will require you to call "receive" in an update loop, because there are no threads to do that automatically.

### Server-side:

#### TcpServer

* This class handles managing TCP connections to multiple clients.
* You can easily connect to this using a TCP socket or the PacketOrganizer.

#### Other

* Eventually there may be some kind of account system.
* There might be more server-side UDP stuff too, but it would make more sense to be "connection-based" like TCP.
* I could also add a thin reliability layer above UDP to have ordered unique IDs on each packet, so that packets can be dropped if out of order (or put back in order). Along with this could be an unlimited packet size by stitching the packets together after all of the pieces are received.

### Client-side:

#### PacketOrganizer

* This class handles receiving/sending packets through TCP and/or UDP.
* It will automatically put the packets it receives into different queues based on the first value in the packet, or the packet "type".
* There is even callback support for handling these packets automatically, instead of manually checking if packets exist, getting a packet, and then popping it off the queue, continually until there are no more packets left. And you would have to do this for every different type.
* See the code for more details and usage instructions.

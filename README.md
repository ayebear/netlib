Netlib
======

Netlib is a simple client/server networking library. It is written in C++11 and uses SFML's networking module.

Non-blocking sockets are used on the client side, without any separate threads for simplicity and stability.

Blocking sockets with threads are used on the server side, for performance and efficiency.

Classes
-------
All of these classes exist in the "net" namespace. If you want to use one in your project, simply include its header file, and make sure to compile the source along with it.

For more advanced usage of these classes, please refer to the header files.

### Server-side:

#### TcpServer

* This class handles managing TCP connections to multiple clients.
* It uses a single thread for both handling connections and receiving packets.
* You can easily connect to this using a TCP socket or the PacketOrganizer.
* Callbacks can be set for the following events:
  * Client connected
    * Callback type: void(int)
    * The parameter passed is the client's unique ID.
  * Client disconnected
    * Callback type: void(int)
    * The parameter passed is the client's unique ID.
  * Packet received
    * Callback type: void(sf::Packet&, int)
    * The packet parameter passed is the packet received.
    * The int parameter passed is the client's unique ID.
* With these callbacks, you can easily determine which client sent what, and manage the clients in your own way.
  * For example, you could store an std::map of the clients' information.
  * The key would be the ID, and the value could be some structure of data associated with a client.
  * In your callback functions, you could add/remove the clients when they connect/disconnect, or when they authenticate with a packet.

##### Example usage

```
#include "tcpserver.h"

// Create a TCP server, and listen on port 2500
net::TcpServer server(2500);

// Setup callbacks
// Bind to a real function (you can also use a lambda)
using namespace std::placeholders;
server.setPacketCallback(std::bind(&MyServer::handlePacket, this, _1, _2));
// You can also set callbacks for when clients connect and disconnect

// Start the server
server.start();

// Do other things while the server is running
bool running = true;
while (running)
{
    // Do stuff...
    // If you are modifying the same objects as your callbacks, make sure to obtain the lock:
    {
        auto lock = server.getLock();
        // Modify thread-unsafe objects...
    }
    // Make sure the lock is destroyed (unlocked) as soon as possible,
    // so the server doesn't wait too long when calling a callback.
}

// Stop the server
server.stop();

// If you don't want the server to stop, call join() instead of stop()
server.join();
```

### Client-side:

#### PacketOrganizer

* This class handles receiving/sending packets through TCP and/or UDP.
* It will automatically put the packets it receives into different queues based on the first value in the packet, or the packet "type".
* There is callback support for handling these packets automatically.
  * Callback type: void(sf::Packet&)
  * You can register a callback for each packet type, using the registerCallback() method.

##### Example usage

```
net::PacketOrganizer client;
// More will be here later.
```

### Other

#### Address

* A simple class that holds an IP address and port.
  * The IP is stored in an sf::IpAddress.
* It can be set from a string, and can generate a string.
  * The string format is "IP:port", example: "10.0.0.1:80"
  * The IP can also be a domain or computer name, or anything sf::IpAddress supports.

#### Future plans

* Eventually there may be some kind of account system.
* There might be more server-side UDP stuff too, but it would make more sense to be "connection-based" like TCP.
  * This could support some kind of UDP hole-punching if it works.
* I could also add a thin reliability layer above UDP to have ordered unique IDs on each packet, so that packets can be dropped if out of order (or put back in order). Along with this could be an unlimited packet size by stitching the packets together after all of the pieces are received.

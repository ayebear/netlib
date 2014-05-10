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
* You can easily connect to this using a TCP socket or a net::Client.
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

// You can also set the listening port after construction
net::TcpServer server;
server.setListeningPort(2500);

// Setup callbacks
using namespace std::placeholders;
server.setPacketCallback(std::bind(&MyServer::handlePacket, this, _1, _2));
server.setConnectedCallback(std::bind(&MyServer::handleClientConnected, this, _1));
server.setDisconnectedCallback(std::bind(&MyServer::handleClientDisconnected, this, _1));

// Start the server (spawns a new thread, then returns)
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
// This will block forever as long as the server is running
```

### Client-side:

#### Client

* Note: This is being updated so that packets are handled immediately instead of being stored first (so it will be more efficient).
  * This will greatly simplify the interface, removing all of the arePackets/getPacket/popPacket/clear functions.
  * It will also fix all issues with packets being handled out of order.
  * Finer control over what groups of callbacks should be called will be possible with the new receive() overloads.
    * This way your application can be in different states, expecting only a subset of your packet types instead of all of them (which could possibly call functions in completely unrelated objects that aren't being dealt with at the moment)
  * These changes should be committed in the next few days.

* This class handles receiving/sending packets through TCP and/or UDP.
* It will automatically put the packets it receives into different queues based on the first value in the packet, or the packet "type".
* There is callback support for handling these packets automatically.
  * Callback type: void(sf::Packet&)
  * You can register a callback for each packet type, using the registerCallback() method.
  * Note: The callbacks are called in order of the packet type, from smallest to largest.

##### Example usage

```
#include "client.h"

// It is recommended to setup an enum for the packet types
enum PacketTypes {Message = 0, AddNumbers, TotalTypes};

// You can make separate functions or lambdas for handling packets
auto handleMessage = [](sf::Packet& packet)
{
    std::string str;
    if (packet >> str)
        std::cout << "Message received: " << str << std::endl;
};

auto handleAddNumbers = [](sf::Packet& packet)
{
    int a = 0;
    int b = 0;
    if (packet >> a >> b)
        std::cout << a << " + " << b << " = " << (a + b) << std::endl;
};

// Create a client object
net::Client client;

// Set the valid range of packet types to receive
client.setValidTypeRange(0, TotalTypes);

// Register callbacks for packet handling functions
client.registerCallback(Message, handleMessage);
client.registerCallback(AddNumbers, handleAddNumbers);
// Note: You can register as many callbacks as you'd like, but only one per type.

// In some loop (like your application's loop), call update()
client.update();
// This will receive and store all valid packets,
// and invoke the callbacks to handle them.
// If you don't want to use callbacks, you can use the arePackets/getPacket/popPacket functions.

// You can also send packets to the server you are connected to
sf::Packet packet;
client.tcpSend(packet);
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

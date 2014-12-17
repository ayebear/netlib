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
* Callbacks are used to handle different events (so you don't need to poll).

##### Example usage

Required steps for the server to work properly:
* Set listening port in constructor or with setListeningPort()
* Set packet handler callback with setPacketCallback()
* Call start() on the server object

###### Include and create instance

```
#include "tcpserver.h"

// Create a TCP server, and listen on port 2500
net::TcpServer server(2500);

// You can also set the listening port after construction
net::TcpServer server;
server.setListeningPort(2500);
```

###### Setup callbacks

Callbacks can be set for the following events:
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

With these callbacks, you can easily determine which client sent what, and manage the clients in your own way.
* For example, you could store an std::map of the clients' information.
* The key would be the ID, and the value could be some structure of data associated with a client.
* In your callback functions, you could add/remove the clients when they connect/disconnect, or when they authenticate with a packet.

Basic example:
```
void handlePacket(sf::Packet& packet, int id)
{
    std::string str;
    packet >> str;
    std::cout << "Packet received from " << id << ": " << str << "\n";
}
server.setPacketCallback(handlePacket);
```

Normally, you will want to use methods inside of classes, so here is an example with std::bind:
```
class MyServer
{
    public:
        void handlePacket(sf::Packet& packet, int id) {/*...*/}
};
MyServer myCustomServer;
using namespace std::placeholders;
server.setPacketCallback(std::bind(&MyServer::handlePacket, myCustomServer, _1, _2));

// Also set the connected/disconnected callbacks
server.setConnectedCallback(std::bind(&MyServer::handleClientConnected, myCustomServer, _1));
server.setDisconnectedCallback(std::bind(&MyServer::handleClientDisconnected, myCustomServer, _1));
```

For more information on std::bind, see this: http://en.cppreference.com/w/cpp/utility/functional/bind

###### Other settings

```
// Set maximum simultaneous connections
server.setConnectionLimit(16);

// Get maximum supported connections (OS dependent)
unsigned maxSupportedConnections = net::TcpServer::maxConnections;

// Set idle client timeout to 10 seconds
server.setClientTimeout(10.0f);
```

###### Running the server

```
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

###### Sending packets to clients

Send to a specific client:
```
server.send(packet, clientId);
```

Send to all clients:
```
server.sendToAll(packet);
```

Send to all clients except one:
```
server.sendToAll(packet, clientToExclude);
```

### Client-side:

#### Client

* This class handles receiving/sending packets through TCP and/or UDP.
* Callbacks are used to handle different packet types.
  * Callback type: void(sf::Packet&)
  * You can register a callback for each packet type, using the registerCallback() method.

##### Example usage

Required steps for the client to work properly:
* Register at least one packet callback.
* Connect to a server with TCP, or bind a UDP port.
* Call the receive() method when you want to handle the received packets.

###### Include and define some things

```
#include "client.h"

// It is recommended to setup an enum for the packet types
enum PacketTypes {Message = 0, AddNumbers, AnotherType, TotalTypes};

// Define your packet handling functions/lambdas
void handleMessage(sf::Packet& packet)
{
    std::string str;
    if (packet >> str)
        std::cout << "Message received: " << str << std::endl;
};

void handleAddNumbers(sf::Packet& packet)
{
    int a = 0;
    int b = 0;
    if (packet >> a >> b)
        std::cout << a << " + " << b << " = " << (a + b) << std::endl;
};
```

###### Create and setup client instance

```
// Create a client object
net::Client client;

// Register callbacks for packet handling functions
client.registerCallback(Message, handleMessage);
client.registerCallback(AddNumbers, handleAddNumbers);
// Note: You can register as many callbacks as you'd like, but only one per type.

// Setup packet type groups (refer to the PacketTypes enum defined earlier)
client.setGroup("someGroup", {Message, AnotherType});
client.setGroup("group2", {AddNumbers});
// Note: Setting groups is completely optional.
// Note: You can register as many groups as you'd like, with any number of packet types in the initializer list.
```

###### Receiving and handling packets

Calling the receive() method will receive data from all of the sockets, and then invoke all of the registered callbacks (or the group that was specified). Packets that were not handled (but have registered callbacks), are stored in a container. These stored packets are the first thing handled in the receive() method, as long as they match the specified type. They will stay in the container until they are handled, or until a call to clear() or keepOnly().

Normally you would want to call this continually in your application's loop.

```
// Receives and handles all registered packet types
client.receive();

// Receives and handles just the specified packet types
client.receive("someGroup");

// Removes all packets except for the ones whose types are in "group2"
// In this case, "AddNumbers" packets would be the only packets remaining
client.keepOnly("group2");

// Removes all of the stored unhandled packets
client.clear();
```

Notice before when "someGroup" was setup, that only "Message" and "AnotherType" were added. This means that when receive is called, it will only handle those two packet types, and storing (not handling) any packets of type "AddNumbers". This is useful in applications that have different states and shouldn't be invoking callbacks in unrelated objects at that point in time.

###### Sending packets

```
sf::Packet packet;
packet << "some data";

// Send packet through TCP to currently connected server
client.send(packet);

// Send packet through UDP to some address
client.send(packet, "10.0.0.1", 2500);

// You can also use a net::Address object
net::Address address("10.0.0.1:2500");
client.send(packet, address);
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

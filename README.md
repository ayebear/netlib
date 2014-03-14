Netlib
======

Netlib is a simple client-server networking library. It is written in C++11 and uses SFML's networking module.

Non-blocking sockets are used without any threads for simplicity and stability.

More documentation and examples will be added as the library is developed.

Classes
-------
All of these classes exist in the "net" namespace.

Most of these classes will require you to call "receive" in an update loop, because there are no threads to do that automatically.

### Server-side:

#### TcpServer

This class handles managing TCP connections to multiple clients. You can easily connect to this using the TcpClient class.

  * setListeningPort()
  * send(packet, id)
    * Sends a packet to a connected client
  * receive(packet)
    * This will need to return the client ID who sent it
  * update()
    * May need this for checking new connections on the TCP listener, as well as removing disconnected clients
  * Callbacks
    * These could pass the client ID to the callback (this way you could easily make an account login system)
    * clientConnectedCallback
    * clientDisconnectedCallback
  * And lots more...

Internal notes:
  * Probably won't need to use a socket selector, because of the non-blocking sockets
  * Just store the clients in unique_ptr/shared_ptr inside of some container
    * Might be able to store them directly, in like a map, or anything that doesn't reallocate
    * The map could be accessed by client ID, which could simplify things
    * Client IDs should be comparable to different types like IP addresses/ports, not sure if this will work with map's find()

#### Other
These are used internally by the TcpServer.
  * ClientManager
  * Client

### Client-side:

#### Transceiver
  * Just a base class with send and receive functions

#### TcpClient: Transceiver
  * connect(serverAddress)
  * disconnect()
  * send(packet)
  * receive(packet)

#### UdpNode: Transceiver
  * A server or client could use this
  * setPort(port)
    * Automatically binds the port to an available one if nothing was passed in
  * getPort()
  * send(packet)
  * receive(packet)
    * This will need to return information about the sender, so that the client can validate that it was the server who sent it

#### PacketOrganizer
  * Note: This class only deals with sf::Packet and not raw data
  * Stores the packets in a map of deques of packets
    * A map would allow any number of types without the user specifying anything
      * This way it could also be templated so you could have any type for the "key" type
  * addTransceiver(Transceiver&)
    * Can set any number of references to Transceiver objects
  * receive()
    * This will call receive on all of the references, and automatically organize and store the packets based on their types
  * getPacket(key)
  * popPacket(key)
  * arePackets(key)
  * clear()
    * Could call this after processing all known packet types, to get rid of any invalid ones
  * This could also have begin() and end() to be able to iterate through the map

#### AccountSystem/LoginManager
  * This is an extra higher level class that you can use to manage user accounts with a database.
  * There will be client-side and server-side parts
    * The client side will have a simple interface for logging in, creating accounts, changing passwords, etc.
    * The server side will handle a TcpServer and all of the database stuff

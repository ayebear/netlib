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

TcpServer
  * setPort()
  * send(packet, id)
  * receive(packet)
    * This will need to return the client ID who sent it
  * And lots more...

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

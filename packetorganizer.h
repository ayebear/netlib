// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#ifndef PACKETORGANIZER_H
#define PACKETORGANIZER_H

#include <map>
#include <deque>
#include <set>
#include <functional>
#include <SFML/Network.hpp>
#include "address.h"

namespace net
{

/*
About PacketOrganizer:
This class handles receiving packets and storing them into different containers based on their type.
    Note: It only handles sf::Packet type packets.
    Also, the packet type should be the first element in the packet and a 32-bit int.
It is meant to be used with client-side applications, and can communicate with a single server.
    If you need to communicate with multiple servers, simply make multiple instances of this class.
There is support for both UDP and TCP, you can use one or both.

Usage:
First, you should either connect to a TCP server with connect(),
    and/or bind a UDP port with setUdpPort().
Non-blocking sockets are used without threads, so in order for packets to be received, you must call
    the receive() method, which will populate the containers with any new packets received.
To read the packets, you would use getPacket() if there arePackets() for that type.
    Then, you would call popPacket() so that the next call to getPacket() won't return the same packet.
Alternatively, you can register callbacks to functions with this signature:
    void handleSomePacketType(sf::Packet& packet);
    using the registerCallback() method. Then, every time handlePackets() is called, the registered
    callbacks will be called for any new packets received. A callback will be called for each packet,
    and those packets will automatically be removed.

Near-future plans:
    Make it more clear on what uses TCP and what uses UDP by naming them more specifically.
    Make the packet header type templated
        This would let you use a string, char, or anything else
    Allow raw binary packets (not sure if there is an easy way to determine this)
        They could just be stored in their own separate deque
*/
class PacketOrganizer
{
    using PacketType = sf::Int32;
    using PacketQueue = std::deque<sf::Packet>;
    using AddressSet = std::set<Address>;
    using CallbackType = std::function<void(sf::Packet&)>;

    public:
        // Constructors/setup
        PacketOrganizer();
        bool connect(const sf::IpAddress& address, unsigned short port, sf::Time timeout=sf::Time::Zero);
        bool connect(const Address& address, sf::Time timeout=sf::Time::Zero);
            // Note that packets can only be received from the connected address through TCP, so there
            // is no need for checking for safe addresses.
        void setUdpPort(unsigned short port); // Bind UDP port to receive data on
        void setSafeAddresses(AddressSet& addresses); // Only accept UDP packets from these addresses
            // Note: If this is not set, then it will accept all packets

        // Communication
        bool update(); // Calls receive and handlePackets
        bool receive(); // Tries to receive data on all sockets, returns true if anything was received
        bool tcpSend(sf::Packet& packet);
        bool udpSend(sf::Packet& packet, const Address& address);
        bool udpSend(sf::Packet& packet, const sf::IpAddress& address, unsigned short port);
        bool isConnected() const; // Returns true if connected through TCP

        // Packets
        sf::Packet& getPacket(PacketType type); // Returns a packet
        bool popPacket(PacketType type); // Removes a packet (returns true if a packet was removed)
        bool arePackets(PacketType type) const; // Returns true if there are any packets
        void clear(); // Clears all packets of all types
        void clear(PacketType type); // Clears all packets of a specific type
        void setValidTypeRange(PacketType min, PacketType max);
            // Newly received packets will only be accepted if within this range

        // Callbacks for handling packets
        void registerCallback(PacketType type, CallbackType callback);
            // Registers (or re-registers) a callback for a specific packet type
        void handlePackets();
            // Calls all of the callbacks registered for the packet types received

    private:
        void storePacket(sf::Packet& packet); // Stores packets into the map
        bool isValidType(PacketType type) const; // Applies if you have a valid type range set
        bool isSafeAddress(const Address& address) const; // Applies if you set the safe addresses

        // Sockets
        sf::TcpSocket tcpSocket;
        sf::UdpSocket udpSocket;
        bool tcpConnected;
        bool udpReady;

        // All of the received packets are stored here
        std::map<PacketType, PacketQueue> packets;

        // Callbacks are stored in here
        std::map<PacketType, CallbackType> callbacks;

        // Valid packet type range
        PacketType minType;
        PacketType maxType;
        bool typeRangeSet;

        // UDP packets will only be received from these addresses
        AddressSet safeAddresses;
};

}

#endif

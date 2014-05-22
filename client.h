// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#ifndef CLIENT_H
#define CLIENT_H

#include <map>
#include <set>
#include <deque>
#include <functional>
#include <initializer_list>
#include <SFML/Network.hpp>
#include "address.h"

namespace net
{

/*
About Client:
    This class handles connecting to a server, and receiving/sending packets through UDP or TCP.
    Packets should be of type sf::Packet, with the first value being an sf::Int32 representing
        the type of data to be expected in the rest of the packet.
    Packets get automatically handled by callbacks that you can set for each packet type.
    It is meant to be used with client-side applications, and can communicate with a single server.
        If you need to communicate with multiple servers, simply make multiple instances of this class.

Usage:
    Refer to README.md.

Near-future plans:
    Make the packet header type templated
        This would let you use a string, char, or anything else that can go into an sf::Packet
    Allow raw binary packets (not sure if there is an easy way to determine this)
*/
class Client
{
    public:
        // Types
        using PacketType = sf::Int32;
        using AddressSet = std::set<Address>;
        using CallbackType = std::function<void(sf::Packet&)>;
        enum Status
        {
            Nothing = 0,
            Received = 1,
            Handled = 2
        };

        // Constructors/setup
        Client();

        // TCP socket
        bool connect(const sf::IpAddress& address, unsigned short port, sf::Time timeout = sf::Time::Zero);
        bool connect(const Address& address, sf::Time timeout = sf::Time::Zero);
        void disconnect();

        // UDP socket
        void bindPort(unsigned short port); // Bind UDP port to receive data on
        void setSafeAddresses(AddressSet& addresses); // Only accept UDP packets from these addresses
            // Note: If this is not set, then it will accept all packets

        // Communication
        int receive(const std::string& groupName = ""); // Receives and handles all or specified packet types
        bool send(sf::Packet& packet); // Send packet through TCP
        bool send(sf::Packet& packet, const Address& address); // Send packet through UDP
        bool send(sf::Packet& packet, const sf::IpAddress& address, unsigned short port); // Send packet through UDP
        bool isConnected() const; // Returns true if connected through TCP

        // Packet handling
        void registerCallback(PacketType type, CallbackType callback);
        void setGroup(const std::string& groupName, std::initializer_list<PacketType> packetTypes);
        void keepOnly(const std::string& groupName); // Removes all other packets
        void clear(); // Removes all of the stored unhandled packets

    private:
        int receiveUdp(const std::string& groupName = "");
        int receiveTcp(const std::string& groupName = "");
        int handlePacket(sf::Packet& packet, const std::string& groupName = "");
        void handlePacketType(sf::Packet& packet, PacketType type);
        bool isSafeAddress(const Address& address) const;
        void storePacket(sf::Packet& packet, PacketType type);
        int handleStoredPackets(const std::string& groupName = "");

        // Sockets
        sf::TcpSocket tcpSocket;
        sf::UdpSocket udpSocket;
        bool tcpConnected;
        bool udpReady;

        // Callbacks are stored in here
        std::map<PacketType, CallbackType> callbacks;

        // Groups of packet types are stored in here
        std::map<std::string, std::set<PacketType> > groups;

        // Packets to be handled when specified
        using PacketPair = std::pair<PacketType, sf::Packet>;
        std::deque<PacketPair> packets;

        // UDP packets will only be received from these addresses
        AddressSet safeAddresses;
};

}

#endif

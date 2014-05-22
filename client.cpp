// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#include "client.h"

namespace net
{

Client::Client():
    tcpConnected(false),
    udpReady(false)
{
    udpSocket.setBlocking(false);
    tcpSocket.setBlocking(false);
}

bool Client::connect(const sf::IpAddress& address, unsigned short port, sf::Time timeout)
{
    // Use a blocking connect
    tcpSocket.setBlocking(true);
    tcpConnected = (tcpSocket.connect(address, port, timeout) == sf::Socket::Done);
    tcpSocket.setBlocking(false);
    return tcpConnected;
}

bool Client::connect(const Address& address, sf::Time timeout)
{
    return connect(address.ip, address.port, timeout);
}

void Client::disconnect()
{
    tcpSocket.disconnect();
    tcpConnected = false;
}

void Client::bindPort(unsigned short port)
{
    udpSocket.bind(port);
    udpReady = true;
}

void Client::setSafeAddresses(AddressSet& addresses)
{
    safeAddresses = addresses;
}

int Client::receive(const std::string& groupName)
{
    // Handle the stored packets first, since those are the oldest
    int status = handleStoredPackets(groupName);
    status |= receiveUdp(groupName);
    status |= receiveTcp(groupName);
    // This returns true if anything was handled or received
    return status;
}

bool Client::send(sf::Packet& packet)
{
    return (tcpConnected && tcpSocket.send(packet) == sf::Socket::Done);
}

bool Client::send(sf::Packet& packet, const Address& address)
{
    return (udpSocket.send(packet, address.ip, address.port) == sf::Socket::Done);
}

bool Client::send(sf::Packet& packet, const sf::IpAddress& address, unsigned short port)
{
    return (udpSocket.send(packet, address, port) == sf::Socket::Done);
}

bool Client::isConnected() const
{
    return tcpConnected;
}

void Client::registerCallback(PacketType type, CallbackType callback)
{
    callbacks[type] = callback;
}

void Client::setGroup(const std::string& groupName, std::initializer_list<PacketType> packetTypes)
{
    groups[groupName] = packetTypes;
}

void Client::keepOnly(const std::string& groupName)
{
    auto groupFound = groups.find(groupName);
    if (groupFound != groups.end())
    {
        auto shouldRemove = [&](const PacketPair& packet)
        {
            // The packet should be removed if the type is not found in the group
            return (groupFound->second.find(packet.first) == groupFound->second.end());
        };
        packets.erase(std::remove_if(packets.begin(), packets.end(), shouldRemove), packets.end());
    }
}

void Client::clear()
{
    packets.clear();
}

int Client::receiveUdp(const std::string& groupName)
{
    // Receive and handle any UDP packets
    int status = Nothing;
    if (udpReady)
    {
        Address address;
        sf::Packet packet;
        while (udpSocket.receive(packet, address.ip, address.port) == sf::Socket::Done)
        {
            if (isSafeAddress(address))
            {
                status |= Received;
                status |= handlePacket(packet, groupName);
            }
        }
    }
    return status;
}

int Client::receiveTcp(const std::string& groupName)
{
    // Receive and handle any TCP packets
    int status = Nothing;
    if (tcpConnected)
    {
        sf::Packet packet;
        auto socketStatus = tcpSocket.receive(packet);
        while (socketStatus == sf::Socket::Done)
        {
            status |= Received;
            status |= handlePacket(packet, groupName);
            socketStatus = tcpSocket.receive(packet);
        }
        if (socketStatus == sf::Socket::Disconnected || socketStatus == sf::Socket::Error)
            tcpConnected = false;
    }
    return status;
}

int Client::handlePacket(sf::Packet& packet, const std::string& groupName)
{
    int status = Nothing;
    // Extract the packet type and make sure it is valid
    PacketType type = -1;
    if (packet >> type)
    {
        // Handle the packet if no group name is specified
        if (groupName.empty())
        {
            handlePacketType(packet, type);
            status = Handled;
        }
        else
        {
            // Only handle the packet if the group is valid and the type is part of the group
            // Otherwise, store the packet for later
            auto groupFound = groups.find(groupName);
            if (groupFound != groups.end() && groupFound->second.find(type) != groupFound->second.end())
            {
                handlePacketType(packet, type);
                status = Handled;
            }
            else
                storePacket(packet, type);
        }
    }
    return status;
}

void Client::handlePacketType(sf::Packet& packet, PacketType type)
{
    // Lookup the callback to use, and call it if it exists
    auto found = callbacks.find(type);
    if (found != callbacks.end() && found->second)
        found->second(packet);
}

bool Client::isSafeAddress(const Address& address) const
{
    return (safeAddresses.empty() || safeAddresses.find(address) != safeAddresses.end());
}

void Client::storePacket(sf::Packet& packet, PacketType type)
{
    packets.emplace_back(type, packet);
}

int Client::handleStoredPackets(const std::string& groupName)
{
    int status = Nothing;
    if (!packets.empty())
    {
        if (groupName.empty())
        {
            // Handle all of the stored packets, then clear them
            for (auto& packet: packets)
                handlePacketType(packet.second, packet.first);
            packets.clear();
            status = Handled;
        }
        else
        {
            auto groupFound = groups.find(groupName);
            if (groupFound != groups.end())
            {
                // Handle only the specified packets, and erase them
                auto packet = packets.begin();
                while (packet != packets.end())
                {
                    if (groupFound->second.find(packet->first) != groupFound->second.end())
                    {
                        handlePacketType(packet->second, packet->first);
                        packet = packets.erase(packet);
                        status = Handled;
                    }
                    else
                        ++packet;
                }
            }
        }
    }
    return status;
}

}

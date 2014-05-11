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

bool Client::receive(const std::string& groupName)
{
    bool status = receiveUdp(groupName);
    status |= receiveTcp(groupName);
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

bool Client::receiveUdp(const std::string& groupName)
{
    // Receive and handle any UDP packets
    bool status = false;
    if (udpReady)
    {
        Address address;
        sf::Packet packet;
        while (udpSocket.receive(packet, address.ip, address.port) == sf::Socket::Done)
        {
            if (isSafeAddress(address))
            {
                status = true;
                handlePacket(packet, groupName);
            }
        }
    }
    return status;
}

bool Client::receiveTcp(const std::string& groupName)
{
    // Receive and handle any TCP packets
    bool status = false;
    if (tcpConnected)
    {
        sf::Packet packet;
        auto socketStatus = tcpSocket.receive(packet);
        while (socketStatus == sf::Socket::Done)
        {
            status = true;
            handlePacket(packet, groupName);
            socketStatus = tcpSocket.receive(packet);
        }
        if (socketStatus == sf::Socket::Disconnected || socketStatus == sf::Socket::Error)
            tcpConnected = false;
    }
    return status;
}

void Client::handlePacket(sf::Packet& packet, const std::string& groupName)
{
    // Extract the packet type and make sure it is valid
    PacketType type = -1;
    if (packet >> type)
    {
        // Handle the packet if no group name is specified
        if (groupName.empty())
            handlePacketType(packet, type);
        else
        {
            // Only handle the packet if the type is part of the group
            auto groupFound = groups.find(groupName);
            if (groupFound != groups.end() && groupFound->second.find(type) != groupFound->second.end())
                handlePacketType(packet, type);
        }
    }
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

}

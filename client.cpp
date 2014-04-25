// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#include "client.h"

namespace net
{

Client::Client():
    tcpConnected(false),
    udpReady(false),
    typeRangeSet(false)
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

void Client::setUdpPort(unsigned short port)
{
    udpSocket.bind(port);
    udpReady = true;
}

void Client::setSafeAddresses(AddressSet& addresses)
{
    safeAddresses = addresses;
}

bool Client::update()
{
    bool status = receive();
    handlePackets();
    return status;
}

bool Client::receive()
{
    bool status = false;
    Address address;
    sf::Packet packet;
    if (udpReady && udpSocket.receive(packet, address.ip, address.port) == sf::Socket::Done && isSafeAddress(address))
    {
        status = true;
        storePacket(packet);
        packet.clear();
    }
    if (tcpConnected && tcpSocket.receive(packet) == sf::Socket::Done)
    {
        status = true;
        storePacket(packet);
    }
    return status;
}

bool Client::tcpSend(sf::Packet& packet)
{
    return (tcpSocket.send(packet) == sf::Socket::Done);
}

bool Client::udpSend(sf::Packet& packet, const Address& address)
{
    return (udpSocket.send(packet, address.ip, address.port) == sf::Socket::Done);
}

bool Client::udpSend(sf::Packet& packet, const sf::IpAddress& address, unsigned short port)
{
    return (udpSocket.send(packet, address, port) == sf::Socket::Done);
}

bool Client::isConnected() const
{
    return tcpConnected;
}

sf::Packet& Client::getPacket(PacketType type)
{
    return (packets[type].front());
}

bool Client::popPacket(PacketType type)
{
    packets[type].pop_front();
    return (arePackets(type));
}

bool Client::arePackets(PacketType type) const
{
    auto found = packets.find(type);
    return (found != packets.end() && !found->second.empty());
}

void Client::clear()
{
    packets.clear();
}

void Client::clear(PacketType type)
{
    packets[type].clear();
}

void Client::setValidTypeRange(PacketType min, PacketType max)
{
    minType = min;
    maxType = max;
    typeRangeSet = true;
}

void Client::registerCallback(PacketType type, CallbackType callback)
{
    callbacks[type] = callback;
}

void Client::handlePackets()
{
    // Loop through each registered callback
    for (auto& handler: callbacks)
    {
        auto type = handler.first;
        auto callback = handler.second;
        if (callback) // Make sure the callback was set properly
        {
            // Handle all of the packets received of this type
            while (arePackets(type))
            {
                callback(getPacket(type));
                popPacket(type);
            }
        }
    }
}

void Client::storePacket(sf::Packet& packet)
{
    PacketType type = -1;
    if (packet >> type && isValidType(type))
        packets[type].push_back(packet);
}

bool Client::isValidType(PacketType type) const
{
    return (!typeRangeSet || (type >= minType && type <= maxType));
}

bool Client::isSafeAddress(const Address& address) const
{
    return (safeAddresses.empty() || safeAddresses.find(address) != safeAddresses.end());
}

}

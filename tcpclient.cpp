// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#include "tcpclient.h"

namespace net
{

TcpClient::TcpClient()
{
    tcpSocket.setBlocking(false);
}

TcpClient::~TcpClient()
{
}

bool TcpClient::connect(const sf::IpAddress& address, unsigned short port, sf::Time timeout)
{
    tcpSocket.setBlocking(true);
    bool status = (tcpSocket.connect(address, port, timeout) == sf::Socket::Done);
    tcpSocket.setBlocking(false);
    return status;
}

void TcpClient::disconnect()
{
    tcpSocket.disconnect();
}

bool TcpClient::send(sf::Packet& packet)
{
    return (tcpSocket.send(packet) == sf::Socket::Done);
}

bool TcpClient::receive(sf::Packet& packet)
{
    return (tcpSocket.receive(packet) == sf::Socket::Done);
}

}

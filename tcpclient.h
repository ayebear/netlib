// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "transceiver.h"

namespace net
{

// This class is basically just an sf::TcpSocket, I might get rid of it unless I can make it higher level somehow
class TcpClient : public Transceiver
{
    public:
        TcpClient();
        virtual ~TcpClient();
        bool connect(const sf::IpAddress& address, unsigned short port, sf::Time timeout=sf::Time::Zero);
        void disconnect();
        bool send(sf::Packet& packet);
        bool receive(sf::Packet& packet);

    private:
        sf::TcpSocket tcpSocket;
};

}

#endif

// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <SFML/Network.hpp>

namespace net
{

class Transceiver
{
    public:
        Transceiver() {}
        virtual ~Transceiver() {}
        virtual bool send(sf::Packet& packet) = 0;
        virtual bool receive(sf::Packet& packet) = 0;
};

}

#endif

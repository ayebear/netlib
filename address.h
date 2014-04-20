// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#ifndef ADDRESS_H
#define ADDRESS_H

#include <SFML/Network.hpp>

namespace net
{

// Simple class that stores an IP address and port
// Can be set from a string, and can generate a string
struct Address
{
    Address();

    // These use a format like "ip:port"
    Address(const std::string& str);
    Address(const std::string& str, unsigned short p); // The string here should just be the IP
    bool set(const std::string& str);
    bool set(const std::string& str, unsigned short p);
    bool operator=(const std::string& str);
    std::string toString() const;

    // For use with associative containers
    bool operator<(const Address& addr) const;
    bool operator==(const Address& addr) const;

    // These can be public because the string is generated on demand
    sf::IpAddress ip;
    unsigned short port;
};

}

#endif

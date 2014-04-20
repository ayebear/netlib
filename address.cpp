// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#include "address.h"
#include <sstream>
#include <iostream>

namespace net
{

Address::Address():
    port(0)
{
}

Address::Address(const std::string& str):
    Address()
{
    set(str);
}

Address::Address(const std::string& str, unsigned short p):
    ip(str),
    port(p)
{
}

bool Address::set(const std::string& str)
{
    bool status = false;
    auto separator = str.find(':');
    if (separator != std::string::npos)
    {
        // Split the string
        std::string ipStr = str.substr(0, separator);
        std::string portStr = str.substr(separator + 1);
        // Extract the port number
        std::istringstream tmpStream(portStr);
        unsigned short tmpPort = 0;
        status = (tmpStream >> tmpPort);
        // Set the values if all went well
        if (status)
        {
            ip = ipStr;
            port = tmpPort;
        }
    }
    return status;
}

bool Address::set(const std::string& str, unsigned short p)
{
    ip = str;
    port = p;
    return true;
}

bool Address::operator=(const std::string& str)
{
    return set(str);
}

std::string Address::toString() const
{
    std::ostringstream tmpStream;
    tmpStream << ip.toString() << ":" << port;
    return tmpStream.str();
}

bool Address::operator<(const Address& addr) const
{
    if (ip == addr.ip)
        return (port < addr.port);
    else
        return (ip < addr.ip);
}

bool Address::operator==(const Address& addr) const
{
    return (ip == addr.ip && port == addr.port);
}

}

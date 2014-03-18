// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#include "tcpserver.h"
#include <iostream>

namespace net
{

TcpServer::TcpServer()
{
    clientPos = 0;
    listener.setBlocking(false);
}

TcpServer::TcpServer(unsigned short port):
    TcpServer()
{
    setListeningPort(port);
}

TcpServer::TcpServer(unsigned short port, CallbackType callbackC, CallbackType callbackD):
    TcpServer()
{
    setListeningPort(port);
    setConnectedCallback(callbackC);
    setDisconnectedCallback(callbackD);
}

void TcpServer::setListeningPort(unsigned short port)
{
    listener.listen(port);
}

void TcpServer::setConnectedCallback(CallbackType callback)
{
    connectedCallback = callback;
}

void TcpServer::setDisconnectedCallback(CallbackType callback)
{
    disconnectedCallback = callback;
}

bool TcpServer::send(sf::Packet& packet)
{
    bool status = true;
    for (int id: clientIds)
    {
        if (!send(packet, id))
            status = false;
    }
    return status;
}

bool TcpServer::send(sf::Packet& packet, int id)
{
    bool status = false;
    if (clientIsConnected(id))
        status = (clients[id]->send(packet) == sf::Socket::Done);
    return status;
}

bool TcpServer::receive(sf::Packet& packet, int& id)
{
    // Loop through all of the clients (from last position), until something is received
    // If nothing is received, then return false
    bool status = false;
    int totalIds = clientIds.size();
    // count is used so that this will check every client
    for (int count = 0; !status && count < totalIds; ++clientPos, ++count)
    {
        if (clientPos >= totalIds)
            clientPos = 0;
        int clientId = clientIds[clientPos];
        if (clientIsConnected(clientId))
        {
            // Try to receive data from the socket
            auto socketStatus = clients[clientId]->receive(packet);
            if (socketStatus == sf::Socket::Done)
            {
                // Successfully received data
                status = true;
                id = clientId;
            }
            else if (socketStatus == sf::Socket::Error || socketStatus == sf::Socket::Disconnected)
                clientsToRemove.push_back(clientId); // Remove the client
        }
    }
    removeClientsToRemove();
    return status;
}

bool TcpServer::update()
{
    // TODO: Could maybe have a max idle time for connected clients, if they haven't sent/received data
    removeOldClients();
    return acceptNewClients();
}

sf::IpAddress TcpServer::getClientAddress(int id) const
{
    sf::IpAddress ip;
    if (clientIsConnected(id))
        ip = clients[id]->getRemoteAddress();
    return ip;
}

void TcpServer::kickClient(int id)
{
    // Disconnect and remove the client
    if (clientIsConnected(id))
    {
        clients[id]->disconnect();
        removeClient(id);
    }
}

bool TcpServer::clientIsConnected(int id) const
{
    // Checks if the client ID is valid, and if the TCP socket is created and connected
    return (id >= 0 && id < (int)clients.size() && clients[id] && clients[id]->getRemotePort() != 0);
}

bool TcpServer::acceptNewClients()
{
    // Accept and add any new clients
    bool status = false;
    setupClient(tmpClient);
    while (listener.accept(*tmpClient) == sf::Socket::Done)
    {
        addClient(std::move(tmpClient));
        status = true;
        setupClient(tmpClient);
    }
    return status;
}

void TcpServer::removeOldClients()
{
    for (int id: clientIds)
    {
        if (!clientIsConnected(id))
            clientsToRemove.push_back(id);
    }
    removeClientsToRemove();
}

int TcpServer::addClient(TcpSocketPtr newClient)
{
    int id = 0;
    if (freeClientIds.empty())
    {
        // Generate a new ID and add a new client
        id = clients.size();
        clients.push_back(std::move(newClient));
    }
    else
    {
        // Use an existing ID and replace the client
        id = freeClientIds.back();
        freeClientIds.pop_back();
        clients[id] = std::move(newClient);
    }
    clientIds.push_back(id);
    if (connectedCallback)
        connectedCallback(id);
    return id;
}

void TcpServer::removeClient(int id)
{
    // TODO: Come up with a more efficient way to do this
    // Probably will need to use a set or different data structure
    auto found = find(clientIds.begin(), clientIds.end(), id);
    if (found != clientIds.end())
    {
        clientIds.erase(found);
        freeClientIds.push_back(id);
        clients[id].reset();
        if (disconnectedCallback)
            disconnectedCallback(id);
    }
}

void TcpServer::setupClient(TcpSocketPtr& client)
{
    if (!client)
    {
        // Create a new TcpSocket in non-blocking mode
        client.reset(new sf::TcpSocket());
        client->setBlocking(false);
    }
}

void TcpServer::removeClientsToRemove()
{
    for (int id: clientsToRemove)
        removeClient(id);
    clientsToRemove.clear();
}

}

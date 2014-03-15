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
    bool status = false;
    for (int id: clientIds)
        status = send(packet, id);
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
    // TODO: Improve the way this works. It should loop through ALL of the sockets instead of just
        // the ones after the last one received.

    // Loop through all of the clients (from last position), until something is received
    // If nothing is received, then return false
    bool status = false;
    if (clientPos >= (int)clientIds.size())
        clientPos = 0;
    for (; !status && clientPos < (int)clientIds.size(); ++clientPos)
    {
        int clientId = clientIds[clientPos];
        if (clientIsConnected(clientId) && clients[clientId]->receive(packet) == sf::Socket::Done)
        {
            status = true;
            id = clientId;
            //++clientPos; // So that next time this loops, it won't be on the same client
        }
    }
    return status;
}

void TcpServer::update()
{
    // Could maybe have a max idle time for connected clients, if they haven't sent/received data
    acceptNewClients();
    removeOldClients();
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

void TcpServer::acceptNewClients()
{
    // Accept and add any new clients
    setupClient(tmpClient);
    while (listener.accept(*tmpClient) == sf::Socket::Done)
    {
        addClient(std::move(tmpClient));
        setupClient(tmpClient);
    }
}

void TcpServer::removeOldClients()
{
    auto tmpIds = clientIds; // Need this because we are removing elements
        // from the array being iterated through
    for (int id: tmpIds)
    {
        if (!clientIsConnected(id))
            removeClient(id);
    }
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

}

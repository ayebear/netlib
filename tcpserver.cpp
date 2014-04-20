// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#include "tcpserver.h"
//#include <iostream>
//using std::cout;

namespace net
{

TcpServer::TcpServer()
{
    clientPos = 0;
    listener.setBlocking(true);
    listenerAdded = false;
}

TcpServer::TcpServer(unsigned short port):
    TcpServer()
{
    setListeningPort(port);
}

TcpServer::TcpServer(unsigned short port, CallbackType c1, CallbackType c2, PacketCallbackType c3):
    TcpServer()
{
    setListeningPort(port);
    setConnectedCallback(c1);
    setDisconnectedCallback(c2);
    setPacketCallback(c3);
}

TcpServer::~TcpServer()
{
    // Wait for the thread to finish if it is running
    running = false;
    join();
}

void TcpServer::setListeningPort(unsigned short port)
{
    listener.listen(port);
    // Only add the listener after it starts listening on a port
    if (!listenerAdded)
    {
        selector.add(listener);
        listenerAdded = true;
    }
}

void TcpServer::setConnectedCallback(CallbackType callback)
{
    connectedCallback = callback;
}

void TcpServer::setDisconnectedCallback(CallbackType callback)
{
    disconnectedCallback = callback;
}

void TcpServer::setPacketCallback(PacketCallbackType callback)
{
    packetCallback = callback;
}

TcpServer::LockType TcpServer::getLock()
{
    return LockType(callbackMutex);
}

bool TcpServer::send(sf::Packet& packet)
{
    bool status = true;
    LockType lock(internalMutex);
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
    LockType lock(internalMutex);
    if (clientIsConnected(id))
        status = (clients[id]->send(packet) == sf::Socket::Done);
    return status;
}

void TcpServer::start()
{
    if (!serverThread.joinable())
        serverThread = std::thread(&TcpServer::serverLoop, this);
}

void TcpServer::stop()
{
    running = false;
    join();
    LockType lock(internalMutex);
    selector.clear();
    selector.add(listener);
    clients.clear();
    clientIds.clear();
    freeClientIds.clear();
    clientsToRemove.clear();
}

void TcpServer::join()
{
    if (serverThread.joinable())
        serverThread.join();
}

sf::IpAddress TcpServer::getClientAddress(int id) const
{
    sf::IpAddress ip;
    LockType lock(internalMutex);
    if (clientIsConnectedNoLock(id))
        ip = clients[id]->getRemoteAddress();
    return ip;
}

void TcpServer::kickClient(int id)
{
    // Disconnect and remove the client
    LockType lock(internalMutex);
    if (clientIsConnectedNoLock(id))
    {
        clients[id]->disconnect();
        removeClient(id);
    }
}

bool TcpServer::clientIsConnected(int id) const
{
    LockType lock(internalMutex);
    return clientIsConnectedNoLock(id);
}

void TcpServer::serverLoop()
{
    running = true;
    while (running)
    {
        //cout << "SERVER: Running loop...\n";
        if (selector.wait(sf::milliseconds(500)))
        {
            LockType lock(internalMutex);
            if (selector.isReady(listener))
                acceptNewClient();
            else
                receive();
        }
    }
}

void TcpServer::receive()
{
    //cout << "SERVER: receive() called.\n";
    // Loop through all of the clients (from last position), until something is received
    bool status = false;
    int totalIds = clientIds.size();
    // count is used so that this will check every client
    for (int count = 0; !status && count < totalIds; ++clientPos, ++count)
    {
        if (clientPos >= totalIds)
            clientPos = 0;
        int clientId = clientIds[clientPos];
        //cout << "SERVER: Checking client " << clientId << ".\n";
        if (clientIsConnectedNoLock(clientId) && selector.isReady(*clients[clientId]))
        {
            // Try to receive data from the socket
            sf::Packet packet;
            auto socketStatus = clients[clientId]->receive(packet);
            if (socketStatus == sf::Socket::Done)
            {
                // Handle the received data
                status = true;
                if (packetCallback)
                {
                    LockType lock(callbackMutex);
                    packetCallback(packet, clientId);
                }
            }
            else if (socketStatus == sf::Socket::Error || socketStatus == sf::Socket::Disconnected)
                clientsToRemove.push_back(clientId); // Remove the client
        }
    }
    // TODO: Could maybe have a max idle time for connected clients,
    // if they haven't sent/received data in a while
    removeOldClients();
}

void TcpServer::acceptNewClient()
{
    // Accept and add a new client
    setupClient(tmpClient);
    if (listener.accept(*tmpClient) == sf::Socket::Done)
        addClient(std::move(tmpClient));
}

void TcpServer::removeOldClients()
{
    for (int id: clientIds)
    {
        if (!clientIsConnectedNoLock(id))
            clientsToRemove.push_back(id);
    }
    removeClientsToRemove();
}

int TcpServer::addClient(TcpSocketPtr newClient)
{
    int id = 0;
    selector.add(*newClient);
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
    {
        LockType lock(callbackMutex);
        connectedCallback(id);
    }
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
        selector.remove(*clients[id]);
        clients[id].reset();
        if (disconnectedCallback)
        {
            LockType lock(callbackMutex);
            disconnectedCallback(id);
        }
    }
}

void TcpServer::setupClient(TcpSocketPtr& client)
{
    if (!client)
    {
        // Create a new TcpSocket in non-blocking mode
        client.reset(new sf::TcpSocket());
        client->setBlocking(true);
    }
}

void TcpServer::removeClientsToRemove()
{
    for (int id: clientsToRemove)
        removeClient(id);
    clientsToRemove.clear();
}

bool TcpServer::clientIsConnectedNoLock(int id) const
{
    // Checks if the client ID is valid, and if the TCP socket is created and connected
    return (id >= 0 && id < (int)clients.size() && clients[id] && clients[id]->getRemotePort() != 0);
}

}

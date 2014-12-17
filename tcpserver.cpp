// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#include "tcpserver.h"

namespace net
{

TcpServer::TcpServer():
    lastId(0),
    listenerAdded(false),
    connectionLimit(maxConnections),
    timeout(0.0f)
{
    listener.setBlocking(true);
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

bool TcpServer::setConnectionLimit(unsigned connections)
{
    bool status = false;
    if (connections <= maxConnections)
    {
        connectionLimit = connections;
        status = true;
    }
    return status;
}

void TcpServer::setClientTimeout(float t)
{
    timeout = t;
}

TcpServer::LockType TcpServer::getLock()
{
    return LockType(callbackMutex);
}

bool TcpServer::send(sf::Packet& packet, int id)
{
    bool status = false;
    LockType lock(internalMutex);
    auto found = clients.find(id);
    if (clientIsConnected(found))
        status = (found->second.socket->send(packet) == sf::Socket::Done);
    return status;
}

bool TcpServer::sendToAll(sf::Packet& packet, int id)
{
    bool status = true;
    LockType lock(internalMutex);
    for (auto& client: clients)
    {
        // Don't send anything to the excluded client
        if (id != client.first && client.second.socket)
        {
            if (client.second.socket->send(packet) != sf::Socket::Done)
                status = false;
        }
    }
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
    auto found = clients.find(id);
    if (clientIsConnected(found))
        ip = found->second.socket->getRemoteAddress();
    return ip;
}

void TcpServer::kickClient(int id)
{
    // Remove the client (which also disconnects them)
    LockType lock(internalMutex);
    removeClient(clients.find(id));
}

bool TcpServer::clientIsConnected(int id) const
{
    LockType lock(internalMutex);
    return clientIsConnected(clients.find(id));
}

void TcpServer::serverLoop()
{
    running = true;
    while (running)
    {
        // Don't wait forever on the selector, so that the loop can gracefully end
        if (selector.wait(sf::milliseconds(500)))
        {
            LockType lock(internalMutex);
            if (selector.isReady(listener))
                acceptNewClient();
            else
                receive();
        }
        else
        {
            LockType lock(internalMutex);
            receive();
            // Just to remove old connections
        }
    }
}

void TcpServer::receive()
{
    // Loop through all of the sockets, and receive any data
    auto clientIter = clients.begin();
    while (clientIter != clients.end())
    {
        bool shouldRemoveClient = false;
        auto& timer = clientIter->second.timer;
        if (clientIter->second.socket)
        {
            auto& client = *(clientIter->second.socket);
            shouldRemoveClient = (client.getRemotePort() == 0);
            if (!shouldRemoveClient && selector.isReady(client))
            {
                // Try to receive data from the socket
                sf::Packet packet;
                auto socketStatus = client.receive(packet);
                if (socketStatus == sf::Socket::Done)
                {
                    timer.restart();
                    // Handle the received data
                    if (packetCallback)
                    {
                        LockType lock(callbackMutex);
                        packetCallback(packet, clientIter->first);
                    }
                }
                else if (socketStatus != sf::Socket::NotReady)
                    shouldRemoveClient = true;
            }
        }

        // Check if the client has been idle for longer than the timeout
        if (!shouldRemoveClient && timeout > 0.0f && timer.getElapsedTime().asSeconds() >= timeout)
            shouldRemoveClient = true;

        // Increment iterator, remove client if it needs to be removed
        if (shouldRemoveClient)
            clientIter = removeClient(clientIter);
        else
            ++clientIter;
    }
}

void TcpServer::acceptNewClient()
{
    // Accept and add a new client
    setupClient(tmpClient);
    if (listener.accept(*tmpClient) == sf::Socket::Done)
    {
        // Gracefully close any new connections over the limit
        if (clients.size() < connectionLimit)
            addClient(std::move(tmpClient));
        else
            tmpClient.reset();
    }
}

int TcpServer::addClient(TcpSocketPtr newClient)
{
    // Generate new ID
    int id = lastId++;

    // Add to the selector and the map
    selector.add(*newClient);
    clients[id].socket = std::move(newClient);

    // Call the client connected callback
    if (connectedCallback)
    {
        LockType lock(callbackMutex);
        connectedCallback(id);
    }
    return id;
}

TcpServer::ClientMap::iterator TcpServer::removeClient(ClientMap::iterator it)
{
    if (it != clients.end())
    {
        // Remove the socket from the selector, and disconnect it
        if (it->second.socket)
        {
            selector.remove(*it->second.socket);
            it->second.socket->disconnect();
        }

        // Save the ID
        int id = it->first;

        // Remove the smart pointer from the map
        it = clients.erase(it);

        // Call the client disconnected callback
        if (disconnectedCallback)
        {
            LockType lock(callbackMutex);
            disconnectedCallback(id);
        }
    }
    return it;
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

bool TcpServer::clientIsConnected(ClientMap::const_iterator it) const
{
    return (it != clients.end() && it->second.socket && it->second.socket->getRemotePort() != 0);
}

}

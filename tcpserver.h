// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <vector>
#include <memory>
#include <functional>
#include <SFML/Network.hpp>
#include "clientmanager.h"

namespace net
{

/*
This class acts as a server that manages multiple TCP connections.
It can handle new connections and disconnects, and can even invoke optional callbacks when these events occur.
No threads are used, it uses all non-blocking sockets and a non-blocking listener.
All clients can be accessed by their unique ID, which is simply an int.

To use this as a server, you must first set a listening port, then continually call update() in a loop. If you want to receive packets, you can call receive (which you would normally do in the same loop as update()).
*/
class TcpServer
{
    using CallbackType = std::function<void(int)>;
    using TcpSocketPtr = std::unique_ptr<sf::TcpSocket>;

    public:
        // Constructors/setup
        TcpServer();
        TcpServer(unsigned short port);
        TcpServer(unsigned short port, CallbackType callbackC, CallbackType callbackD);
        void setListeningPort(unsigned short port);
        void setConnectedCallback(CallbackType callback);
        void setDisconnectedCallback(CallbackType callback);

        // Communication
        bool send(sf::Packet& packet); // Send to all
        bool send(sf::Packet& packet, int id); // Send to specific client
        bool receive(sf::Packet& packet, int& id); // Returns true if a packet was received
        bool update(); // Accepts new connections and removes disconnected clients
            // Returns true if any new clients connected

        // Clients
        sf::IpAddress getClientAddress(int id) const; // Returns IP address of a client
        void kickClient(int id); // Disconnects a client
        bool clientIsConnected(int id) const; // Checks if a client is connected

    private:
        bool acceptNewClients();
        void removeOldClients();
        int addClient(TcpSocketPtr newClient);
        void removeClient(int id);
        void setupClient(TcpSocketPtr& client);
        void removeClientsToRemove(); // Removes all clients in clientsToRemove list

        sf::TcpListener listener; // Listener for new connections
        CallbackType connectedCallback;
        CallbackType disconnectedCallback;

        std::vector<TcpSocketPtr> clients; // Stores the pointers to the sockets (or clients)
        std::vector<int> clientIds; // Uses this to iterate through the connected clients
        std::vector<int> freeClientIds; // These IDs can be reused for new clients
        std::vector<int> clientsToRemove; // Used to prevent loops from screwing up
        TcpSocketPtr tmpClient; // This is used by the listener
        int clientPos; // The current position in the clientIds vector
};

}

#endif

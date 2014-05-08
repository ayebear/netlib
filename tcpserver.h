// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <SFML/Network.hpp>

namespace net
{

/*
This class acts as a server that manages multiple TCP connections.
It can handle new connections and disconnects, and can even invoke optional callbacks when these events occur.
All clients can be accessed by their unique ID, which is simply an int.
It uses a single separate thread with a socket selector to handle all of the sockets and the listener.
    NOTE: This means that you are limited to:
        63 connections on Windows
        1023 connections on Linux
        1023 connections on Mac OS X
    (The listener takes up a slot in the socket selector)

To use this as a server, you must first set a listening port, then start the thread with start().
To receive packets, or handle new clients connecting/disconnecting, set the callbacks.
Note: If you are modifying the same objects in the callbacks as you are in the main thread, they will
    need to be locked, since they are not running in the same thread. A lock is provided for
    convenience, which is locked before the callback is called. You can obtain this lock by calling
    the getLock() method, which returns a std::unique_lock<std::recursive_mutex>.
For some simple example usage, please refer to the readme.
*/
class TcpServer
{
    using CallbackType = std::function<void(int)>;
    using PacketCallbackType = std::function<void(sf::Packet&, int)>;
    using TcpSocketPtr = std::unique_ptr<sf::TcpSocket>;
    using LockType = std::unique_lock<std::recursive_mutex>;

    public:

        #ifdef _WIN32
            static const unsigned maxConnections = 63;
        #else
            static const unsigned maxConnections = 1023;
        #endif

        // Constructors/setup
        TcpServer();
        TcpServer(unsigned short port);
        TcpServer(unsigned short port, CallbackType c1, CallbackType c2, PacketCallbackType c3);
        ~TcpServer();
        void setListeningPort(unsigned short port);
        void setConnectedCallback(CallbackType callback);
        void setDisconnectedCallback(CallbackType callback);
        void setPacketCallback(PacketCallbackType callback);
        bool setConnectionLimit(unsigned connections = maxConnections);
        void setClientTimeout(float t = 0.0f);

        // Thread synchronization
        LockType getLock();

        // Communication
        bool send(sf::Packet& packet); // Send to all
        bool send(sf::Packet& packet, int id); // Send to specific client
        void start(); // Launches the server loop thread
        void stop(); // Stops the server loop thread
        void join(); // Waits for the server thread to finish running

        // Clients
        sf::IpAddress getClientAddress(int id) const; // Returns IP address of a client
        void kickClient(int id); // Disconnects a client
        bool clientIsConnected(int id) const; // Checks if a client is connected (uses a lock)

    private:

        struct TimedClient
        {
            TcpSocketPtr socket;
            sf::Clock timer;
        };

        using ClientMap = std::map<int, TimedClient>;

        // Main loop for handling connections and receiving data
        void serverLoop();

        // Receives data from a client and removes old clients
        void receive();

        // Clients
        void acceptNewClient();
        int addClient(TcpSocketPtr newClient);
        ClientMap::iterator removeClient(ClientMap::iterator it);
        void setupClient(TcpSocketPtr& client);
        bool clientIsConnected(ClientMap::const_iterator it) const;

        // Callbacks
        CallbackType connectedCallback;
        CallbackType disconnectedCallback;
        PacketCallbackType packetCallback;

        // Main thread
        std::thread serverThread;
        std::atomic_bool running;
        mutable std::recursive_mutex internalMutex;
        std::recursive_mutex callbackMutex;

        // Networking and client management
        sf::SocketSelector selector; // Selector to handle the listener and sockets
        sf::TcpListener listener; // Listener for new connections
        ClientMap clients; // Stores the pointers to the sockets (or clients)
        TcpSocketPtr tmpClient; // This is used by the listener to accept connections
        int lastId; // This is used to generate unique IDs by just incrementing
        bool listenerAdded; // So the listener isn't added more than once
        unsigned connectionLimit; // Maximum number of open sockets
        float timeout; // Time until idle client should be kicked
};

}

#endif

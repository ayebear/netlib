// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include "tcpserver.h"
#include "tcpclient.h"

using std::cout;

class ExampleServer
{
    public:
        ExampleServer();
        void start();
        void handlePacket(sf::Packet& packet, int id);
        void clientConnected(int id);
        void clientDisconnected(int id);
        void createClients();

    private:
        net::TcpServer server;
        bool running;
};

int main()
{
    ExampleServer server;
    server.start();
    return 0;
}

ExampleServer::ExampleServer():
    server(2500) // Create the server and listen on port 2500
{
    cout << "SERVER: Creating server...\n";

    // Set the callbacks
    using std::placeholders::_1;
    server.setConnectedCallback(std::bind(&ExampleServer::clientConnected, this, _1));
    server.setDisconnectedCallback(std::bind(&ExampleServer::clientDisconnected, this, _1));
}

void ExampleServer::start()
{
    // Variables used by the loop
    running = true;
    sf::Packet packet;
    int id = 0;

    // Create a thread to represent some clients
    std::thread clientTestingThread(&ExampleServer::createClients, this);
    clientTestingThread.detach();

    cout << "SERVER: Server is running...\n";
    // This is what a main loop could look like
    while (running)
    {
        server.update();
        if (server.receive(packet, id))
            handlePacket(packet, id);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    cout << "SERVER: Server finished running.\n";
}

void ExampleServer::handlePacket(sf::Packet& packet, int id)
{
    cout << "SERVER: Packet received from client " << id << ".\n";
    std::string str;
    packet >> str;
    cout << "SERVER: Packet contains: \"" << str << "\"\n";
    if (str == "quit")
        running = false;
    else if (str == "kickme")
        server.kickClient(id);
}

void ExampleServer::clientConnected(int id)
{
    cout << "SERVER: Client " << id << " connected.\n";
}

void ExampleServer::clientDisconnected(int id)
{
    cout << "SERVER: Client " << id << " disconnected.\n";
}

void ExampleServer::createClients()
{
    // Just testing some connections...
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cout << "CLIENT: Creating clients...\n";
    net::TcpClient client1;
    net::TcpClient client2;
    net::TcpClient client3;
    //cout << "Client side: client1 connecting...\n";
    client1.connect(sf::IpAddress::LocalHost, 2500);
    //cout << "Client side: client2 connecting...\n";
    client2.connect(sf::IpAddress::LocalHost, 2500);
    //cout << "Client side: client3 connecting...\n";
    client3.connect(sf::IpAddress::LocalHost, 2500);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cout << "CLIENT: Sending test packets...\n";
    sf::Packet packet;
    packet << "testing";
    if (!client1.send(packet))
        cout << "CLIENT: ERROR SENDING PACKET\n";
    client2.send(packet);
    sf::Packet packet2;
    packet2 << "another test";
    client3.send(packet2);

    client1.disconnect();
    client1.connect(sf::IpAddress::LocalHost, 2500);
    net::TcpClient client4;
    client4.connect(sf::IpAddress::LocalHost, 2500);
    sf::Packet packet3;
    packet3 << "quit";
    client4.send(packet);
    client1.send(packet3);

    cout << "CLIENT: Disconnecting clients...\n";
}

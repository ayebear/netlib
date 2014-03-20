// See the file COPYRIGHT.txt for authors and copyright information.
// See the file LICENSE.txt for copying conditions.

#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include "tcpserver.h"
#include "address.h"
#include "packetorganizer.h"

using std::cout;
using std::endl;

class ExampleServer
{
    enum PacketTypes {Msg = 0, Cmd, Test, TotalTypes};

    public:
        ExampleServer();
        void start();
        void handlePacket(sf::Packet& packet, int id);
        void clientConnected(int id);
        void clientDisconnected(int id);

    private:
        // These would normally be on the client-side but are just running in another thread
        // for easy testing.
        void runTests();
        void handleMsg(sf::Packet& packet);
        void handleCmd(sf::Packet& packet);
        void handleTest(sf::Packet& packet);

        net::TcpServer server;
        sf::Clock delay; // Just used to make sure the clients are disconnecting properly
        sf::Clock idleTimer; // Used to switch the server between full-performance and idle mode
        int idleTime; // In milliseconds
        bool running;
};

int main()
{
    ExampleServer server;
    server.start();
    //net::Address someAddress("10.0.0.2", 12345);
    //std::cout << someAddress.toString() << std::endl;
    //someAddress.set("192.168.1.1:443");
    //std::cout << someAddress.toString() << std::endl;
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
    idleTime = 1000;
    running = true;
    sf::Packet packet;
    int id = 0;

    // Create a thread to represent some clients
    std::thread clientTestingThread(&ExampleServer::runTests, this);
    clientTestingThread.detach();

    cout << "SERVER: Server is running...\n";
    // This is what a main loop could look like
    while (running || delay.getElapsedTime().asSeconds() <= 2)
    {
        bool updated = server.update();
        bool received = server.receive(packet, id);
        if (received)
            handlePacket(packet, id);
        // TODO: Find a better way to handle limiting the CPU cycles...
        if (!updated && !received)
        {
            if (idleTimer.getElapsedTime().asMilliseconds() > idleTime)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // else, stay in performance mode and don't sleep any
        }
        else
            idleTimer.restart();
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
    {
        running = false;
        delay.restart();
        cout << "SERVER: Shutting down...\n";
    }
    else if (str == "kickme")
        server.kickClient(id);
}

void ExampleServer::clientConnected(int id)
{
    cout << "SERVER: Client " << id << " connected.\n";
    sf::Packet packet;
    packet << Msg << "Welcome to the Example Server!";
    server.send(packet, id);
    packet.clear();
    packet << Cmd << "sudo make me a sandwich";
    server.send(packet, id);
    server.send(packet, id);
    server.send(packet, id);
    packet.clear();
    packet << Test << 123 << 456;
    server.send(packet, id);
}

void ExampleServer::clientDisconnected(int id)
{
    cout << "SERVER: Client " << id << " disconnected.\n";
}

void ExampleServer::runTests()
{
    // Stress testing
    /*net::TcpClient clients[20];
    for (auto& client: clients)
        client.connect(sf::IpAddress::LocalHost, 2500);
    cout << "CLIENT: Clients done connecting. Will spam in 5 seconds.\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    cout << "CLIENT: SPAMMING 2000 PACKETS!!!\n";
    sf::Packet packet;
    packet << "SPAMMMMMMM";
    for (int i = 0; i < 100; ++i)
        for (auto& client: clients)
            client.send(packet);
    packet.clear();
    packet << "quit";
    clients[0].send(packet);
    cout << "CLIENT: Done spamming.\n";*/

    // Packet organizer testing
    net::PacketOrganizer client;
    client.connect(sf::IpAddress::LocalHost, 2500);
    client.setValidTypeRange(0, TotalTypes);
    sf::Clock connectionTimer;
    std::string str;
    // Register callbacks for packet handling:
    using std::placeholders::_1;
    client.registerCallback(Msg, std::bind(&ExampleServer::handleMsg, this, _1));
    client.registerCallback(Cmd, std::bind(&ExampleServer::handleCmd, this, _1));
    client.registerCallback(Test, std::bind(&ExampleServer::handleTest, this, _1));
    // Client's main loop
    while (client.isConnected() && connectionTimer.getElapsedTime().asSeconds() < 3)
    {
        client.receive();
        client.invokeCallbacks();
    }
    sf::Packet packet;
    packet << "quit";
    client.tcpSend(packet);
}

void ExampleServer::handleMsg(sf::Packet& packet)
{
    std::string str;
    packet >> str;
    cout << "CLIENT: Msg packet received: " << str << endl;
}

void ExampleServer::handleCmd(sf::Packet& packet)
{
    std::string str;
    packet >> str;
    cout << "CLIENT: Cmd packet received: " << str << endl;
}

void ExampleServer::handleTest(sf::Packet& packet)
{
    int x, y;
    packet >> x >> y;
    cout << "CLIENT: Test packet received: " << x << ", " << y << endl;
}

#include "Server.h"

#include <iostream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "GraphicsEngine.h"
#include "Pong.h"
#pragma comment(lib, "ws2_32.lib")

bool Server::isServer;
int Server::currentSocket;
GameState Server::gameState;

const int MAGIC_NUMBER = 123456;

#define PORT 8080

void initWinsock() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        exit(EXIT_FAILURE);
    }
#endif
}

void cleanupWinsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void closeSocket(int socket)
{
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

void SetNonBlocking(int socket) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(socket, FIONBIO, &mode); // Windows: Make socket non-blocking
#else
    int flags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK); // Linux/macOS
#endif
}

void Server::StartServer()
{
    Server::isServer = true;
    initWinsock();

    int serverSocket, clientSocket;
    sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        std::cerr << "Socket creation failed." << std::endl;
        cleanupWinsock();
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind socket
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Bind failed." << std::endl;
        closeSocket(serverSocket);
        cleanupWinsock();
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(serverSocket, 5) < 0)
    {
        std::cerr << "Listen failed." << std::endl;
        closeSocket(serverSocket);
        cleanupWinsock();
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &addrLen);
    if (clientSocket < 0)
    {
        std::cerr << "Accept failed." << std::endl;
        closeSocket(clientSocket);
        cleanupWinsock();
        exit(EXIT_FAILURE);
    }

    Server::currentSocket = clientSocket;
    SetNonBlocking(serverSocket);

    std::cout << "Client connected." << std::endl;

    // Start seperate networking threads for sending and receiving messages
    std::thread receiveThread([clientSocket]
    {
       GameState gameState;
        while (true)
        {
            Server::RecieveUpdate(clientSocket, gameState);

            Pong::Puck2->posY = gameState.paddle2Y;
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Update every 10 ms
        }
    });

    std::thread sendThread([clientSocket]
    {
       GameState gameState;
        while (true)
        {
            gameState.ballX = Pong::Ball->posX;
            gameState.ballY = Pong::Ball->posY;

            gameState.ballVelocityX = Pong::Ball->velocityX;
            gameState.ballVelocityY = Pong::Ball->velocityY;

            // Don't need to send xPos since that doesn't change
            gameState.paddle1Y = Pong::Puck1->posY;

            gameState.player1Score = Pong::player1Score;
            gameState.player2Score = Pong::player2Score;
            
            Server::SendUpdate(clientSocket, gameState);

            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Update every 10 ms
        }
    });

    // Execute the threads to run asynchronously
    receiveThread.detach();
    sendThread.detach();
}

void Server::StartClient(std::string ip)
{
    Server::isServer = false;
    initWinsock();

    int clientSocket;
    sockaddr_in serverAddr;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        std::cerr << "Socket creation failed." << std::endl;
        cleanupWinsock();
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Connection to server failed." << std::endl;
        closeSocket(clientSocket);
        cleanupWinsock();
        exit(EXIT_FAILURE);
    }

    Server::currentSocket = clientSocket;
    SetNonBlocking(clientSocket);

    std::cout << "Connected to server." << std::endl;

    // Start threads
    std::thread receiveThread([clientSocket]
    {
       GameState gameState;
        while (true)
        {
            Server::RecieveUpdate(clientSocket, gameState);

            Pong::Ball->SetPosition(gameState.ballX, gameState.ballY);
            Pong::Ball->SetVelocity(gameState.ballVelocityX, gameState.ballVelocityY);
            Pong::Puck1->posY = gameState.paddle1Y;
            Pong::player1Score = gameState.player1Score;
            Pong::player2Score = gameState.player2Score;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Update every 10 ms
        }
    });

    std::thread sendThread([clientSocket]
    {
       GameState gameState;
        while (true)
        {
            gameState.paddle2Y = Pong::Puck2->posY;
            
            Server::SendUpdate(clientSocket, gameState);

            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Update every 10 ms
        }
    });

    // Execute the threads to run asynchronously
    receiveThread.detach();
    sendThread.detach();
}

void Server::SendUpdate(int clientSocket, const GameState& gameState)
{
    GameState networkState = gameState;

    // The client should not be sending this information
    if (!Server::isServer)
    {
        networkState.ballX = 0;
        networkState.ballY = 0;
        networkState.ballVelocityX = 0;
        networkState.ballVelocityY = 0;
        networkState.paddle1Y = 0;
        networkState.player1Score = 0;
        networkState.player2Score = 0;
    }

    // Covert integer fields to network byte order
    networkState.magicNumber = htonl(MAGIC_NUMBER);
    networkState.ballX = gameState.ballX;
    networkState.ballY = gameState.ballY;
    networkState.ballVelocityX = gameState.ballVelocityX;
    networkState.ballVelocityY = gameState.ballVelocityY;
    networkState.paddle1Y = gameState.paddle1Y;
    networkState.paddle2Y = gameState.paddle2Y;
    networkState.player1Score = htonl(gameState.player1Score);
    networkState.player2Score = htonl(gameState.player2Score);

    size_t totalBytesSent = 0;
    size_t structSize = sizeof(GameState);
    const char* data = reinterpret_cast<const char*>(&networkState);

    while (totalBytesSent < structSize)
    {
        int bytesSent = send(clientSocket, data + totalBytesSent, structSize - totalBytesSent, 0);

        if (bytesSent == SOCKET_ERROR)
        {
            int error = WSAGetLastError();

            std::cerr << "Send failed: " << error << std::endl;
            return;
        }

        totalBytesSent += bytesSent;
    }
}

void Server::RecieveUpdate(int clientSocket, GameState& gameState)
{
    GameState receiveState = {};

    size_t totalBytesReceived = 0;
    size_t structSize = sizeof(GameState);
    char* data = reinterpret_cast<char*>(&receiveState);

    while (totalBytesReceived < structSize)
    {
        int bytesReceived = recv(clientSocket, data + totalBytesReceived, structSize - totalBytesReceived, 0);

        if (bytesReceived == 0)
        {
            std::cerr << "Connection closed." << std::endl;
            return;
        }
        if (bytesReceived < 0)
        {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) return;
            std::cerr << "Receive failed: " << error << std::endl;
            return;
        }
        totalBytesReceived += bytesReceived;
    }

    if (ntohl(receiveState.magicNumber) != MAGIC_NUMBER)
    {
        std::cerr << "Invalid game state received.  Discarding packet. " << ntohl(receiveState.magicNumber) << std::endl;
        return;
    }

    // Apply data
    gameState.ballX = receiveState.ballX;
    gameState.ballY = receiveState.ballY;
    gameState.ballVelocityX = receiveState.ballVelocityX;
    gameState.ballVelocityY = receiveState.ballVelocityY;
    gameState.paddle1Y = receiveState.paddle1Y;
    gameState.paddle2Y = receiveState.paddle2Y;
    gameState.player1Score = ntohl(receiveState.player1Score); // Only do this for integers
    gameState.player2Score = ntohl(receiveState.player2Score);
}

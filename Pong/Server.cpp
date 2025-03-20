#include "Server.h"

#include <thread>
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "Pong.h"
#pragma comment(lib, "ws2_32.lib")

bool Server::isServer;
GameState Server::gameState;
int Server::currentSocket;

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


void Server::StartServer() {
    Server::isServer = true;
    initWinsock();

    int serverSocket, clientSocket;
    sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        cleanupWinsock();
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind socket
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        closeSocket(serverSocket);
        cleanupWinsock();
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Listen failed" << std::endl;
        closeSocket(serverSocket);
        cleanupWinsock();
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &addrLen);
    if (clientSocket < 0) {
        std::cerr << "Client connection failed" << std::endl;
        closeSocket(serverSocket);
        cleanupWinsock();
        exit(EXIT_FAILURE);
    }

    Server::currentSocket = clientSocket;
    SetNonBlocking(clientSocket); // Make the socket non-blocking

    std::cout << "Client connected" << std::endl;

    // Start separate networking threads
    std::thread receiveThread([clientSocket]() {
        GameState gameState;
        while (true) {
            Server::RecieveUpdate(clientSocket, gameState);

            Pong::Puck2->posY = gameState.paddle2Y;
        }
    });

    std::thread sendThread([clientSocket]() {
        GameState gameState;
        while (true) {

            gameState.ballX = Pong::Ball->posX;
            gameState.ballY = Pong::Ball->posY;

            gameState.ballVelocityX = Pong::Ball->velocityX;
            gameState.ballVelocityY = Pong::Ball->velocityY;

            gameState.paddle1Y = Pong::Puck1->posY;

            gameState.player1Score = Pong::player1Score;
            gameState.player2Score = Pong::player2Score;
            
            Server::SendUpdate(clientSocket, gameState);
        }
    });

    // Detach threads so they run independently
    receiveThread.detach();
    sendThread.detach();

    // Do NOT block here—allow the main thread to run `RunGame()`
}



void Server::StartClient(const std::string& serverIP) {
    Server::isServer = false;
    initWinsock();

    int clientSocket;
    sockaddr_in serverAddr;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        cleanupWinsock();
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection to server failed" << std::endl;
        closeSocket(clientSocket);
        cleanupWinsock();
        exit(EXIT_FAILURE);
    }

    Server::currentSocket = clientSocket;
    SetNonBlocking(clientSocket); // Make the socket non-blocking

    std::cout << "Connected to server" << std::endl;

    // Start networking threads
    std::thread receiveThread([clientSocket]() {
        GameState gameState;
        while (true) {
            Server::RecieveUpdate(clientSocket, gameState);

            Pong::Ball->SetPosition(gameState.ballX, gameState.ballY);
            Pong::Ball->SetVelocity(gameState.ballVelocityX, gameState.ballVelocityY);
            Pong::Puck1->posY = gameState.paddle1Y;
            Pong::player1Score = gameState.player1Score;
            Pong::player2Score = gameState.player2Score;
        }
    });

    std::thread sendThread([clientSocket]() {
        GameState gameState;
        while (true) {
        
            gameState.paddle2Y = Pong::Puck2->posY;
            
            Server::SendUpdate(clientSocket, gameState);
        }
    });

    // Detach threads so they run independently
    receiveThread.detach();
    sendThread.detach();
}



void Server::SendUpdate(int socket, const GameState& state) {
    GameState networkState = state; // Copy state before modifying

    if (!Server::isServer) {
        // Client only sends paddle1Y (its own paddle)
        networkState.ballX = 0;
        networkState.ballY = 0;
        networkState.ballVelocityX = 0;
        networkState.ballVelocityY = 0;
        networkState.paddle1Y = 0; // Client does not control opponent paddle
        networkState.player1Score = 0;
        networkState.player2Score = 0;
    }

    // Convert integer fields to network byte order
    networkState.ballX = htonl(networkState.ballX);
    networkState.ballY = htonl(networkState.ballY);
    networkState.paddle1Y = htonl(networkState.paddle1Y);
    networkState.paddle2Y = htonl(networkState.paddle2Y);
    networkState.ballVelocityX = networkState.ballVelocityX;
    networkState.ballVelocityY = networkState.ballVelocityY;
    networkState.player1Score = htonl(networkState.player1Score);
    networkState.player2Score = htonl(networkState.player2Score);


    // Ensure full struct is sent
    size_t totalBytesSent = 0;
    size_t structSize = sizeof(GameState);
    const char* data = reinterpret_cast<const char*>(&networkState);

    while (totalBytesSent < structSize) {
        int bytesSent = send(socket, data + totalBytesSent, structSize - totalBytesSent, 0);
        
        if (bytesSent == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                // Non-blocking mode: wait a bit and retry
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            } else {
                std::cerr << "Send failed: " << error << std::endl;
                return;
            }
        }
        
        totalBytesSent += bytesSent;
    }
}
void Server::RecieveUpdate(int socket, GameState& state) {
    GameState receivedState = {};

    size_t totalBytesReceived = 0;
    size_t structSize = sizeof(GameState);
    char* data = reinterpret_cast<char*>(&receivedState);

    while (totalBytesReceived < structSize) {
        int bytesReceived = recv(socket, data + totalBytesReceived, structSize - totalBytesReceived, 0);

        if (bytesReceived == 0) {
            std::cerr << "Connection closed." << std::endl;
            return;
        }
        if (bytesReceived < 0) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) return; // No data available, just return

            std::cerr << "Receive failed: " << error << std::endl;
            return;
        }
        totalBytesReceived += bytesReceived;
    }

    // Convert integer fields back to host byte order
    state.ballX = ntohl(receivedState.ballX);
    state.ballY = ntohl(receivedState.ballY);
    state.paddle1Y = ntohl(receivedState.paddle1Y);
    state.paddle2Y = ntohl(receivedState.paddle2Y);
    state.ballVelocityX = receivedState.ballVelocityX;
    state.ballVelocityY = receivedState.ballVelocityY;
    state.player1Score = ntohl(receivedState.player1Score);
    state.player2Score = ntohl(receivedState.player2Score);
}
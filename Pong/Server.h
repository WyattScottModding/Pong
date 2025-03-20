#pragma once

#include <string>


#pragma pack(1) // Disable padding
struct GameState {
    int ballX;
    int ballY;
    float ballVelocityX;
    float ballVelocityY;
    int paddle1Y;
    int paddle2Y;
    int player1Score;
    int player2Score;
};
#pragma pack() // Reset default alignment



class Server
{
public:
    static bool isServer;
    static GameState gameState;
    static int currentSocket;
public:
    static void StartServer();
    static void StartClient(const std::string& serverIP);
    static void SendUpdate(int clientSocket, const GameState& state);
    static void RecieveUpdate(int clientSocket, GameState& state);
};

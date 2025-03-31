#pragma once
#include <string>

#pragma pack(1) // Disable padding
struct GameState
{
    int magicNumber = 123456; // For determining bad packets
    float ballX;
    float ballY;
    float ballVelocityX;
    float ballVelocityY;
    float paddle1Y;
    float paddle1X;
    float paddle2Y;
    float paddle2X;
    int player1Score;
    int player2Score;
};
#pragma pack()

class Server
{
public:
    static bool isServer;
    static int currentSocket;
    static GameState gameState;
public:
    static void StartServer();
    static void StartClient(std::string ip);

    static void SendUpdate(int clientSocket, const GameState& gameState);
    static void RecieveUpdate(int clientSocket, GameState& gameState);
};

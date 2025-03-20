#include "Pong.h"

#include <algorithm>

#include "GraphicsEngine.h"
#include <chrono>
#include <thread>
#include <cmath>
#include <iostream>
#include <ostream>

#include "Server.h"


using namespace std;
using namespace std::chrono;

Object *Pong::Puck1;
Object *Pong::Puck2;
Object *Pong::Ball;

float Pong::GameSpeed = 0.3f;

bool Pong::upPressed = false;
bool Pong::downPressed = false;
bool Pong::wPressed = false;
bool Pong::sPressed = false;

int Pong::player1Score = 0;
int Pong::player2Score = 0;

int main(int argc, char* argv[])
{
    char choice;
    std::cout << "Host (H) or Join (J)? ";
    std::cin >> choice;
    
    if (choice == 'H' || choice == 'h') {
        // Start as server
        Server::StartServer();
    } else if (choice == 'J' || choice == 'j') {
        // Start as client
        std::string serverIP = "";
        std::cout << "Enter server IP: ";
        std::cin >> serverIP;
        
        Server::StartClient(serverIP);
    } else {
        std::cout << "Invalid choice." << std::endl;
    }
    
    
    srand(time(NULL));
    
    Pong::InitGame();
    Pong::RunGame();
    
    return 0;
}

void Pong::InitGame()
{
    Pong::Puck1 = new Object(10, 50);
    Pong::Puck1->SetPosition(100, GraphicsEngine::ScreenSize/2);
    
    Pong::Puck2 = new Object(10, 50);
    Pong::Puck2->SetPosition(GraphicsEngine::ScreenSize - 100, GraphicsEngine::ScreenSize/2);
    
    Pong::Ball = new Object(10, 10);
    ResetBall();

    GraphicsEngine::CreateScreen();
}

void Pong::RunGame()
{
    const std::chrono::milliseconds frameDuration(1000 / 60); // 60 FPS
    
    int socket = Server::currentSocket; // Use the server or client socket

    while (true)
    {
        auto frame_start = std::chrono::high_resolution_clock::now();
        
        MovePaddle();
        
        // Update game logic only on server
        if (Server::isServer)
        {
            MoveBall();
        }

        if (!GraphicsEngine::DrawScreen()) break;

        auto frame_end = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds frame_time = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start);

        std::chrono::milliseconds sleep_time = frameDuration - frame_time;
        if (sleep_time > std::chrono::milliseconds(0))
        {
            std::this_thread::sleep_for(sleep_time);
        }
    }
}


void Pong::MovePaddle()
{
    const int speed = 15;
    if (wPressed) Pong::Puck1->posY = clamp(Pong::Puck1->posY - speed, 50, GraphicsEngine::ScreenSize - 90);
    if (sPressed) Pong::Puck1->posY = clamp(Pong::Puck1->posY + speed, 50, GraphicsEngine::ScreenSize - 90);
    if (upPressed) Pong::Puck2->posY = clamp(Pong::Puck2->posY - speed, 50, GraphicsEngine::ScreenSize - 90);
    if (downPressed) Pong::Puck2->posY = clamp(Pong::Puck2->posY + speed, 50, GraphicsEngine::ScreenSize - 90);
}


// Function to maintain consistent ball speed
void NormalizeVelocity(Object* ball, float speed) {
    float magnitude = sqrt(ball->velocityX * ball->velocityX + ball->velocityY * ball->velocityY);
    if (magnitude != 0) {
        ball->velocityX = (ball->velocityX / magnitude) * speed;
        ball->velocityY = (ball->velocityY / magnitude) * speed;
    }
}

void Pong::MoveBall()
{
    Ball->UpdatePosition();

    float ballSpeed = sqrt(Ball->velocityX * Ball->velocityX + Ball->velocityY * Ball->velocityY);

    // Colliding with Puck1
    if (Puck1->IsColliding(Ball)) {
        Ball->posX = Puck1->posX + Puck1->width * 2;

        float paddleCenter = Puck1->posY;
        float hitPosition = (Ball->posY - paddleCenter) / Puck1->height;

        // Bounce angle based on hit position
        float bounceAngle = hitPosition * (45.0f * (3.14159f / 180.0f));

        Ball->velocityX = cos(bounceAngle) * ballSpeed;
        Ball->velocityY = sin(bounceAngle) * ballSpeed;

        // Add paddle motion influence
        Ball->velocityX += Puck1->velocityX * 0.2f;

        // Normalize to keep constant speed
        NormalizeVelocity(Ball, ballSpeed);
    }

    // Colliding with Puck2
    if (Puck2->IsColliding(Ball)) {
        Ball->posX = Puck2->posX - Puck2->width * 2;

        float paddleCenter = Puck2->posY;
        float hitPosition = (Ball->posY - paddleCenter) / Puck2->height;

        float bounceAngle = hitPosition * (45.0f * (3.14159f / 180.0f));

        Ball->velocityX = -cos(bounceAngle) * ballSpeed;
        Ball->velocityY = sin(bounceAngle) * ballSpeed;

        Ball->velocityX += Puck2->velocityX * 0.2f;

        // Normalize to maintain consistent speed
        NormalizeVelocity(Ball, ballSpeed);
    }

    // Get window dimensions correctly
    RECT windowRect;
    RECT clientRect;
    GetWindowRect(GraphicsEngine::hwnd, &windowRect);
    GetClientRect(GraphicsEngine::hwnd, &clientRect);

    int borderWidth = (windowRect.right - windowRect.left) - clientRect.right;
    int borderHeight = (windowRect.bottom - windowRect.top) - clientRect.bottom;

    int topOffset = borderHeight - (windowRect.bottom - windowRect.top - clientRect.bottom);

    // Vertical bounce (top and bottom)
    if (Ball->posY - Ball->height < topOffset) {
        Ball->posY = topOffset + Ball->height;
        Ball->velocityY = -Ball->velocityY;
    }

    if (Ball->posY + Ball->height > clientRect.bottom) {
        Ball->posY = clientRect.bottom - Ball->height;
        Ball->velocityY = -Ball->velocityY;
    }

    // Horizontal bounce (left and right)
    if (Ball->posX - Ball->width < borderWidth / 2) {
        Ball->posX = borderWidth / 2 + Ball->width;
        Ball->velocityX = -Ball->velocityX;

        Pong::player2Score++;
        Pong::ResetBall();
    }

    if (Ball->posX + Ball->width > clientRect.right) {
        Ball->posX = clientRect.right - Ball->width;
        Ball->velocityX = -Ball->velocityX;

        Pong::player1Score++;
        Pong::ResetBall();
    }
}

void Pong::ResetBall()
{
    Pong::Ball->SetPosition(GraphicsEngine::ScreenSize/2, GraphicsEngine::ScreenSize/2);

    float speed = log(Pong::player2Score + Pong::player1Score + 10);
    
    // Randomize X velocity (either -1 or 1) multiplied by a base speed
    float velocityX = (rand() % 2 == 0 ? 1 : -1) * (3.0f + (rand() % 3)); // Random between 3-5

    // Randomize Y velocity (between -2 and 2 but not 0)
    float velocityY = (rand() % 5 - 2); // -2 to 2

    // Ensure Y is never 0
    if (velocityY == 0) velocityY = (rand() % 2 == 0 ? 1 : -1);

    // Scale the velocity by the calculated speed
    float scaleFactor = speed / sqrt(velocityX * velocityX + velocityY * velocityY);
    velocityX *= scaleFactor;
    velocityY *= scaleFactor;

    // Apply velocity
    Pong::Ball->SetVelocity(velocityX, velocityY);
}

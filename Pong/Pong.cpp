#include "Pong.h"

#include <chrono>
#include <cmath>
#include <thread>
#include <algorithm>
#include <iostream>

#include "GraphicsEngine.h"
#include "Server.h"

Object *Pong::Puck1;
Object *Pong::Puck2;
Object *Pong::Ball;

int Pong::player1Score = 0;
int Pong::player2Score = 0;
int Pong::EndScore = 20;

bool Pong::upPressed = false;
bool Pong::downPressed = false;

float Pong::GameSpeed = 0.2f;

int main(int argc, char* argv[])
{
    char choice;
    std::cout << "Host (H) or Join (J)? ";
    std::cin >> choice;

    if (choice == 'H' || choice == 'h')
    {
        std::cout << "Enter the speed of the game (0.05 - 0.4) ";
        std::cin >> Pong::GameSpeed;

        Server::StartServer();
    }
    else if (choice == 'J' || choice == 'j')
    {
        std::string serverIP = "";
        std::cout << "Enter server IP: ";
        std::cin >> serverIP;

        Server::StartClient(serverIP);
    }
    else
    {
        std::cerr << "Please enter a valid choice." << std::endl;
    }

    // Sets a random seed for the random variables
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
    Pong::ResetBall();
    
    GraphicsEngine::CreateScreen();
}

void Pong::RunGame()
{
    const std::chrono::milliseconds frameDuration(1000/60);

    auto startTime = std::chrono::high_resolution_clock::now();
    const std::chrono::seconds startTimer(5); // 5 seconds before the game starts
    
    
    while (true)
    {
        auto frameStart = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(frameStart - startTime);
        int remainingTime = -1;
        bool gameOver = Pong::player1Score >= EndScore || Pong::player2Score >= EndScore;

        if (elapsedTime < startTimer)
        {
            remainingTime = (startTimer - elapsedTime).count();
        }
        else if (!gameOver && Server::isServer) // Only move the ball on the server side
        {
            MoveBall();
        }

        if (!gameOver) // Can move the paddle if the game isn't over
            MovePaddle();
        
        
        if (!GraphicsEngine::DrawScreen(remainingTime)) break;

        auto frame_end = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds frame_time = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - startTime);

        std::chrono::milliseconds sleep_time = frameDuration - frame_time;
        if (sleep_time > std::chrono::milliseconds(0))
        {
            std::this_thread::sleep_for(sleep_time);
        }
    }
}

void Pong::MoveBall()
{
    Ball->UpdatePosition();

    float ballSpeed = sqrt(Ball->velocityX * Ball->velocityX + Ball->velocityY * Ball->velocityY);

    // Colliding with puck 1
    if (Puck1->IsColliding(Ball))
    {
        Ball->posX = Puck1->posX + Puck1->width * 2;

        float paddleCenter = Puck1->posY;
        float hitPosition = (Ball->posY - paddleCenter) / Puck1->height;

        float bounceAngle = hitPosition * (45.0f * (3.14159f / 180.0f));

        Ball->velocityX = cos(bounceAngle) * ballSpeed;
        Ball->velocityY = sin(bounceAngle) * ballSpeed;

        // Puck movement affects ball movement
        Ball->velocityX += Puck1->velocityX * 0.2f;

        NormalizeVelocity(Ball, ballSpeed);
    }

    // Colliding with puck 2
    if (Puck2->IsColliding(Ball))
    {
        Ball->posX = Puck2->posX - Puck2->width * 2;

        float paddleCenter = Puck2->posY;
        float hitPosition = (Ball->posY - paddleCenter) / Puck2->height;

        float bounceAngle = hitPosition * (45.0f * (3.14159f / 180.0f));

        Ball->velocityX = -cos(bounceAngle) * ballSpeed;
        Ball->velocityY = sin(bounceAngle) * ballSpeed;

        // Puck movement affects ball movement
        Ball->velocityX += Puck2->velocityX * 0.2f;

        NormalizeVelocity(Ball, ballSpeed);
    }

    // Colliding with the window
    RECT windowRect;
    RECT clientRect;
    GetWindowRect(GraphicsEngine::hwnd, &windowRect);
    GetClientRect(GraphicsEngine::hwnd, &clientRect);

    int borderWidth = (windowRect.right - windowRect.left) - clientRect.right;
    int borderHeight = (windowRect.bottom - windowRect.top) - clientRect.bottom;

    int topOffset = borderHeight - (windowRect.bottom - windowRect.top - clientRect.bottom);

    // Vertical Bounce
    if (Ball->posY - Ball->height < topOffset)
    {
        Ball->posY = topOffset + Ball->height;
        Ball->velocityY = -Ball->velocityY;
    }
    
    if (Ball->posY - Ball->height > clientRect.bottom)
    {
        Ball->posY = clientRect.bottom - Ball->height;
        Ball->velocityY = -Ball->velocityY;
    }

    // Horizontal Bounce (Executes a score and resets the ball)
    if (Ball->posX - Ball->width < borderWidth / 2)
    {
        Ball->posX = borderWidth / 2 - Ball->width;
        Ball->velocityX = -Ball->velocityX;

        Pong::ResetBall();
        player2Score++;
    }

    if (Ball->posX + Ball->width > clientRect.right)
    {
        Ball->posX = clientRect.right - Ball->width;
        Ball->velocityX = -Ball->velocityX;

        Pong::ResetBall();
        player1Score++;
    }
}

void Pong::ResetBall()
{
    Pong::Ball->SetPosition(GraphicsEngine::ScreenSize/2, GraphicsEngine::ScreenSize/2);

    float speed = log(player1Score + player2Score + 10) * Pong::GameSpeed; // Speed goes up as you score more

    // Random X and Y velocity
    float angle = (static_cast<float>(rand()) / RAND_MAX) * 90.0f - 45.0f;

    if (rand() % 2 == 0) angle += 180.0f;

    float radians = angle * (3.14159f / 180.0f);

    Pong::Ball->SetVelocity(cos(radians), sin(radians));
    NormalizeVelocity(Pong::Ball, speed);
}

void Pong::MovePaddle()
{
    // Paddle speed
    const float speed = 0.4f;

    if (Server::isServer)
    {
        if (upPressed) Pong::Puck1->posY = std::clamp(Puck1->posY - speed, 50.0f, GraphicsEngine::ScreenSize - 90.0f);
        if (downPressed) Pong::Puck1->posY = std::clamp(Puck1->posY + speed, 50.0f, GraphicsEngine::ScreenSize - 90.0f);
    }
    else
    {
        if (upPressed) Pong::Puck2->posY = std::clamp(Puck2->posY - speed, 50.0f, GraphicsEngine::ScreenSize - 90.0f);
        if (downPressed) Pong::Puck2->posY = std::clamp(Puck2->posY + speed, 50.0f, GraphicsEngine::ScreenSize - 90.0f);
    }
}

void Pong::NormalizeVelocity(Object* obj, float velocity)
{
    float magnitude = sqrt(obj->velocityX * obj->velocityX + obj->velocityY * obj->velocityY);

    if (magnitude > 0.0f)
    {
        obj->velocityX = (obj->velocityX / magnitude) * velocity;
        obj->velocityY = (obj->velocityY / magnitude) * velocity;
    }
}

#pragma once
#include "Object.h"


class Pong
{
public:
    static Object *Puck1;
    static Object *Puck2;
    static Object *Ball;

    static int player1Score;
    static int player2Score;
    static int EndScore;

    static bool upPressed;
    static bool downPressed;

    static float GameSpeed;
public:
    static void InitGame();
    static void RunGame();

    static void MoveBall();
    static void ResetBall();
    static void MovePaddle();

    static void NormalizeVelocity(Object* obj, float velocity);
};

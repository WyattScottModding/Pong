#pragma once

#include "Object.h"

class Pong
{
public:
    static float GameSpeed;

    static Object *Puck1;
    static Object *Puck2;
    static Object *Ball;

    static bool upPressed;
    static bool downPressed;
    static bool wPressed;
    static bool sPressed;

    static int player1Score;
    static int player2Score;
public:
    static void InitGame();
    static void RunGame();
    static void MovePaddle();
    static void MoveBall();
    static void ResetBall();
};

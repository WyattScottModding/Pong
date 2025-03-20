#pragma once
#include <chrono>

class Object
{
public:
    int posX;
    int posY;

    float velocityX;
    float velocityY;

    int width;
    int height;

    std::chrono::steady_clock::time_point lastCollisionTime;

public:
    Object();
    Object(int width, int height);

    void SetPosition(int x, int y);
    void SetVelocity(float x, float y);

    void UpdatePosition();

    bool IsColliding(Object* a);
};

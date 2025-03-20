#include "Object.h"

Object::Object()
{
    this->width = 0;
    this->height = 0;

    this->posX = 0;
    this->posY = 0;

    this->velocityX = 0;
    this->velocityY = 0;
}

Object::Object(int width, int height)
{
    this->width = width;
    this->height = height;

    this->posX = 0;
    this->posY = 0;

    this->velocityX = 0;
    this->velocityY = 0;
}

void Object::SetPosition(int x, int y)
{
    this->posX = x;
    this->posY = y;
}

void Object::SetVelocity(float x, float y)
{
    this->velocityX = x;
    this->velocityY = y;
}

void Object::UpdatePosition()
{
    this->posX += this->velocityX;
    this->posY += this->velocityY;
}

bool Object::IsColliding(Object* a) {
    
    // Cooldown in milliseconds
    const int COLLISION_COOLDOWN_MS = 100; 

    // Get the current time
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastCollision = std::chrono::duration_cast<std::chrono::milliseconds>(now - a->lastCollisionTime).count();

    // Check cooldown and bounding box collision
    if (timeSinceLastCollision > COLLISION_COOLDOWN_MS &&
        a->posX - a->width < this->posX + this->width &&
        a->posX + a->width > this->posX - this->width &&
        a->posY - a->height < this->posY + this->height &&
        a->posY + a->height > this->posY - this->height) {

        // Register the collision and reset the timer
        a->lastCollisionTime = now;
        return true;
    }

    return false;
}

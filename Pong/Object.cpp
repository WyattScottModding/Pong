#include "Object.h"

#include <chrono>

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

void Object::SetPosition(float x, float y)
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

bool Object::IsColliding(Object* other)
{
    // Cooldown in milliseconds
    const int COLLISION_COOLDOWN_MS = 100;

    auto now = std::chrono::high_resolution_clock::now();
    auto timeSinceLastCollision = std::chrono::duration_cast<std::chrono::milliseconds>(now - other->lastCollisionTime).count();

    // Check if there is a collision
    if (timeSinceLastCollision > COLLISION_COOLDOWN_MS &&
        other->posX - other->width < this->posX + this->width &&
        other->posY - other->height < this->posY + this->height &&
        other->posX + other->width > this->posX - this->width &&
        other->posY + other->height > this->posY - this->height)
    {
        other->lastCollisionTime = now;
        return true;
    }
    
    return false;
}

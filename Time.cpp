#include "Time.h"

float Time::deltaTime = 0.0f;
float Time::time = 0.0f;

void Time::Update(float dt) {
    deltaTime = dt;
    time += dt;
}

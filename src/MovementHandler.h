#ifndef MOVEMENTHANDLER_H
#define MOVEMENTHANDLER_H

#pragma once

#include <chrono>
#include <simd/simd.h>
#include <unordered_map>

class MovementHandler {
public:
    MovementHandler();

    void keyDown(uint16_t key);

    void keyUp(uint16_t key);

    void mouseMove(double dx, double dy);

    std::chrono::high_resolution_clock::time_point lastInteraction{std::chrono::high_resolution_clock::now()};

    // Call once per frame with elapsed seconds
    void update(float dt);

    bool hasMoved() const {
        return _moved;
    }

    void resetMoved() {
        _moved = false;
    }

    // Accessors for camera state
    simd::float3 position() const { return _camPos; }
    float yaw() const { return _yaw; }
    float pitch() const { return _pitch; }

private:
    std::unordered_map<uint16_t, bool> _keys;
    simd::float3 _camPos;
    float _yaw, _pitch;
    simd::float3 _velocity;
    bool _moved = false;

    // tuning parameters
    const float _accel = 50.0f; // world‐units per second²
    const float _damp = 8.0f; // damping per second
    const float _sens = 0.002f; // radians per pixel

    static constexpr auto kInteractionCooldown = std::chrono::milliseconds(100);
};


#endif //MOVEMENTHANDLER_H

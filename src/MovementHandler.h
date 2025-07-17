#ifndef MOVEMENTHANDLER_H
#define MOVEMENTHANDLER_H

#pragma once

#include <chrono>
#include <simd/simd.h>
#include <unordered_map>
#include <mutex>

class MovementHandler {
public:
    MovementHandler();

    // Call from input/event thread:
    void keyDown(uint16_t key);

    void keyUp(uint16_t key);

    void mouseMove(double dx, double dy);

    // Reset camera to initial pose (thread-safe)
    void resetCamera();

    // Threaded updater: call periodically with elapsed seconds
    void update(float dt);

    // Thread-safe accessors for camera state
    simd::float3 getPosition();

    float getYaw();

    float getPitch();

    // Check if camera moved since last read, then clear flag
    bool hasMovedAndClear();

    std::chrono::high_resolution_clock::time_point lastInteraction;

private:
    mutable std::mutex _mtx;
    simd::float3 _camPos;
    float _yaw;
    float _pitch;
    simd::float3 _velocity;
    std::unordered_map<uint16_t, bool> _keys;
    bool _moved = false;

    // tuning parameters
    static constexpr float _accel = 50.0f; // world-units per second^2
    static constexpr float _damp = 8.0f; // damping per second
    static constexpr float _sens = 0.002f; // radians per pixel

    static constexpr auto kInteractionCooldown = std::chrono::milliseconds(100);
};

#endif // MOVEMENTHANDLER_H

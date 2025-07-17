#include "MovementHandler.h"
#include <cmath>
#include <numbers>

static constexpr float PI_2 = 1.57079632679f;

MovementHandler::MovementHandler()
    : _camPos{-2, 3, 6}, _yaw(std::numbers::pi_v<float> * 11 / 12), _pitch(-std::numbers::pi_v<float> * 1 / 12),
      _velocity{0, 0, 0} {
}

void MovementHandler::keyDown(const uint16_t key) {
    _keys[key] = true;
    _moved = true;
}

void MovementHandler::keyUp(const uint16_t key) {
    _keys[key] = false;
    _moved = true;
}

void MovementHandler::mouseMove(const double dx, const double dy) {
    _yaw -= static_cast<float>(dx) * _sens;
    _pitch -= static_cast<float>(dy) * _sens;
    // clamp pitch to avoid gimbal
    if (_pitch > PI_2 - 0.01f) _pitch = PI_2 - 0.01f;
    if (_pitch < -PI_2 + 0.01f) _pitch = -PI_2 + 0.01f;
    _moved = true;
    lastInteraction = std::chrono::high_resolution_clock::now();
}

void MovementHandler::update(float dt) {
    // build forward/right in XZ plane
    simd::float3 forward = simd::normalize(simd::float3{
        std::cos(_pitch) * std::sin(_yaw),
        0,
        std::cos(_pitch) * std::cos(_yaw)
    });

    const auto worldUp = simd::float3{0, 1, 0};

    simd::float3 right = simd::normalize(simd::cross(forward, worldUp));

    // accumulate input acceleration
    simd::float3 accel{0};
    if (_keys['w']) accel += forward;
    if (_keys['s']) accel -= forward;
    if (_keys['a']) accel -= right;
    if (_keys['d']) accel += right;
    if (_keys[' ']) accel += worldUp;
    if (_keys['c']) accel -= worldUp;

    // integrate velocity & position
    _velocity += accel * (_accel * dt);
    _camPos += _velocity * dt;

    // apply damping: v *= exp(-damp * dt)
    float f = std::exp(-_damp * dt);
    _velocity *= f;

    // if the velocity isn't 0, set moved to true
    if (simd::length(_velocity) > 1e-2f) {
        _moved = true;
    }
}


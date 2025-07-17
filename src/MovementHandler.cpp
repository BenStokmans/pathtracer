#include "MovementHandler.h"
#include <cmath>
#include <numbers>
#include <print>

static constexpr float PI_2 = 1.57079632679f;

MovementHandler::MovementHandler()
    : _camPos{-2, 3, 6},
      _yaw(std::numbers::pi_v<float> * 11 / 12),
      _pitch(-std::numbers::pi_v<float> * 1 / 12),
      _velocity{0, 0, 0} {
}

void MovementHandler::keyDown(uint16_t key) {
    std::lock_guard lg(_mtx);
    _keys[key] = true;
    _moved = true;
}

void MovementHandler::keyUp(uint16_t key) {
    std::lock_guard lg(_mtx);
    _keys[key] = false;
    _moved = true;
}

void MovementHandler::mouseMove(double dx, double dy) {
    std::lock_guard lg(_mtx);
    _yaw -= float(dx) * _sens;
    _pitch -= float(dy) * _sens;
    constexpr float eps = 0.01f;
    _pitch = std::clamp(_pitch, -PI_2 + eps, PI_2 - eps);
    _moved = true;
    lastInteraction = std::chrono::high_resolution_clock::now();
}

void MovementHandler::update(float dt) {
    std::lock_guard lg(_mtx);
    // build frame axes
    simd::float3 forward = simd::normalize(simd::float3{
        std::cos(_pitch) * std::sin(_yaw), 0, std::cos(_pitch) * std::cos(_yaw)
    });
    simd::float3 worldUp = {0, 1, 0};
    simd::float3 right = simd::normalize(simd::cross(forward, worldUp));

    simd::float3 accel{0};
    if (_keys['w']) accel += forward;
    if (_keys['s']) accel -= forward;
    if (_keys['a']) accel -= right;
    if (_keys['d']) accel += right;
    if (_keys[' ']) accel += simd::float3{0, 1, 0};
    if (_keys['c']) accel -= simd::float3{0, 1, 0};

    _velocity += accel * (_accel * dt);
    _camPos += _velocity * dt;
    // damping
    _velocity *= std::exp(-_damp * dt);

    if (simd::length(_velocity) > 1e-2f) _moved = true;
}

simd::float3 MovementHandler::getPosition() {
    std::lock_guard lg(_mtx);
    return _camPos;
}

float MovementHandler::getYaw() {
    std::lock_guard lg(_mtx);
    return _yaw;
}

float MovementHandler::getPitch() {
    std::lock_guard lg(_mtx);
    return _pitch;
}

bool MovementHandler::hasMovedAndClear() {
    std::lock_guard lg(_mtx);
    bool m = _moved;
    _moved = false;
    return m;
}

void MovementHandler::resetCamera() {
    std::lock_guard lg(_mtx);
    _camPos = {-2, 3, 6};
    _yaw = std::numbers::pi_v<float> * 11 / 12;
    _pitch = -std::numbers::pi_v<float> * 1 / 12;
    _velocity = {0, 0, 0};
    _moved = true;
}

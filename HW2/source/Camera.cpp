#include "Camera.h"

// Default constructor: initializes the camera at (0,0,50), looking at the origin, with Y-up.
Camera::Camera()
    : position(0.0f, 0.0f, 50.0f),
      target(0.0f, 0.0f, 0.0f),
      up(0.0f, 1.0f, 0.0f)
{
    updateView();
    // Set a default perspective (fov in radians, aspect ratio, near and far planes)
    setPerspective(40 * 3.14f / 180.0f, 800.0f / 600.0f, 2.0f, 1000.0f);
}

// Parameterized constructor.
Camera::Camera(const cy::Vec3f& pos, const cy::Vec3f& target, const cy::Vec3f& up)
    : position(pos), target(target), up(up)
{
    updateView();
    setPerspective(40 * 3.14f / 180.0f, 800.0f / 600.0f, 2.0f, 1000.0f);
}

void Camera::setPosition(const cy::Vec3f& pos) {
    position = pos;
    updateView();
}

void Camera::setTarget(const cy::Vec3f& target) {
    this->target = target;
    updateView();
}

void Camera::setUp(const cy::Vec3f& up) {
    this->up = up;
    updateView();
}

void Camera::setPerspective(float fov, float aspect, float nearPlane, float farPlane) {
    projectionMatrix = cy::Matrix4f::Perspective(fov, aspect, nearPlane, farPlane);
}

void Camera::updateView() {
    viewMatrix = cy::Matrix4f::View(position, target, up);
}

const cy::Matrix4f& Camera::getViewMatrix() const {
    return viewMatrix;
}

const cy::Matrix4f& Camera::getProjectionMatrix() const {
    return projectionMatrix;
}

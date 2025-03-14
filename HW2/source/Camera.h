#ifndef CAMERA_H
#define CAMERA_H

#include "cyMatrix.h"
#include "cyVector.h"

class Camera {
public:
    // Constructors
    Camera();
    Camera(const cy::Vec3f& position, const cy::Vec3f& target, const cy::Vec3f& up);

    // Set camera parameters
    void setPosition(const cy::Vec3f& pos);
    void setTarget(const cy::Vec3f& target);
    void setUp(const cy::Vec3f& up);
    void setPerspective(float fov, float aspect, float nearPlane, float farPlane);

    // Update view matrix based on current position, target, and up vector.
    void updateView();

    // Getters for matrices
    const cy::Matrix4f& getViewMatrix() const;
    const cy::Matrix4f& getProjectionMatrix() const;

private:
    cy::Vec3f position;
    cy::Vec3f target;
    cy::Vec3f up;

    cy::Matrix4f viewMatrix;
    cy::Matrix4f projectionMatrix;
};

#endif // CAMERA_H

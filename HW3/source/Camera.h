#ifndef CAMERA_H
#define CAMERA_H

#include "cyMatrix.h"
#include "cyVector.h"
#include "Util.h"
#include <chrono>

class Camera {
    public:
        Camera(cy::Vec3f worldSpacePosition) : position(worldSpacePosition), front(0.0f, 0.0f, -1.0f), up(0.0f, 1.0f, 0.0f), 
        yaw(-90.0f), pitch(0.0f), movementSpeed(2.5f), mouseSensitivity(0.05f),
        perspectiveMatrix(cy::Matrix4f(1.0)) {
            update();
        }
    
        void update() {
            front.x = cos(Util::degreesToRadians(yaw)) * cos(Util::degreesToRadians(pitch));
            front.y = sin(Util::degreesToRadians(pitch));
            front.z = sin(Util::degreesToRadians(yaw)) * cos(Util::degreesToRadians(pitch));
            front.Normalize();
    
            right = front.Cross(up);
            right.Normalize();
            upVector = right.Cross(front);
            upVector.Normalize();
        }
    
        void processMouseMovement(float xoffset, float yoffset, bool rightButtonPressed) {
            xoffset *= mouseSensitivity;
            yoffset *= mouseSensitivity;
            
            if(rightButtonPressed) {
                yaw -= xoffset;
                pitch -= yoffset;
            } 

            // Prevent flipping when looking too far up or down
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
    
            update();
        }

        void processKeyboard(unsigned char key) {
            float velocity = movementSpeed;
        
            if (key == 'w' || key == 'W') {
                // Move forward in the direction of the camera's front vector.
                position = position + front * velocity;
            } else if (key == 's' || key == 'S') {
                // Move backward.
                position = position - front * velocity;
            } else if (key == 'a' || key == 'A') {
                // Strafe left: subtract the right vector.
                position = position - right * velocity;
            } else if (key == 'd' || key == 'D') {
                // Strafe right: add the right vector.
                position = position + right * velocity;
            }

            update();
        }
        
    
        cy::Vec3f getPosition() const { return position; }
        cy::Vec3f getFront() const { return front; }
        cy::Matrix4f getLookAtMatrix() const { return cy::Matrix4f::View(position, position+front, upVector); }
    
        cy::Matrix4f& getProjectionMatrix() {
            return perspectiveMatrix;
        }
    
        void setPosition(cy::Vec3f &newPos) {
            position = newPos;
        }
    
        void setFrontDirection(cy::Vec3f frontDir) {
            front = frontDir;
        }
    
        void setPerspectiveMatrix(float fov_degrees, float aspect, float znear, float zfar) {
            perspectiveMatrix = cy::Matrix4f::Perspective(Util::degreesToRadians(fov_degrees), aspect, znear, zfar);
    
        }

private:
    // camera position in world space
    cy::Vec3f position;
    cy::Vec3f front;
    cy::Vec3f up; // world space up direction
    cy::Vec3f right;
    cy::Vec3f upVector; // camera's up vector (in world space)

    cy::Matrix4f perspectiveMatrix;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;
};

#endif // CAMERA_H

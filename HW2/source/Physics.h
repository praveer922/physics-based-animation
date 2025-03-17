#ifndef PHYSICS_H
#define PHYSICS_H

#include <iostream>
#include <cmath>


struct PhysicsState {
    float mass;
    cy::Vec3f position;
    cy::Vec3f velocity;

    cy::Matrix3f orientation;    // Rotation matrix representing orientation
    cy::Vec3f angularVelocity;   // Angular velocity (axis-angle representation)
};

// Global boundaries and restitution factor.
float restitution = 0.8f; // restitution controls bounce energy loss
cy::Vec3f minBounds = {-47.0f, -25.0f, -47.0f};
cy::Vec3f maxBounds = {47.0f, 25.0f, 47.0f};


namespace Physics {

// This function uses an explicit integration method for updating the physics state.
inline void PhysicsUpdate(PhysicsState& state, cy::Vec3f force, cy::Vec3f torque, float deltaTime) {
    if (state.mass <= 0.0f) return; // Avoid division by zero

    // Compute acceleration using Newton's Second Law: F = ma -> a = F/m
    cy::Vec3f acceleration = force * (1.0f / state.mass);
    
    // Integrate velocity: v = v0 + a * dt
    state.velocity += acceleration * deltaTime;    
    // Integrate position: p = p0 + v * dt
    state.position += state.velocity * deltaTime;


    // Rotational dynamics (assuming an identity inertia tensor):
    cy::Vec3f angularAcceleration = torque;
    // Integrate angular velocity: ω = ω0 + α * dt
    state.angularVelocity += angularAcceleration * deltaTime;

    // Compute incremental rotation from angular velocity
    float angularSpeed = state.angularVelocity.Length();
    if (angularSpeed > 0.0f) {
        cy::Vec3f axis = state.angularVelocity.GetNormalized();
        cy::Matrix3f incrementalRotation;
        incrementalRotation.SetRotation(axis, angularSpeed * deltaTime);

        // Update orientation
        state.orientation = incrementalRotation * state.orientation;
    }

    // Check for wall boundary on the x, y, and z axes (3D box)
    for (int i = 0; i < 3; i++) {
        if (state.position[i] < minBounds[i]) {
            state.position[i] = minBounds[i];
            state.velocity[i] = -state.velocity[i] * restitution;
        } else if (state.position[i] > maxBounds[i]) {
            state.position[i] = maxBounds[i];
            state.velocity[i] = -state.velocity[i] * restitution;
        }
    }
}

} // namespace Physics

#endif // PHYSICS_H

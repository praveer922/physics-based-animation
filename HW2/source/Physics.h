#ifndef PHYSICS_H
#define PHYSICS_H

#include <iostream>
#include <cmath>
#include "Util.h"


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

// Process collisions for each vertex of the model
// vertices: a collection of the model's vertices in world space
void ProcessFloorCollision(PhysicsState& state, const std::vector<cy::Vec3f>& vertices) {
    // For a horizontal floor at y = 0, the contact normal is upward.
    const cy::Vec3f normal(0.0f, 1.0f, 0.0f);
    const float floorHeight = minBounds[1];

    // Iterate over each vertex in the model
    for (const auto& worldVertex : vertices) {
        // Check if the vertex is penetrating the floor.
        if (worldVertex.y < floorHeight) {
            // Compute penetration depth.
            float penetration = floorHeight - worldVertex.y;

            // Compute the lever arm from the center of mass to the contact point.
            cy::Vec3f r = worldVertex - state.position;

            // Compute the velocity at the contact point.
            // Linear velocity plus the rotational contribution: v = v_cm + ω × r
            cy::Vec3f v_contact = state.velocity + state.angularVelocity.Cross(r);

            // Only process if the vertex is moving into the floor.
            float v_rel = v_contact.Dot(normal);
            if (v_rel < 0.0f) {
                // Compute the impulse magnitude.
                // The formula for impulse magnitude is:
                //   j = -(1 + restitution) * (v_rel) / (1/m + n · ((r x n) x r))
                // With an identity inertia tensor, (I^-1 * (r x n)) simplifies to (r x n),
                // so the denominator becomes: 1/m + |r x n|^2.
                float denominator = (1.0f / state.mass) + (r.Cross(normal)).LengthSquared();
                float j = -(1.0f + restitution) * v_rel / denominator;

                // The impulse vector is along the contact normal.
                cy::Vec3f impulse = j * normal;

                // Apply the impulse to the linear velocity.
                state.velocity += impulse / state.mass;

                // Apply the impulse to the angular velocity.
                // The change in angular velocity is given by: Δω = I⁻¹ * (r × impulse).
                // With I⁻¹ assumed to be the identity, it's just r × impulse.
                state.angularVelocity += r.Cross(impulse);

                // Optionally: Adjust the position to reduce interpenetration.
                // This is a simple positional correction along the normal.
                state.position.y += penetration;
            }
        }
    }
}


void OnMouseClick(int mouseX, int mouseY, cy::Vec3f& externalTorque, PhysicsState & physicsState) {
    cy::Vec3f hitPoint = Util::screenToWorldSpaceXPlane(mouseX,mouseY,800,600);

    std::cout << "Mouse click: " << hitPoint.x << "," << hitPoint.y << "," << hitPoint.z << std::endl;
    
    // Compute the lever arm from the object's center of mass.
    cy::Vec3f r = hitPoint - physicsState.position;
    
    // Choose a force impulse to simulate the collision.
    float forceMagnitude = 10000000000.0f; // Adjust this value to your needs.
    cy::Vec3f forceDir = r.GetNormalized();
    cy::Vec3f forceImpulse = forceDir * forceMagnitude;
    
    // Calculate the torque: τ = r × F.
    externalTorque = r.Cross(forceImpulse);
    std::cout << "External torque: " << externalTorque.x << "," << externalTorque.y << "," << externalTorque.z << std::endl;
}


} // namespace Physics

#endif // PHYSICS_H

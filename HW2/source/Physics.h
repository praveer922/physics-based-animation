#ifndef PHYSICS_H
#define PHYSICS_H

#include <iostream>
#include <cmath>


struct PhysicsState {
    float mass;
    cy::Vec3f position;
    cy::Vec3f velocity;
    cy::Vec3f acceleration;
};

// Global boundaries and restitution factor.
float restitution = 0.8f; // restitution controls bounce energy loss
cy::Vec2f minBounds = {-22.0f, -17.0f};
cy::Vec2f maxBounds = {22.0f, 17.0f};


namespace Physics {

// This function uses an implicit integration method for updating the physics state.
inline void PhysicsUpdateImplicit(PhysicsState& state, float deltaTime) {
    if (state.mass <= 0.0f) return; // Avoid division by zero

    float k = 1; // force proportion

    // --- Step 1. Compute the acceleration from the current position.
    // For a circular field: a(x) = (k/m)*(-y, x, 0)
    cy::Vec3f a;
    a.x = (k / state.mass) * (-state.position.y);
    a.y = (k / state.mass) * ( state.position.x);
    a.z = 0.0f; // Motion is in the xy-plane

    // --- Step 2. Compute the right-hand side vector:
    // b = v(t) + deltaTime * a(x(t))
    cy::Vec3f b = state.velocity + a * deltaTime;

    // --- Step 3. Form the 2x2 matrix A = I - deltaTime^2 * J
    // where J = [ [0, -k/m], [k/m, 0] ].
    // Thus, A = [ [1,  deltaTime^2 * (k/m)],
    //             [-deltaTime^2 * (k/m), 1] ]
    float factor = deltaTime * deltaTime * (k / state.mass);
    float A11 = 1.0f;
    float A12 = factor;
    float A21 = -factor;
    float A22 = 1.0f;

    // --- Step 4. Invert the 2x2 matrix A.
    // Determinant: det = A11*A22 - A12*A21 = 1 + factor^2
    float det = A11 * A22 - A12 * A21;
    if (std::fabs(det) < 1e-8f) {
        std::cerr << "Matrix inversion error: determinant too close to zero.\n";
        return;
    }

    // Inverse of A:
    // A^{-1} = (1/det) * [ [A22, -A12],
    //                      [-A21, A11] ]
    float invA11 = A22 / det;
    float invA12 = -A12 / det;
    float invA21 = -A21 / det;
    float invA22 = A11 / det;

    // --- Step 5. Solve for the new velocity components in the xy-plane:
    // v_new_xy = A^{-1} * b_xy.
    float newVx = invA11 * b.x + invA12 * b.y;
    float newVy = invA21 * b.x + invA22 * b.y;
    
    // For z, we update explicitly (or assume it remains unchanged if motion is strictly planar).
    float newVz = state.velocity.z;

    // Update the state velocity.
    state.velocity.x = newVx;
    state.velocity.y = newVy;
    state.velocity.z = newVz;

    // --- Step 6. Update the position:
    // x(t+1) = x(t) + deltaTime * v(t+1)
    state.position = state.position + state.velocity * deltaTime;

    // Check for wall boundary on the x and y axes.
    for (int i = 0; i < 2; i++) {
        if (state.position[i] < minBounds[i]) {
            state.position[i] = minBounds[i];
            state.velocity[i] = -state.velocity[i] * restitution;
        } else if (state.position[i] > maxBounds[i]) {
            state.position[i] = maxBounds[i];
            state.velocity[i] = -state.velocity[i] * restitution;
        }
    }
}

// This function uses an explicit integration method for updating the physics state.
inline void PhysicsUpdate(PhysicsState& state, cy::Vec3f force, float deltaTime) {
    if (state.mass <= 0.0f) return; // Avoid division by zero

    // Compute acceleration using Newton's Second Law: F = ma -> a = F/m
    state.acceleration = force * (1.0f / state.mass);
    
    // Integrate velocity: v = v0 + a * dt
    state.velocity += state.acceleration * deltaTime;
    
    // Integrate position: p = p0 + v * dt
    state.position += state.velocity * deltaTime;

    // Check for wall boundary on the x and y axes.
    for (int i = 0; i < 2; i++) {
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

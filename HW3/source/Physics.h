#ifndef PHYSICS_H
#define PHYSICS_H

#include <iostream>
#include <cmath>
#include "Util.h"

// structs for nodes and springs
struct MassPoint {
    cy::Vec3f position;
    cy::Vec3f velocity    = cy::Vec3f(0,0,0);
    cy::Vec3f force       = cy::Vec3f(0,0,0);
    float     mass        = 1.0f;
    bool      fixed       = false;        // anchors won’t move
};

struct Spring {
    int       a, b;        // indices into your MassPoint array
    float     restLength;
    float     stiffness;   // e.g. 50–200
    float     damping;     // e.g. 0.1–1.0
};

// Global boundaries and restitution factor.
float restitution = 0.8f; // restitution controls bounce energy loss
cy::Vec3f minBounds = {-47.0f, -25.0f, -47.0f};
cy::Vec3f maxBounds = {47.0f, 25.0f, 47.0f};


namespace Physics {

// This function uses an explicit integration method for updating the physics state.
inline void PhysicsUpdate(std::vector<MassPoint> & mpoints, std::vector<Spring> & springs, const cy::Vec3f externalForce, float deltaTime) {
    // zero forces
    for (auto &mp : mpoints) {
        if (!mp.fixed) mp.force = cy::Vec3f(0, -9.8f * mp.mass, 0) + externalForce;  // gravity
    }

    // spring forces
    for (auto &s : springs) {
        auto &A = mpoints[s.a];
        auto &B = mpoints[s.b];
        cy::Vec3f dir   = B.position - A.position;
        float      len  = dir.Length();
        if (len > 0) {
            cy::Vec3f e = dir / len;
            // Hooke’s law
            float fs = -s.stiffness * (len - s.restLength);
            // damping: relative velocity along the spring
            float fd = -s.damping * ( (B.velocity - A.velocity).Dot(e) );
            cy::Vec3f f = e * (fs + fd);
            if (!A.fixed) A.force +=  f;
            if (!B.fixed) B.force += -f;
        }
    }

    // integrate (semi‑implicit Euler)
    for (auto &mp : mpoints) {
        if (mp.fixed) continue;
        mp.velocity += (deltaTime/mp.mass) * mp.force;
        mp.position += deltaTime * mp.velocity;
    }
}

/*
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
*/

void OnMouseClick(int mouseX, int mouseY, cy::Vec3f& externalForce) {

    const float magnitude = 10.0f;
    
    externalForce = cy::Vec3f(mouseX * magnitude,
        mouseY * magnitude,
        0.0f);

    std::cout << "External force: " << externalForce.x << "," << externalForce.y << "," << externalForce.z << std::endl;
}




} // namespace Physics

#endif // PHYSICS_H

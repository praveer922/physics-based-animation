#ifndef UTIL_H
#define UTIL_H

#include <cmath>

namespace Util { 

    float degreesToRadians(float degrees) {
        return degrees * M_PI / 180.0;
    }

    cy::Vec2f screenToNDC(int screenX, int screenY, int screenWidth, int screenHeight) {
        cy::Vec2f ndc;
        // Convert X from screen space to NDC (-1 to 1)
        ndc.x = 2.0f * screenX / screenWidth - 1.0f;
        // Convert Y from screen space to NDC (1 to -1)
        ndc.y = 1.0f - 2.0f * screenY / screenHeight;
        return ndc;
    }


    cy::Vec3f screenToWorldSpaceXPlane(int screenX, int screenY, int screenWidth, int screenHeight) {
        cy::Vec2f ndc = screenToNDC(screenX, screenY, screenWidth, screenHeight);
        // Convert X from ndc to project X plane in worldspace
        // as an approximation, just scale it to (-50,50)
        return cy::Vec3f(ndc.x*50,ndc.y*50,-10.0f);

    }


}

#endif 
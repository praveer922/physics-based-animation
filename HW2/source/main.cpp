#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "cyTriMesh.h"
#include "cyMatrix.h"
#include "cyGL.h"
#include <iostream>
#include <chrono>

using namespace std;


/*
CONTROLS:
V -- toggle velocity field
F -- toggle force field
R -- reset 
I -- demo implicit force field
*/

int num_vertices;
GLuint VAO;
GLuint lineVAO;
float rot_x = -90.0f;
float rot_y = 0.0f;
float light_rot_y = 0.0f;
float light_rot_z = 0.0f;
float camera_distance = 50.0f;
cy::GLSLProgram prog;
cy::GLSLProgram lineProg;
bool leftButtonPressed = false;
bool controlKeyPressed = false;
cy::TriMesh mesh;
cy::Vec3f lightPosLocalSpace = cy::Vec3f(15.0, -15.0, 15.0);

// Variables to store mouse click position
float lastClickX, lastClickY;
bool arrowVisible = false;

// variables for physics
struct PhysicsState {
    cy::Vec3f position;
    cy::Vec3f velocity;
    cy::Vec3f acceleration;
    float mass;
};

PhysicsState physicsState;
cy::Vec3f forceVector;
float dampingFactor = 0.5f;
float restitution = 0.8f; // restitution controls bounce energy loss
cy::Vec2f minBounds = {-22.0f, -17.0f};
cy::Vec2f maxBounds = {22.0f, 17.0f};

// simulation/render time steps
auto lastTime = std::chrono::high_resolution_clock::now();

// view and proj matrices (since camera is fixed)
cy::Matrix4f view = cy::Matrix4f::View(cy::Vec3f(0.0f, 0.0f, camera_distance), cy::Vec3f(0.0f,0.0f,0.0f), cy::Vec3f(0.0f,1.0f,0.0f));
cy::Matrix4f proj = cy::Matrix4f::Perspective(40 * 3.14 / 180.0, 800.0 / 600.0, 2.0f, 1000.0f);
float scaleFactor = 1.0f; // scale factor for obj model

// toggles for velocity/force fields and implicit mode
bool velocityFieldOn = false;
bool forceFieldOn = false;
bool implicitMode = false;


GLfloat lineVertices[] = {
    0.0f, 0.0f,   // Start point in screen space (normalized NDC) for (400, 300)
    0.0f, 0.333f  // End point in screen space (normalized NDC) for (400, 400)
};

cy::Vec2f screenToNDC(int screenX, int screenY, int screenWidth, int screenHeight) {
    cy::Vec2f ndc;
    // Convert X from screen space to NDC (-1 to 1)
    ndc.x = 2.0f * screenX / screenWidth - 1.0f;
    // Convert Y from screen space to NDC (1 to -1)
    ndc.y = 1.0f - 2.0f * screenY / screenHeight;
    return ndc;
}

cy::Vec3f getVelocityMapping(cy::Vec3f worldPos) {
    return cy::Vec3f(0.01f, -0.01f, 0.0f);
}

cy::Vec3f getForceMapping(const cy::Vec3f& worldPos) {
    // Compute radial vector
    cy::Vec3f radial = worldPos;
    radial.z = 0.0f; // Assuming motion in XY plane
    
    // Compute distance (radius)
    float radius = radial.Length();
    if (radius == 0.0f) return cy::Vec3f(0.0f, 0.0f, 0.0f); // Avoid division by zero
    
    // Normalize to get unit radial direction
    cy::Vec3f radialDir = radial / radius;
    
    // Compute tangential direction (rotate 90 degrees counterclockwise in 2D)
    cy::Vec3f tangentialDir(-radialDir.y, radialDir.x, 0.0f);
    
    // Scale force by radius (force magnitude increases with distance)
    return tangentialDir * radius;
}

void PhysicsUpdateImplicit(PhysicsState& state, float deltaTime) {
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
    // Determinant: det = A11*A22 - A12*A21
    float det = A11 * A22 - A12 * A21; // = 1 + factor^2
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
    
    // For z, we update explicitly (or assume it remains 0 if motion is strictly planar).
    float newVz = state.velocity.z;  // Here we leave it unchanged

    // Update the state velocity.
    state.velocity.x = newVx;
    state.velocity.y = newVy;
    state.velocity.z = newVz;

    // --- Step 6. Update the position:
    // x(t+1) = x(t) + deltaTime * v(t+1)
    state.position = state.position + state.velocity * deltaTime;

    // check for wall boundary
    for (int i=0; i<2;i++) { // x and y axes
        if (state.position[i] < minBounds[i]) {
            state.position[i] = minBounds[i]; // Keep within bounds
            state.velocity[i] = -state.velocity[i] * restitution; // Reverse velocity with restitution factor
        }
        else if (state.position[i] > maxBounds[i]) {
            state.position[i] = maxBounds[i]; 
            state.velocity[i] = -state.velocity[i] * restitution;
        }
    }
}




void PhysicsUpdate(PhysicsState& state, cy::Vec3f force, float deltaTime) {
    if (state.mass <= 0.0f) return; // Avoid division by zero

    if (forceFieldOn) {
        force += getForceMapping(state.position);
    }

    // Compute acceleration using Newton's Second Law: F = ma -> a = F/m
    state.acceleration = force / state.mass;
    
    // Integrate velocity: v = v0 + a * dt
    state.velocity += state.acceleration * deltaTime;

    if (velocityFieldOn) {
        state.velocity += getVelocityMapping(state.position) * dampingFactor;
    }
    
    // Integrate position: p = p0 + v * dt
    state.position += state.velocity * deltaTime;

    // check for wall boundary
    for (int i=0; i<2;i++) { // x and y axes
        if (state.position[i] < minBounds[i]) {
            state.position[i] = minBounds[i]; // Keep within bounds
            state.velocity[i] = -state.velocity[i] * restitution; // Reverse velocity with restitution factor
        }
        else if (state.position[i] > maxBounds[i]) {
            state.position[i] = maxBounds[i]; 
            state.velocity[i] = -state.velocity[i] * restitution;
        }
    }

}



void display() {
    // set uniforms    
     // Calculate the bounding box
    mesh.ComputeBoundingBox();
    cy::Vec3f center = (mesh.GetBoundMin() + mesh.GetBoundMax()) * 0.5f;
    // Adjust the model transformation matrix to center the object (reverse matrix multiplication order)
    cy::Matrix4f model = cy::Matrix4f::Translation(physicsState.position) * 
                         cy::Matrix4f::Scale(scaleFactor) *
                         cy::Matrix4f::Translation(-center);


    // Your rendering code goes here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Draw your graphics here
    prog.Bind();
    prog["lightPosLocalSpace"] = lightPosLocalSpace;
    prog["lightRot"] = cy::Matrix4f::RotationY(light_rot_y * 3.14 /180.0) * cy::Matrix4f::RotationZ(light_rot_z * 3.14 /180.0);
    prog["model"] = model;
    prog["view"] = view;
    prog["projection"] = proj;
    prog["cameraViewPos"] = view * cy::Vec3f(0.0f, 0.0f, camera_distance);
    prog["normalTransform"] = (view*model).GetSubMatrix3();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, mesh.NF() * 3, GL_UNSIGNED_INT, 0);

    // DRAW LINE
    if (arrowVisible)
    {
        GLuint lineVBO;
        glGenVertexArrays(1, &lineVAO);
        glBindVertexArray(lineVAO);

        glGenBuffers(1, &lineVBO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

        // Define two points in normalized screen space (NDC)
        // Compute the sphere's clip-space coordinate from its world position
        cy::Vec4f clipPos = proj * view * cy::Vec4f(physicsState.position, 1.0f);
        // Perform perspective divide to get NDC
        clipPos /= clipPos.w;
        // ndcStart is the (x, y) in normalized device coordinates
        cy::Vec2f ndcStart(clipPos.x, clipPos.y);
        cy::Vec2f ndcEnd = screenToNDC(lastClickX,lastClickY, 800, 600);
        GLfloat lineVertices[] = {
            ndcStart.x, ndcStart.y,   // Start point in screen space (normalized NDC) 
            ndcEnd.x, ndcEnd.y  // End point in screen space (normalized NDC) 
        };

        //update current force vector
        forceVector = (cy::Vec3f(ndcEnd.x, ndcEnd.y, 0.0f) - cy::Vec3f(ndcStart.x, ndcStart.y, 0.0f)) * 20.0f;

        glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0); // Unbind the VAO
        lineProg.Bind();
        glBindVertexArray(lineVAO);
        glDrawArrays(GL_LINES, 0, 2); // Draw the line using two points
        glBindVertexArray(0);
    }

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) {  // ASCII value for the Esc key
        glutLeaveMainLoop();
    } else if (key == 'v' || key == 'V') {  
        velocityFieldOn = !velocityFieldOn;
        forceFieldOn = false;
        implicitMode = false;
    } else if (key == 'f' || key == 'F') {  
        forceFieldOn = !forceFieldOn;
        velocityFieldOn = false;
        implicitMode = false;
    } else if (key == 'i' || key == 'I') {  
        implicitMode = true;
        velocityFieldOn = false;
        forceFieldOn = false;

        // reset variables for implicit mode demo
        arrowVisible =false;
        forceVector = {0.0f,0.0f,0.0f};
        physicsState.acceleration = {0.0f,0.0f, 0.0f};
        physicsState.velocity = {0.0f,0.0f, 0.0f};
        physicsState.position = {1.0f, 0.0f, 0.0f};
    }

    if (velocityFieldOn) {
        cout << "Velocity field is ON." << endl;
    } else {
        cout << "Velocity field is OFF." << endl;
    } 

    if (forceFieldOn) {
        cout << "Force field is ON." << endl;
    } else {
        cout << "Force field is OFF." << endl;
    }

    if (implicitMode) {
        cout << "Now in Implicit mode demo." << endl;
    } 
}

void specialKeyboard(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_F6:
            // Reload shaders when F6 key is pressed
            prog.BuildFiles("vs.txt", "fs.txt");
            prog.Bind();
            cout << "Shaders recompiled successfully." << endl;
            glutPostRedisplay();
            break;
        case GLUT_KEY_CTRL_L:
        case GLUT_KEY_CTRL_R:
            controlKeyPressed = true;
            break;
    }
}

void specialKeyboardUp(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_CTRL_L:
        case GLUT_KEY_CTRL_R:
            controlKeyPressed = false;
            break;
    }
}

void handleMouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (!implicitMode) {
            lastClickX = x;
            lastClickY = y;
            arrowVisible = true;
            glutPostRedisplay();
        }
        
    }
}


void loadModel(int argc, char** argv, cy::TriMesh & mesh) {
    char * modelName;

    // load model
    if (argc<1) {
        cout << "No model given. Specify model name in cmd-line arg." << endl;
        exit(0);
    } else {
        modelName = argv[1];
        cout << "Loading " << modelName << "..." << endl;
    }

    bool success = mesh.LoadFromFileObj(modelName);
    if (!success) {
        cout << "Model loading failed." << endl;
        exit(0);
    } else {
        cout << "Loaded model successfully." << endl;
        num_vertices = mesh.NV();
        mesh.ComputeNormals();
    }

    // compute model scale factor
    if (std::string(modelName) == "dragon.obj") {
        scaleFactor = 5.0f;
    } else if (std::string(modelName) == "armadillo.obj") {
        scaleFactor = 0.06f;
    } else {
        scaleFactor = 1.0f;
    }

}

void idle() {
    // first, update physics
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsedTime = currentTime - lastTime;
    float deltaTime = elapsedTime.count();
    if (implicitMode) {
        PhysicsUpdateImplicit(physicsState, deltaTime);
    } else {
        PhysicsUpdate(physicsState, forceVector, deltaTime);
    }

    lastTime = currentTime;


    glutPostRedisplay();
}

int main(int argc, char** argv) {
    // initial physics
    physicsState.mass = 1.0f;
    physicsState.position = cy::Vec3f(0.0, 0.0, 0.0); 

    // Initialize GLUT
    glutInit(&argc, argv);

    // Set OpenGL version and profile
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    // Set up a double-buffered window with RGBA color
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

    // some default settings
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);


    // Create a window with a title
    glutCreateWindow("HW2");

    // Initialize GLEW
    glewInit();
    glEnable(GL_DEPTH_TEST);  
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Set up callbacks
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    glutIdleFunc(idle);
    glutSpecialUpFunc(specialKeyboardUp);
    glutMouseFunc(handleMouse);



    // load models
    loadModel(argc, argv, mesh);
    //loadArrowModel("arrow.obj");

    // set up VAO and VBO and EBO and NBO
    glGenVertexArrays(1, &VAO); 
    glBindVertexArray(VAO);

    GLuint normalVBO;
    glGenBuffers(1, &normalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f) * mesh.NV(), &mesh.VN(0), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1); // Assuming attribute index 1 for normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f)*num_vertices, &mesh.V(0), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh.NF() * 3, &mesh.F(0), GL_STATIC_DRAW);


    // link shaders
    prog.BuildFiles("vs.txt", "fs.txt");
    lineProg.BuildFiles("arrow_vs.txt", "lightcube_fs.txt");


    // Enter the GLUT event loop
    glutMainLoop();

    return 0;
}
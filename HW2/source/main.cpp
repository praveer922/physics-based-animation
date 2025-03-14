#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "cyTriMesh.h"
#include "cyMatrix.h"
#include "cyGL.h"
#include "Camera.h"
#include "Physics.h"
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
cy::GLSLProgram prog;
cy::GLSLProgram lineProg;
bool leftButtonPressed = false;
bool controlKeyPressed = false;
cy::TriMesh mesh;
cy::Vec3f lightPosLocalSpace = cy::Vec3f(15.0, -15.0, 15.0);

// Variables to store mouse click position
float lastClickX, lastClickY;
bool arrowVisible = false;

PhysicsState physicsState;
cy::Vec3f forceVector;
float dampingFactor = 0.5f;

// simulation/render time steps
auto lastTime = std::chrono::high_resolution_clock::now();

// init camera
Camera camera(cy::Vec3f(0.0f, 0.0f, 50.0f), // camera at 0,0,50
              cy::Vec3f(0.0f, 0.0f, 0.0f),
              cy::Vec3f(0.0f, 1.0f, 0.0f));
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
    prog["view"] = camera.getViewMatrix();
    prog["projection"] =  camera.getProjectionMatrix();
    prog["normalTransform"] = (camera.getViewMatrix()*model).GetSubMatrix3();
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
        cy::Vec4f clipPos = camera.getProjectionMatrix() * camera.getViewMatrix() * cy::Vec4f(physicsState.position, 1.0f);
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
        Physics::PhysicsUpdateImplicit(physicsState, deltaTime);
    } else {
        Physics::PhysicsUpdate(physicsState, forceVector, deltaTime);
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
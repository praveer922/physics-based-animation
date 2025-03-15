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

// init physics variables
PhysicsState physicsState;
cy::Vec3f forceVector;

// simulation/render time steps
auto lastTime = std::chrono::high_resolution_clock::now();

// init camera
float lastClickX = 400, lastClickY = 300;
float lastX = 400, lastY = 300;
Camera camera(cy::Vec3f(0.0f, 0.0f, 50.0f)); // camera at 0,0,50
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

    cy::Matrix4f view = camera.getLookAtMatrix();
    cy::Matrix4f proj = camera.getProjectionMatrix();


    // Your rendering code goes here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Draw your graphics here
    prog.Bind();
    prog["lightPosLocalSpace"] = lightPosLocalSpace;
    prog["lightRot"] = cy::Matrix4f::RotationY(light_rot_y * 3.14 /180.0) * cy::Matrix4f::RotationZ(light_rot_z * 3.14 /180.0);
    prog["model"] = model;
    prog["view"] = view;
    prog["projection"] =  proj;
    prog["normalTransform"] = (view*model).GetSubMatrix3();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, mesh.NF() * 3, GL_UNSIGNED_INT, 0);

    glutSwapBuffers();
}
void keyboard(unsigned char key, int x, int y) {

    if (key == 27) {  // Esc key
        glutLeaveMainLoop();
    } /*else if (key == 'w' || key == 'W') {
        // Move forward
        cy::Vec3f forward = camera.getTarget() - camera.getPosition();
        forward.Normalize();
        cy::Vec3f newPos = camera.getPosition() + forward * speed;
        camera.setPosition(newPos);
        camera.setTarget(newPos + forward);
    } else if (key == 's' || key == 'S') {
        // Move backward
        cy::Vec3f forward = camera.getTarget() - camera.getPosition();
        forward.Normalize();
        cy::Vec3f newPos = camera.getPosition() - forward * speed;
        camera.setPosition(newPos);
        camera.setTarget(newPos + forward);
    } else if (key == 'a' || key == 'A') {
        // Strafe left
        cy::Vec3f forward = camera.getTarget() - camera.getPosition();
        forward.Normalize();
        // Compute right vector (world up is (0,1,0))
        cy::Vec3f right = forward.Cross(cy::Vec3f(0.0f, 1.0f, 0.0f));
        right.Normalize();
        cy::Vec3f newPos = camera.getPosition() - right * speed;
        camera.setPosition(newPos);
        camera.setTarget(newPos + forward);
    } else if (key == 'd' || key == 'D') {
        // Strafe right
        cy::Vec3f forward = camera.getTarget() - camera.getPosition();
        forward.Normalize();
        cy::Vec3f right = forward.Cross(cy::Vec3f(0.0f, 1.0f, 0.0f));
        right.Normalize();
        std::cout << "right: " << right.x << ", " << right.y << ", " << right.z << std::endl;
        right *= speed;
        std::cout << "rightspeed: " << right.x << ", " << right.y << ", " << right.z << std::endl;
        cy::Vec3f newPos = camera.getPosition() + (right * speed);
        std::cout << "newpos: " << newPos.x << ", " << newPos.y << ", " << newPos.z << std::endl;
        camera.setPosition(newPos);
        camera.setTarget(newPos + forward);
    }

    std::cout << "New Camera Position: " << camera.getPosition().x << ", " 
          << camera.getPosition().y << ", " << camera.getPosition().z << std::endl;
    */

    // Redraw the scene to reflect the camera changes
    glutPostRedisplay();
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
    // Update last mouse position
    lastX = x;
    lastY = y;
}

void passiveMouseMotion(int x, int y) {
    float xoffset = x - lastX;
    float yoffset = lastY - y; // reversed since y-coordinates range from bottom to top
    lastX = x;
    lastY = y;

    camera.processMouseMovement(xoffset, yoffset);

    glutPostRedisplay();
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
    glutPassiveMotionFunc(passiveMouseMotion);



    // load models
    loadModel(argc, argv, mesh);
    

    //init camera
    camera.setPerspectiveMatrix(65,800.0f/600.0f, 2.0f, 600.0f);

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
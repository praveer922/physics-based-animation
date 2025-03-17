#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "cyTriMesh.h"
#include "cyMatrix.h"
#include "cyGL.h"
#include "Camera.h"
#include "Physics.h"
#include "Models.h"
#include <iostream>
#include <chrono>

using namespace std;

GLuint VAO;
GLuint planeVAO;
float rot_x = -90.0f;
float rot_y = 0.0f;
float light_rot_y = 0.0f;
float light_rot_z = 0.0f;
cy::GLSLProgram prog;
cy::GLSLProgram planeProg;
bool leftButtonPressed = false;
cy::TriMesh mesh;
cy::Vec3f lightPosLocalSpace = cy::Vec3f(15.0, -15.0, 15.0);

// init physics variables
PhysicsState physicsState;
cy::Vec3f forceVector;

// simulation/render time steps
auto lastTime = std::chrono::high_resolution_clock::now();

// init camera
bool rightButtonPressed = false;
float lastClickX = 400, lastClickY = 300;
float lastX = 400, lastY = 300;
Camera camera(cy::Vec3f(0.0f, 0.0f, 50.0f)); // camera at 0,0,50
float scaleFactor = 1.0f; // scale factor for obj model



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
    cy::Matrix4f angularRotation = cy::Matrix4f(physicsState.orientation);
    cy::Matrix4f model = cy::Matrix4f::Translation(physicsState.position) * 
                         angularRotation *
                         cy::Matrix4f::Scale(scaleFactor) *
                         cy::Matrix4f::Translation(-center);



    //model = angularRotation * model;

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

    planeProg.Bind();
    planeProg["lightPosLocalSpace"] = lightPosLocalSpace;
    planeProg["lightRot"] = cy::Matrix4f::RotationY(light_rot_y * 3.14 /180.0) * cy::Matrix4f::RotationZ(light_rot_z * 3.14 /180.0);
    model = cy::Matrix4f(1.0);
    planeProg["model"] = model;
    planeProg["view"] = view;
    planeProg["projection"] =  proj;
    planeProg["normalTransform"] = (view*model).GetSubMatrix3();
    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glutSwapBuffers();
}


void keyboard(unsigned char key, int x, int y) {

    if (key == 27) {  // Esc key
        glutLeaveMainLoop();
    } else {
        camera.processKeyboard(key);
    }

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
    }
}
void handleMouse(int button, int state, int x, int y) {

    if (button == GLUT_RIGHT_BUTTON) {
        rightButtonPressed = (state == GLUT_DOWN);
    }

    // Update last mouse position
    lastX = x;
    lastY = y;
}

void mouseMotion(int x, int y) {
    float xoffset = x - lastX;
    float yoffset = lastY - y; // reversed since y-coordinates range from bottom to top
    lastX = x;
    lastY = y;

    camera.processMouseMovement(xoffset, yoffset, rightButtonPressed);

    glutPostRedisplay();
}


void idle() {
    // first, update physics
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsedTime = currentTime - lastTime;
    float deltaTime = elapsedTime.count();
    
    cy::Vec3f gravityForce = cy::Vec3f(0.0f, -9.8f * physicsState.mass, 0.0f);
    Physics::PhysicsUpdate(physicsState, gravityForce, deltaTime);

    lastTime = currentTime;


    glutPostRedisplay();
}

int main(int argc, char** argv) {
    // initial physics
    physicsState.mass = 1.0f;
    physicsState.position = cy::Vec3f(0.0, 0.0, 0.0); 
    physicsState.orientation.SetIdentity();
    physicsState.orientation.SetRotationZ(Util::degreesToRadians(35));
    physicsState.angularVelocity = cy::Vec3f(0.0f);

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
    glutMouseFunc(handleMouse);
    glutMotionFunc(mouseMotion);

    
    //init camera
    camera.setPerspectiveMatrix(65,800.0f/600.0f, 2.0f, 600.0f);

    // load model
    int num_vertices = Models::loadModel(argc, argv, mesh, scaleFactor);

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

    // set up plane
    glGenVertexArrays(1, &planeVAO); 
    glBindVertexArray(planeVAO);

    GLuint planeVBO;
    glGenBuffers(1, &planeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * Models::planeVertices.size(), Models::planeVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);



    // link shaders
    prog.BuildFiles("vs.txt", "fs.txt");
    planeProg.BuildFiles("plane_vs.txt", "plane_fs.txt");


    // Enter the GLUT event loop
    glutMainLoop();

    return 0;
}
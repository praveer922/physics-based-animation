#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "cyTriMesh.h"
#include "cyMatrix.h"
#include "cyGL.h"
#include <iostream>

using namespace std;

int num_vertices;
GLuint VAO;
GLuint lightCubeVAO;
float rot_x = -90.0f;
float rot_y = 0.0f;
float light_rot_y = 0.0f;
float light_rot_z = 0.0f;
float camera_distance = 50.0f;
float lastX = 400, lastY = 300;
cy::GLSLProgram prog;
cy::GLSLProgram lightCubeProg;
bool leftButtonPressed = false;
bool controlKeyPressed = false;
cy::TriMesh mesh;
bool orthogonal_projection_on = false;
cy::Vec3f lightPosLocalSpace = cy::Vec3f(15.0, -15.0, 15.0);

float lightCubeVertices[] = {
        -0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f,  
         0.5f,  0.5f, -0.5f,  
         0.5f,  0.5f, -0.5f,  
        -0.5f,  0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 

        -0.5f, -0.5f,  0.5f, 
         0.5f, -0.5f,  0.5f,  
         0.5f,  0.5f,  0.5f,  
         0.5f,  0.5f,  0.5f,  
        -0.5f,  0.5f,  0.5f, 
        -0.5f, -0.5f,  0.5f, 

        -0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 
        -0.5f, -0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f, 

         0.5f,  0.5f,  0.5f,  
         0.5f,  0.5f, -0.5f,  
         0.5f, -0.5f, -0.5f,  
         0.5f, -0.5f, -0.5f,  
         0.5f, -0.5f,  0.5f,  
         0.5f,  0.5f,  0.5f,  

        -0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f,  
         0.5f, -0.5f,  0.5f,  
         0.5f, -0.5f,  0.5f,  
        -0.5f, -0.5f,  0.5f, 
        -0.5f, -0.5f, -0.5f, 

        -0.5f,  0.5f, -0.5f, 
         0.5f,  0.5f, -0.5f,  
         0.5f,  0.5f,  0.5f,  
         0.5f,  0.5f,  0.5f,  
        -0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f, -0.5f, 
    };

cy::Matrix4f getOrthographicMatrix(float left, float right, float bottom, float top, float near, float far) {
        cy::Matrix4f proj = cy::Matrix4f(2.0f / (right - left), 0, 0, -(right + left) / (right - left),
        0, 2.0f / (top - bottom), 0, -(top + bottom) / (top - bottom),
        0, 0, -2.0f / (far - near),  -(far + near) / (far - near),
        0, 0, 0, 1);

        return proj;
}

void display() {
    // set uniforms    
     // Calculate the bounding box
    mesh.ComputeBoundingBox();
    cy::Vec3f center = (mesh.GetBoundMin() + mesh.GetBoundMax()) * 0.5f;

    //add two rotations to model matrix
    cy::Matrix4f model = cy::Matrix4f::RotationX(rot_x * 3.14 /180.0) * cy::Matrix4f::RotationY(rot_y * 3.14 /180.0);
    // Adjust the model transformation matrix to center the object
    model *= cy::Matrix4f::Translation(-center); 
    
    cy::Matrix4f view = cy::Matrix4f::View(cy::Vec3f(0.0f, 0.0f, camera_distance), cy::Vec3f(0.0f,0.0f,0.0f), cy::Vec3f(0.0f,1.0f,0.0f));
    cy::Matrix4f proj = cy::Matrix4f(1.0);
    if (!orthogonal_projection_on) {
        proj *= cy::Matrix4f::Perspective(40 * 3.14 /180.0, 800.0/600.0, 2.0f, 1000.0f);
    } else {
        float scale_factor = 500.0f/camera_distance;
        proj*= getOrthographicMatrix(-scale_factor, scale_factor, -scale_factor, scale_factor, 0.1f, 1500.0f);
    }

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
    glBindVertexArray(lightCubeVAO);
    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) {  // ASCII value for the Esc key
        glutLeaveMainLoop();
    } else if (key == 'P' || key == 'p') {
        orthogonal_projection_on = !orthogonal_projection_on;
        if (orthogonal_projection_on) {
            cout << "Switched to orthogonal projection." << endl;
        } else {
            cout << "Switched to perspective projection." << endl;
        }
        glutPostRedisplay();
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
    if (button == GLUT_LEFT_BUTTON) {
        leftButtonPressed = (state == GLUT_DOWN);
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

    const float sensitivity = 0.3f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    if (leftButtonPressed) {
        if (controlKeyPressed) {
            light_rot_y +=xoffset;
            light_rot_z += yoffset;
        } else {
            rot_x -=yoffset;
            rot_y +=xoffset;
        }

    } else {
        camera_distance -= yoffset;
    }

    glutPostRedisplay();
}

void idle() {
    // hue += 0.1f;

    // hue = fmod(hue, 360.0f);

    // // Trigger a redraw
    // glutPostRedisplay();
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
}



int main(int argc, char** argv) {
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
    glutCreateWindow("HW1");

    // Initialize GLEW
    glewInit();
    glEnable(GL_DEPTH_TEST);  
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Set up callbacks
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    glutSpecialUpFunc(specialKeyboardUp);
    glutIdleFunc(idle);
    glutMouseFunc(handleMouse);
    glutMotionFunc(mouseMotion);



    // load model
    loadModel(argc, argv, mesh);

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

    glGenVertexArrays(1, &lightCubeVAO); 
    glBindVertexArray(lightCubeVAO);

    GLuint lightCubeVBO;
    glGenBuffers(1, &lightCubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lightCubeVertices), lightCubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


    // link shaders
    prog.BuildFiles("vs.txt", "fs.txt");
    lightCubeProg.BuildFiles("vs.txt", "lightcube_fs.txt");


    // Enter the GLUT event loop
    glutMainLoop();

    return 0;
}
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
GLuint VBO;
GLuint planeVAO;
float rot_x = -90.0f;
float rot_y = 0.0f;
float light_rot_y = 0.0f;
float light_rot_z = 0.0f;
cy::GLSLProgram prog;
cy::GLSLProgram planeProg;
bool leftButtonPressed = false;
int num_vertices;
std::vector<cy::Vec3f> nodes;
cy::Vec3f centroid(0.0f, 0.0f, 0.0f);
cy::Vec3f lightPosLocalSpace = cy::Vec3f(15.0, -15.0, 15.0);

// init physics variables

std::vector<MassPoint> mpoints;
std::vector<Spring> springs;
std::vector<cy::Vec3f> verticesWorldSpace;

// simulation/render time steps
auto lastTime = std::chrono::high_resolution_clock::now();

// init camera
bool rightButtonPressed = false;
float lastClickX = 400, lastClickY = 300;
float lastX = 400, lastY = 300;
Camera camera(cy::Vec3f(0.0f, 0.0f, 50.0f)); // camera at 0,0,50
float scaleFactor = 0.06f;; // scale factor for armadillo model

cy::Vec3f externalForce(0.0f,0.0f,0.0f);


void display() {
    // Adjust the model transformation matrix to center the object (reverse matrix multiplication order)
    cy::Matrix4f model = cy::Matrix4f::Scale(scaleFactor) *
                         cy::Matrix4f::Translation(-centroid);


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
    glDrawElements(GL_TRIANGLES, num_vertices, GL_UNSIGNED_INT, 0);

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

    // Check for left mouse button click
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        Physics::OnMouseClick(x,y,externalForce);
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

    //Physics::ProcessFloorCollision(physicsState, verticesWorldSpace);
    Physics::PhysicsUpdate(mpoints, springs, externalForce, deltaTime);
    externalForce = {0.0f,0.0f,0.0f};
    for (size_t i = 0; i < mpoints.size(); ++i) {
        nodes[i] = mpoints[i].position;
    }
    
    // now push that updated block of memory into the VBO:
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // offset = 0, size = whole buffer
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        nodes.size() * sizeof(cy::Vec3f),
        nodes.data()
    );

    lastTime = currentTime;


    glutPostRedisplay();
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
    glutCreateWindow("HW3");

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

    // load volumetric model
    std::vector<Models::Tetrahedron> tetrahedra;


    if (!Models::loadNodes("armadillo_50k_tet.node", nodes, centroid)) { /* error handling */ }
    if (!Models::loadTetrahedra("armadillo_50k_tet.ele", tetrahedra)) { /* error handling */ }


    // Extract the surface triangles from the tetrahedral mesh
    std::vector<Models::Face> surfaceFaces = Models::extractSurfaceFaces(tetrahedra);

    std::cout << "No. of surface faces = " << surfaceFaces.size() << std::endl;

    // Build the surface mesh data for OpenGL rendering
    std::vector<unsigned int> surfaceIndices;


    // Populate the indices using the surfaceFaces
    for (const auto &face : surfaceFaces) {
        surfaceIndices.push_back(face.a);
        surfaceIndices.push_back(face.b);
        surfaceIndices.push_back(face.c);
    }

    num_vertices = surfaceIndices.size();
    verticesWorldSpace.resize(num_vertices);


    std::vector<cy::Vec3f> surfaceNormals(nodes.size(), cy::Vec3f(0.0f, 0.0f, 0.0f));

    // Loop through each face and add its normal to its vertices.
    for (const auto &face : surfaceFaces) {
        // Retrieve the three vertices for the face.
        cy::Vec3f v0 = nodes[face.a];
        cy::Vec3f v1 = nodes[face.b];
        cy::Vec3f v2 = nodes[face.c];
        
        // Compute the face's normal.
        cy::Vec3f edge1 = v1 - v0;
        cy::Vec3f edge2 = v2 - v0;
        cy::Vec3f faceNormal = edge1.Cross(edge2);
        faceNormal.Normalize();  // Normalize the face normal.
        
        // Accumulate the face normal into each vertex's normal.
        surfaceNormals[face.a] += faceNormal;
        surfaceNormals[face.b] += faceNormal;
        surfaceNormals[face.c] += faceNormal;
    }

    // Normalize all the per-vertex normals (to get the average direction).
    for (size_t i = 0; i < surfaceNormals.size(); i++) {
        float len = surfaceNormals[i].Length();
        if (len > 0.0f)
            surfaceNormals[i] /= len;
    }
    
    // set up VAO and VBO and EBO and NBO
    glGenVertexArrays(1, &VAO); 
    glBindVertexArray(VAO);

    GLuint normalVBO;
    glGenBuffers(1, &normalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f) * mesh.NV(), &mesh.VN(0), GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, surfaceNormals.size() * sizeof(cy::Vec3f), surfaceNormals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1); // Assuming attribute index 1 for normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    //glBufferData(GL_ARRAY_BUFFER, nodes.size() * sizeof(cy::Vec3f), nodes.data(), GL_STATIC_DRAW);
    // allocate an empty buffer of the right size, with DYNAMIC_DRAW
    glBufferData(
        GL_ARRAY_BUFFER,
        nodes.size() * sizeof(cy::Vec3f),
        nullptr,
        GL_DYNAMIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh.NF() * 3, &mesh.F(0), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, surfaceIndices.size() * sizeof(unsigned int), surfaceIndices.data(), GL_STATIC_DRAW);

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


    // physics stuff: set up mass points and springs
    mpoints.reserve(nodes.size());

    for (auto &p : nodes) {
        MassPoint mp;
        mp.position = p;
        mp.mass     = 1.0f;
        mp.fixed    = false;
        mpoints.push_back(mp);
    }

    // say you want the top 1/3 fixed:
    const float fraction = 1.0f/3.0f;

    // first find min and max y
    float yMin = mpoints[0].position.y;
    float yMax = yMin;
    for (auto &mp : mpoints) {
        yMin = std::min(yMin, mp.position.y);
        yMax = std::max(yMax, mp.position.y);
    }
    // cutoff: everything above (1–fraction) up from yMin → fix the top fraction
    float cutoff = yMin + (yMax - yMin)*(1.0f - fraction);

    for (auto &mp : mpoints) {
        if (mp.position.y >= cutoff) {
            mp.fixed = true;
        }
    }

    std::vector<std::pair<int,int>> rawEdges;
    rawEdges.reserve(tetrahedra.size()*6);

    for (auto &T : tetrahedra) {
        int v[4] = {T.v[0], T.v[1], T.v[2], T.v[3]};
        // the 6 edges
        rawEdges.emplace_back(std::min(v[0],v[1]), std::max(v[0],v[1]));
        rawEdges.emplace_back(std::min(v[0],v[2]), std::max(v[0],v[2]));
        rawEdges.emplace_back(std::min(v[0],v[3]), std::max(v[0],v[3]));
        rawEdges.emplace_back(std::min(v[1],v[2]), std::max(v[1],v[2]));
        rawEdges.emplace_back(std::min(v[1],v[3]), std::max(v[1],v[3]));
        rawEdges.emplace_back(std::min(v[2],v[3]), std::max(v[2],v[3]));
    }

    // sort & unique
    std::sort(rawEdges.begin(), rawEdges.end());
    rawEdges.erase(std::unique(rawEdges.begin(), rawEdges.end()), rawEdges.end());

    springs.reserve(rawEdges.size());

    for (auto &e : rawEdges) {
        Spring s;
        s.a = e.first;
        s.b = e.second;
        // rest length from initial positions
        s.restLength = (mpoints[s.a].position - mpoints[s.b].position).Length();
        s.stiffness  = 0.2f;
        s.damping    = 0.01f;
        springs.push_back(s);
    }


    // Enter the GLUT event loop
    glutMainLoop();

    return 0;
}
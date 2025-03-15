#ifndef MODELS_H
#define MODELS_H

#include <vector>
#include <iostream>


namespace Models {

    using namespace std;

    std::vector<float> planeVertices = {
        // positions          // texture Coords 
         5.0f, -1.5f,  5.0f,  1.0f, 1.0f,
        -5.0f, -1.5f,  5.0f,  0.0f, 1.0f,
        -5.0f, -1.5f, -5.0f,  0.0f, 0.0f,

         5.0f, -1.5f, -5.0f,  1.0f, 0.0f,
         5.0f, -1.5f,  5.0f,  1.0f, 1.0f,
        -5.0f, -1.5f, -5.0f,  0.0f, 0.0f
    };

    int loadModel(int argc, char** argv, cy::TriMesh & mesh, float & scaleFactor) {
        char * modelName;
        int num_vertices;
    
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

        return num_vertices;
    
    }




} // namespace models

#endif 
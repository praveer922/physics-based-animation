#ifndef MODELS_H
#define MODELS_H

#include <vector>
#include <iostream>
#include <algorithm>
#include <unordered_map>


namespace Models {

    using namespace std;

    std::vector<float> planeVertices = {
        // positions for a floor at y = -10.0f
         50.0f, -30.0f,  50.0f,
        -50.0f, -30.0f,  50.0f,
        -50.0f, -30.0f, -50.0f,

         50.0f, -30.0f, -50.0f,
         50.0f, -30.0f,  50.0f,
        -50.0f, -30.0f, -50.0f
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

    struct Tetrahedron {
        int v[4]; // indices to the node list
    };
    
    bool loadNodes(const std::string &nodeFile, std::vector<cy::Vec3f>& nodes, cy::Vec3f &centroid) {
        std::ifstream inFile(nodeFile);
        if (!inFile) {
            std::cerr << "Cannot open node file " << nodeFile << std::endl;
            return false;
        }
        int numNodes, dim, numAttr, hasBoundary;
        std::string header;
        std::getline(inFile, header); // read header line (if present, you may need to adjust if comments exist)
        std::istringstream headerStream(header);
        headerStream >> numNodes >> dim >> numAttr >> hasBoundary;
    
        nodes.reserve(numNodes);
        for (int i = 0; i < numNodes; i++) {
            int index;
            cy::Vec3f node;
            inFile >> index >> node.x >> node.y >> node.z;
            // Optionally read extra attributes...
            nodes.push_back(node);
        }

        for (const auto &p : nodes) {
            centroid += p;
        }
        centroid /= (float)nodes.size();
        
        return true;
    }
    
    bool loadTetrahedra(const std::string &tetFile, std::vector<Tetrahedron>& tets) {
        std::ifstream inFile(tetFile);
        if (!inFile) {
            std::cerr << "Cannot open tetrahedral file " << tetFile << std::endl;
            return false;
        }
        int numTets, nodesPerTet, dummy;
        std::string header;
        std::getline(inFile, header);
        std::istringstream headerStream(header);
        headerStream >> numTets >> nodesPerTet >> dummy;
    
        if (nodesPerTet != 4) {
            std::cerr << "Unexpected number of nodes per tetrahedron: " << nodesPerTet << std::endl;
            return false;
        }
    
        tets.reserve(numTets);
        for (int i = 0; i < numTets; i++) {
            Tetrahedron tet;
            int tetIndex;
            inFile >> tetIndex >> tet.v[0] >> tet.v[1] >> tet.v[2] >> tet.v[3];
            // Convert from 1-indexed to 0-indexed
            tet.v[0]--;
            tet.v[1]--;
            tet.v[2]--;
            tet.v[3]--;
            tets.push_back(tet);
        }

        return true;
    }


    // Define a Face struct that always stores vertices sorted
    struct Face {
        int a, b, c;
        Face(int i, int j, int k) {
            int arr[3] = { i, j, k };
            std::sort(arr, arr + 3);
            a = arr[0];
            b = arr[1];
            c = arr[2];
        }
        
        bool operator==(const Face &other) const {
            return a == other.a && b == other.b && c == other.c;
        }
        
        // operator< allows sorting by comparing lexicographically
        bool operator<(const Face &other) const {
            if (a != other.a)
                return a < other.a;
            if (b != other.b)
                return b < other.b;
            return c < other.c;
        }
    };

    std::vector<Face> extractSurfaceFaces(const std::vector<Models::Tetrahedron> &tets) {
        std::vector<Face> allFaces;
        allFaces.reserve(tets.size() * 4);  // each tetrahedron contributes 4 faces
    
        // Insert all faces from each tetrahedron into the vector.
        for (const auto &tet : tets) {
            allFaces.push_back(Face(tet.v[0], tet.v[1], tet.v[2]));
            allFaces.push_back(Face(tet.v[0], tet.v[1], tet.v[3]));
            allFaces.push_back(Face(tet.v[0], tet.v[2], tet.v[3]));
            allFaces.push_back(Face(tet.v[1], tet.v[2], tet.v[3]));
        }
    
        // Sort the faces
        std::sort(allFaces.begin(), allFaces.end());
    
        // Iterate over sorted faces to extract those that occur exactly once.
        std::vector<Face> surfaceFaces;
        for (size_t i = 0; i < allFaces.size(); ) {
            // Count how many times the current face appears.
            size_t j = i + 1;
            while (j < allFaces.size() && allFaces[i] == allFaces[j])
                ++j;
            
            // If the face appears exactly once, add it to the surface faces.
            if (j - i == 1)
                surfaceFaces.push_back(allFaces[i]);
            
            i = j;  // Move to the next distinct face.
        }
        
        return surfaceFaces;
    }

} // namespace models

#endif 
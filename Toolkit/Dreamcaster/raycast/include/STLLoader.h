#ifndef STL_LOADER
#define STL_LOADER

#include "../include/common.h"
#include <string>
#include <vector>

//using namespace Raytracer;

/**
* Load mesh from stl file by its file name.
* @param filename file name of stl file
* @return 0 if succed, error code else
*/
int readSTLFile(std::string filename, std::vector<triangle*> & stlMesh, std::vector<iAVec3*> & vertices, aabb & box);

#endif

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iADreamCasterCommon.h"

#include <vector>

class QString;

//! Load mesh from stl file by its file name.
//! @param filename file name of stl file
//! @param stlMesh the triangles of the mesh
//! @param vertices the vertices of the mesh
//! @param box the bounding box of the mesh
//! @return 0 if reading succeeded, otherwise an error code
int readSTLFile(QString const & filename, std::vector<iAtriangle*> & stlMesh, std::vector<iAVec3f*> & vertices, iAaabb & box);

/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "../include/STLLoader.h"
#include "../../dreamcaster.h"
#include <fstream>
#include <vtksys/SystemTools.hxx>

extern DreamCaster * dcast;

/*std::vector<triangle*>		stlMesh; ///< loaded mesh's triangles vector
std::vector<iAVec3*>		vertices;///< loaded mesh's vertices vector
float scale_coef;///< loaded mesh's scale coefficient
float translate3f[3];///< loaded mesh's axes offsets
aabb box;///< loaded mesh's AABB*/

/*std::vector<triangle*>& getLoadedMesh(){return stlMesh;}
std::vector<iAVec3*>& getLoadedVertices(){return vertices;}
aabb& getBBox()
{
	return box;
}
float getScaleCoef(void) {return scale_coef;}
float* getTranslate(void) {return translate3f;}*/

#define BINARY_FILE 0
#define ASCII_FILE 1

inline void computeBBox(std::vector<triangle*> & stlMesh, std::vector<iAVec3*> & vertices, aabb & box, float & scale_coef, float * translate3f)
{
	float x1=vertices[0]->operator[](0), x2=vertices[0]->operator[](0),\
		  y1=vertices[0]->operator[](1), y2=vertices[0]->operator[](1),\
		  z1=vertices[0]->operator[](2), z2=vertices[0]->operator[](2);
	for (unsigned int i=0; i<vertices.size(); i++)
	{
		if(x1>vertices[i]->operator[](0))
			x1 = vertices[i]->operator[](0);
		if(x2<vertices[i]->operator[](0))
			x2 = vertices[i]->operator[](0);

		if(y1>vertices[i]->operator[](1))
			y1 = vertices[i]->operator[](1);
		if(y2<vertices[i]->operator[](1))
			y2 = vertices[i]->operator[](1);

		if(z1>vertices[i]->operator[](2))
			z1 = vertices[i]->operator[](2);
		if(z2<vertices[i]->operator[](2))
			z2 = vertices[i]->operator[](2);
	}
	//TODO: krivo
	if((x2-x1)<1 && (y2-y1)<1 && (z2-z1)<1)
	{
		int rescaleCoef = 1000;
		dcast->stngs.SCALE_COEF *= rescaleCoef;
		dcast->stngs.COLORING_COEF *= rescaleCoef;
		dcast->stngs.ORIGIN_Z  /= rescaleCoef;
		dcast->stngs.PLANE_Z   /= rescaleCoef;
		dcast->stngs.PLANE_H_W /= rescaleCoef;
		dcast->stngs.PLANE_H_H /= rescaleCoef;
	}
	float eps = 0.15f/dcast->stngs.SCALE_COEF;
	box.setData(x1-eps,x2+eps,y1-eps,y2+eps,z1-eps,z2+eps);
	iAMat4 matrix = ScaleAndCentreBBox(box, &scale_coef, translate3f);
	for (unsigned int i=0; i<vertices.size(); i++)
	{
		(*vertices[i]) = matrix*(*vertices[i]);
	}
	//
	x1=vertices[0]->operator[](0), x2=vertices[0]->operator[](0);
	y1=vertices[0]->operator[](1), y2=vertices[0]->operator[](1);
	z1=vertices[0]->operator[](2), z2=vertices[0]->operator[](2);
	for (unsigned int i=0; i<vertices.size(); i++)
	{
		if(x1>vertices[i]->operator[](0))
			x1 = vertices[i]->operator[](0);
		if(x2<vertices[i]->operator[](0))
			x2 = vertices[i]->operator[](0);

		if(y1>vertices[i]->operator[](1))
			y1 = vertices[i]->operator[](1);
		if(y2<vertices[i]->operator[](1))
			y2 = vertices[i]->operator[](1);

		if(z1>vertices[i]->operator[](2))
			z1 = vertices[i]->operator[](2);
		if(z2<vertices[i]->operator[](2))
			z2 = vertices[i]->operator[](2);
	}
	//TODO: krivo
	eps = 0.15f/dcast->stngs.SCALE_COEF;
	box.setData(x1-eps,x2+eps,y1-eps,y2+eps,z1-eps,z2+eps);
}
/**	\class Tri.
	\brief Structure representing triangle of stl file.

	Just a container. Nothing special. Contains data about triangle's normal and 3 vetrices.	
*/
struct Tri
{
	float normal1;
	float normal2;
	float normal3;

	float vertex1X;        float vertex1Y;        float vertex1Z;
	float vertex2X;        float vertex2Y;        float vertex2Z;
	float vertex3X;        float vertex3Y;        float vertex3Z;
};

int readSTLFile(std::string filename, std::vector<triangle*> & stlMesh, std::vector<iAVec3*> & vertices, aabb & box)
{
	float scale_coef;///< loaded mesh's scale coefficient
	float translate3f[3];///< loaded mesh's axes offsets
	std::ifstream reader;///< reader used to load mesh from stl file
	//clear prev model data
	for (unsigned int i=0; i<vertices.size(); i++)
	{
		iAVec3* vert = vertices[i];
		if(vert) 
			delete vert;
	}
	vertices.clear();
	for (unsigned int i=0; i<stlMesh.size(); i++)
	{
		triangle* tri = stlMesh[i];
		if(tri) 
			delete tri;
	}
	stlMesh.clear();
	//
	if( !filename.empty() )
		reader.open( filename.c_str() );
	else
	{
		printf("Error! Cannot open an .STL file.\n");
		dcast->log("Error! Cannot open an .STL file.");
		return 1;
	}
		
	// safety check to ensure that the file pointer opened is valid
	if (!reader.is_open())
	{
		printf("Error! Cannot open an .STL file.\n");
		dcast->log("Error! Cannot open an .STL file.");
		return 2; 
	}

	// just blindly read in the entire file till eof in to a vector
	std::vector<std::string> Word;
	// File reading and breaking the file into white-space seperated words
	// then storing it in the container object Word
	std::string line;
	while(!reader.eof())
	{
		reader >> line;
		Word.push_back(line);
	}
	reader.close();
	reader.clear();

	// This is the standard format of the stl file
	// it starts with a keyword FACET
	// then stores the normals of a given triangle with the keyword NORMAL
	// the three vertices ( !! clockwise ? ) each starting with the
	// keyword VERTEX
	// all the normals are enclosed within words OUTER LOOP - ENDLOOP
	// every triangle is ended with a ENDFACET keyword

	/* *************************************
	FACET NORMAL Nx Ny Nz
		OUTER LOOP
			VERTEX X1 Y1 Z1
			VERTEX X2 Y2 Z2
			VERTEX X3 Y3 Z3
		ENDLOOP
	ENDFACET 
	* ***************************************/

	// a total of 4 sets of SbVec3f's 1 for the each of the vertices
	// and 1 for the normals.
	// also have a string to indentify the keyword
	std::string name;
	//cVector dummy1, dummy2, dummy3;
	//vector3 v1, v2;
	// THE CODE IN THIS IF READS THE ASCII FILE
	triangle*	copy;

	//Determine the type of the file
	int type;
	vtksys::SystemTools::FileTypeEnum ft =
		vtksys::SystemTools::DetectFileType(filename.c_str());
	switch(ft)
	{
	case vtksys::SystemTools::FileTypeBinary:
		type = BINARY_FILE;
		break;
	case vtksys::SystemTools::FileTypeText:
		type = ASCII_FILE;
		break;
	default:
		return 2;
	}
	// text file
	if (type == ASCII_FILE)
	{	
		iAVec3 helper_vec3;
		for(unsigned int i = 0; i < Word.size(); i++)
		{
			if(Word[i] == "solid")
			{
				name = Word[i+1];
			}
			if(Word[i] == "facet" && Word[i+1] == "normal")
			{
				// create a fresh pointer to triangle
				copy = new triangle;

				//copy->m_N = iAVec3( (float)atof(Word[i+2].c_str()),					// THIS NORMAL CAN BE WRONG :)
				//				  (float)atof(Word[i+3].c_str()),
				//				  (float)atof(Word[i+4].c_str()) );
				copy->vertices[0] = new iAVec3((float)atof(Word[i+8].c_str()),
											(float)atof(Word[i+9].c_str()),
											(float)atof(Word[i+10].c_str())	);
				vertices.push_back(copy->vertices[0]);

				copy->vertices[1] = new iAVec3((float)atof(Word[i+12].c_str()),
											(float)atof(Word[i+13].c_str()),
											(float)atof(Word[i+14].c_str())	);
				vertices.push_back(copy->vertices[1]);

				copy->vertices[2] = new iAVec3((float)atof(Word[i+16].c_str()),
											(float)atof(Word[i+17].c_str()),
											(float)atof(Word[i+18].c_str())	);
				vertices.push_back(copy->vertices[2]);

				// IN THE FOLLOWINGF LINES OF CODE WE REEVALUATE THE NORMAL AS THE ABOVE NORMAL IS PROBLEM SOME
				copy->N =  (*copy->vertices[2]-*copy->vertices[1])
					        ^(*copy->vertices[0]-*copy->vertices[2]);
				normalize(copy->N);// RESETTING THE NORMAL HERE
	
				//counter++;
				stlMesh.push_back( copy );
			}
		}

		// Pump all the values from the STL code in to a "double" array to store
		// vertex values, then we can render the same data using vertex arrays
		Word.clear();
		computeBBox(stlMesh, vertices, box, scale_coef, translate3f);
		return 0;
	}
	// HERE FOR THE BINARY VERSION 
	else
	{
		struct Tri item;
		FILE *fptr = fopen(filename.c_str(),"rb");
		if(!fptr)	return 1;
		unsigned char header[80];
		if (fread(&header, sizeof(char), 80, fptr) != 80)
		{
			printf("Error! Cannot read .STL file header.\n");
			dcast->log("Error! Cannot read .STL file header.\n");
			return 4;
		}
		unsigned long noOfFacets;
		if (fread(&noOfFacets, sizeof(unsigned long), 1, fptr) != 1)
		{
			printf("Error! Cannot read .STL file header.\n");
			dcast->log("Error! Cannot read .STL file header.\n");
			return 4;
		}

		unsigned short zero;

		long count = 0;
		for (unsigned long i=0;i<noOfFacets;i++)
		{
			if ((fread(&item,sizeof(item),1,fptr) != 1) ||				// reads triangle
				(fread(&zero,sizeof(unsigned short),1,fptr) != 1))		// reads an unsigned short present after every triangle
			{
				printf("Error! Problem reading .STL file content.\n");
				dcast->log("Error! Cannot read .STL file content.\n");
				return 4;
			}

			copy = new triangle;

			copy->N = iAVec3(item.normal1, item.normal2, item.normal3 );

			iAVec3 helper_vec3;
			helper_vec3 = iAVec3(item.vertex1X, item.vertex1Y, item.vertex1Z );
			copy->vertices[0] = new iAVec3(helper_vec3);
			vertices.push_back(copy->vertices[0]);
			
			helper_vec3 = iAVec3(item.vertex2X, item.vertex2Y, item.vertex2Z );
			copy->vertices[1] = new iAVec3(helper_vec3);
			vertices.push_back(copy->vertices[1]);

			helper_vec3 = iAVec3(item.vertex3X, item.vertex3Y, item.vertex3Z );
			copy->vertices[2] = new iAVec3(helper_vec3);
			vertices.push_back(copy->vertices[2]);
			
			// RESETTING THE NORMAL HERE
			copy->N =  (*copy->vertices[2]-*copy->vertices[1])
						^(*copy->vertices[0]-*copy->vertices[2]);
			normalize(copy->N);

			count++;

			stlMesh.push_back( copy );
		}
		fclose(fptr);
		computeBBox(stlMesh, vertices, box, scale_coef, translate3f);
		return 0;
	}
	return 3;
}

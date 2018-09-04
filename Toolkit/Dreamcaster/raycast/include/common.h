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
#pragma once

#include "iAmat4.h"

#include <vector>
#include <qmath.h>

#define max_macro(a,b)  (((a) > (b)) ? (a) : (b))
#define min_macro(a,b)  (((a) < (b)) ? (a) : (b))

inline float Rand( float a_Range );

//namespace Raytracer {
	//#define SQRDISTANCE(A,B) ((A.x-B.x)*(A.x-B.x)+(A.y-B.y)*(A.y-B.y)+(A.z-B.z)*(A.z-B.z))
	//main application settings stored in registry
	struct SETTINGS
	{
		SETTINGS()
		{
			EPSILON		= 0.0011f;
			TRACEDEPTH		= 6;

			THREAD_GRID_X	= 4;
			THREAD_GRID_Y	= 4;
			SCALE_COEF	= 1.0f;
			COLORING_COEF = 0.007f;

			RFRAME_W			= 128;
			RFRAME_H			= 128;
			VFRAME_W			= 140;
			VFRAME_H			= 140;

			ORIGIN_Z		= -469.3f;
			PLANE_Z		= 1069.299f;
			PLANE_H_W		= 204.8f;
			PLANE_H_H		= 204.8f;

			TREE_L1			= 2;
			TREE_L2			= 17;
			TREE_L3			= 25;
			TREE_SPLIT1		= 10000;
			TREE_SPLIT2		= 100000;

			BG_COL_R = 50;
			BG_COL_G = 50;
			BG_COL_B = 50;

			PLATE_COL_R = 50;
			PLATE_COL_G = 150;
			PLATE_COL_B = 50;

			COL_RANGE_MIN_R = 255;
			COL_RANGE_MIN_G = 255;
			COL_RANGE_MIN_B = 255;

			COL_RANGE_MAX_R = 255;
			COL_RANGE_MAX_G = 0;
			COL_RANGE_MAX_B = 0;

			BATCH_SIZE = 10;
			MIN_TRI_PER_NODE = 1;
			USE_SAH = 0;
		}
		float EPSILON;
		int TRACEDEPTH;

		int THREAD_GRID_X;
		int THREAD_GRID_Y;
		float SCALE_COEF;
		float COLORING_COEF;

		int RFRAME_W;
		int RFRAME_H;
		int VFRAME_W;
		int VFRAME_H;

		float ORIGIN_Z;
		float PLANE_Z;
		float PLANE_H_W;
		float PLANE_H_H;

		int TREE_L1;
		int TREE_L2;
		int TREE_L3;
		int TREE_SPLIT1;
		int TREE_SPLIT2;

		unsigned int BG_COL_R;
		unsigned int BG_COL_G;
		unsigned int BG_COL_B;

		//Plate color on 3d view
		unsigned int PLATE_COL_R;
		unsigned int PLATE_COL_G;
		unsigned int PLATE_COL_B;

		//Color table min value
		unsigned int COL_RANGE_MIN_R;
		unsigned int COL_RANGE_MIN_G;
		unsigned int COL_RANGE_MIN_B;

		//Color table max value
		unsigned int COL_RANGE_MAX_R;
		unsigned int COL_RANGE_MAX_G;
		unsigned int COL_RANGE_MAX_B;

		int COL_RANGE_DR;
		int COL_RANGE_DG;
		int COL_RANGE_DB;
		//Batch size (number of frames per pass) of CUDA rendering
		unsigned int BATCH_SIZE;
		//Minimum number of triangles in kd-tree node, if less, then node is a leaf
		unsigned int MIN_TRI_PER_NODE;
		//Use SAH when building kd-tree or not
		unsigned int USE_SAH;
		//#define SQRDISTANCE(A,B) ((A.x-B.x)*(A.x-B.x)+(A.y-B.y)*(A.y-B.y)+(A.z-B.z)*(A.z-B.z))
	};
	/**
	* Parses config (from local config store). Initializes some variables.
	* @param a cfg_filename filename of config file.
	*/
	int ParseConfigFile(SETTINGS * settings);
	/**
	* Converts radians in degrees.
	* @param rad radians value.
	* @return degrees value
	*/
	inline double rad2deg(float rad)
	{
		return rad*180/M_PI;
	}
	/**
	* Converts degrees in radians .
	* @param degrees value.
	* @return rads value
	*/
	inline double deg2rad(float deg)
	{
		return (deg/180.0f)*M_PI;
	}
	inline void Time2Char(int ftime, char *t)
	{
		t[6] = (ftime / 100) % 10 + '0';
		t[7] = (ftime / 10) % 10 + '0';
		t[8] = (ftime % 10) + '0';
		int secs = (ftime / 1000) % 60;
		int mins = (ftime / 60000) % 100;
		t[3] = ((secs / 10) % 10) + '0';
		t[4] = (secs % 10) + '0';
		t[1] = (mins % 10) + '0';
		t[0] = ((mins / 10) % 10) + '0';
	}
	iAVec3 projectPtOnLine(iAVec3 &o, iAVec3 &dir, iAVec3& pt);
	float distPointToLine(iAVec3 &o, iAVec3 &dir, iAVec3& pt);
	float distLineToLine( iAVec3 & o1, iAVec3 & d1, iAVec3 & o2, iAVec3 & d2 );
	/**	\struct arotation_tabb.
	\brief Structure representing combination of parameters in single placement.
	*/
	struct parameters_t
	{
		parameters_t(double a_av_pen_len, double a_av_dip_ang, double a_max_pen_len, double a_badSurfPrcnt) :
			avPenLen(a_av_pen_len), 
			avDipAng(a_av_dip_ang), 
			maxPenLen(a_max_pen_len),
			badAreaPercentage(a_badSurfPrcnt)
		{}
		parameters_t() :
			avPenLen(0.0), 
			avDipAng(0.0), 
			maxPenLen(0.0),
			badAreaPercentage(0.0)
		{}
		double& operator[] (int index)
		{
			switch(index)
			{
			case 0:
				return avPenLen;
				break;
			case 1:
				return avDipAng;
				break;
			case 2:
				return maxPenLen;
			    break;
			case 3:
				return badAreaPercentage;
			    break;
			}
			return avPenLen;
			/*if(index==0)
				return avPenLen;
			else if(index==1)
				return avDipAng;
			return maxPenLen;*/
		}
		double avPenLen;
		double avDipAng;
		double maxPenLen;	
		double badAreaPercentage;
	};

	/**	\struct rotation_t.
	\brief Structure representing rotations of rendering about x, y, z axes in radians.
	*/
	struct rotation_t
	{
		rotation_t() :rotX(0), rotY(0), rotZ(0) {}
		rotation_t(float a_rotX, float a_rotY, float a_rotZ) :rotX(a_rotX), rotY(a_rotY), rotZ(a_rotZ) {}
		float rotX, rotY, rotZ;
	};
	/**	\struct aabb.
		\brief Structure representing axis aligned bounding box.

		Has ranges of each of 3 axes values, center coordinates, half-dimensions, index of maximum dimension.	
	*/
	struct aabb
	{
		aabb();
		aabb(aabb& el);
		aabb(float a_x1, float a_x2, float a_y1, float a_y2, float a_z1, float a_z2);
		/**
		* Determines if v is inside AABB.
		* @note if point is on bound it considered to be inside AABB
		* @return 1 if inside, 0 otherwise
		*/
		inline int isInside(iAVec3& v) const
		{
			if (v.x()<=x2 && v.x()>=x1 &&
			    v.y()<=y2 && v.y()>=y1 &&
				v.z()<=z2 && v.z()>=z1)
				return 1;
			return 0;
		}
		void setData(float a_x1, float a_x2, float a_y1, float a_y2, float a_z1, float a_z2);
		void setData(const aabb & el);
		/**
		* Calculate center vector of bb.
		*/
		iAVec3 center() const;
		/**
		* Calculate half size vector of bb.
		*/
		iAVec3 half_size() const;
		/**
		* Calculates mainDim member (index of maximum dimension).
		*/
		int mainDim();
		float surfaceArea();

		float x1,x2,y1,y2,z1,z2;
	};

	iAMat4 ScaleAndCentreBBox(aabb &box, float *scale_coef_out=0, float* translate3f_out = 0);
	class Vertex
	{
	public:
		Vertex() {};
		Vertex( iAVec3 a_Pos ) : m_Pos( a_Pos ) {};
		//float GetU() { return m_U; }
		//float GetV() { return m_V; }
		iAVec3& GetNormal() { return m_Normal; }
		iAVec3& GetPos() { return m_Pos; }
		//void SetUV( float a_U, float a_V ) { m_U = a_U; m_V = a_V; }
		void SetPos( iAVec3& a_Pos ) { m_Pos = a_Pos; }
		void SetNormal( iAVec3& a_N ) { m_Normal = a_N; }
	private:
		iAVec3 m_Pos;
		iAVec3 m_Normal;
	};
	/**	\struct triangle.
		\brief Class representing triangle in 3d space.

		Containing all parameters needed for triangle description.	
	*/
	struct triangle
	{
		triangle( iAVec3* a_V1, iAVec3* a_V2, iAVec3* a_V3)
		{
			vertices[0] = a_V1;
			vertices[1] = a_V2;
			vertices[2] = a_V3;
		};
		triangle()
		{
			vertices[0] = 0;
			vertices[1] = 0;
			vertices[2] = 0;
		};
		iAVec3 N;
		iAVec3 *vertices[3];// 12
	};

	struct wald_tri
	{
		iAVec3 m_N;
		iAVec3 m_A;
		float nu, nv, nd;
		unsigned int k;	
		float bnu, bnv;
		float cnu, cnv;	
	};
	struct ct_state{
		iAVec3 o;///< rays origin
		iAVec3 c;///< corner of plate
		iAVec3 dx;///< dx of plane in 3d
		iAVec3 dy;///< dy of plane in 3d
	};
	/**	\struct plane.
	\brief Class representing plane in 3d space.

	Containing all parameters needed for plane description.	
	*/
	class plane
	{
	public:
		plane() : N( 0, 0, 0 ), D( 0 ) {};
		plane( iAVec3 a_Normal, float a_D ) : N( a_Normal ), D( a_D ) {};
		iAVec3 N;
		float D;
	};

	struct ModelData 
	{
		std::vector<triangle*>		stlMesh; ///< loaded mesh's triangles vector
		std::vector<iAVec3*>		vertices;///< loaded mesh's vertices vector
		aabb box;///< loaded mesh's aabb
	};

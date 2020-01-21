/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <QThread>

#include "iADreamCasterCommon.h"
#include "iADataFormat.h"

#include "cl_common.h"

//! Class representing a ray in 3D.
class iARay
{
public:
	iARay() : m_Origin( iAVec3f( 0, 0, 0 ) ), m_Direction( iAVec3f( 0, 0, 0 ) ) {};
	iARay( iAVec3f & a_Origin, iAVec3f & a_Dir );
	iARay( const iAVec3f * a_Origin, iAVec3f & a_Dir );
	void SetOrigin( iAVec3f & a_Origin ) { m_Origin = a_Origin; }
	void SetDirection( iAVec3f & a_Direction ) { m_Direction = a_Direction; }
	inline const iAVec3f & GetOrigin() const { return m_Origin; }
	inline const iAVec3f & GetDirection() const { return m_Direction; }
private:
	iAVec3f m_Origin;    //!< ray origin's position
	iAVec3f m_Direction; //!< ray direction vector
};

class iAScene;
class iARaycastingThread;
struct iATraverseStack;

//! Class in charge of the raycasting process; it is used to init the render system, start the rendering process and contains all scene data.
class iAEngine
{
	friend class iARaycastingThread;
public:
	iAEngine(iADreamCasterSettings * settings, float * dc_cuda_avpl_buff,	float * dc_cuda_dipang_buff );
	~iAEngine();
	//! Sets the render target canvas.
	//! @param a_Dest image pixel buffer.
	void SetTarget( unsigned int* a_Dest);
	//! Get engine's scene.
	//! @return pointer to scene class
	iAScene* scene() { return m_Scene; }
	//! Raytrace single ray.
	//! @note not used (using thread->DepthRaytrace(...) instead)
	int DepthRaytrace (iARay& a_Ray, iAVec3f & a_Acc, int a_Depth, float a_RIndex, float& a_Dist, iARayPenetration * ray_p, std::vector<iAIntersection*> &vecIntersections, iATraverseStack * stack, bool dipAsColor=false );
	//! Initializes the renderer, by resetting line / tile counters´(=render parameters) and precalculating some values.
	//! Prepares transformation matrix which is applied to origin and screen plane.
	//! @param vp_corners [out] plane's corners in 3d
	//! @param vp_delta [out] plane's x and y axes' directions in 3D
	//! @param o [out] ray's origin point in world coordinates
	void InitRender(iAVec3f * vp_corners, iAVec3f * vp_delta, iAVec3f * o);
	//! Transforms vector corresponding to rotation and position
	//! @param vec Vector to transform
	void Transform(iAVec3f * vec);
	//! Render engine's scene.
	//! @param vp_corners
	//! @param vp_delta
	//! @param o
	//! @param rememberData remember data.
	//! @param dipAsColor draw image colored corresponding to dip angles.
	//! @param cuda_enabled use cuda based code for rendering
	//! @param rasterization whether to use rasterization
	//! @return true
	bool Render(const iAVec3f * vp_corners, const iAVec3f * vp_delta, const iAVec3f * o, bool rememberData = true, bool dipAsColor = false, bool cuda_enabled=false, bool rasterization = false);
	//! Render scene on CPU
	bool RenderCPU(const iAVec3f * vp_corners, const iAVec3f * vp_delta, const iAVec3f * o, bool rememberData = true, bool dipAsColor = false);
	//! Render scene on GPU
	bool RenderGPU(const iAVec3f * vp_corners, const iAVec3f * vp_delta, const iAVec3f * o, bool rememberData = true, bool dipAsColor = false, bool rasterization = false);
	bool RenderBatchGPU(unsigned int batchSize, iAVec3f *os, iAVec3f * corns, iAVec3f * deltaxs, iAVec3f * deltays, float * rotsX, float * rotsY, float * rotsZ, bool rememberData = true, bool dipAsColor = false);
	//! Get ray traced image pixel buffer.
	unsigned int* getBuffer(){return m_Dest;}
	//! Set camera rotations.
	void setRotations(float a_X, float a_Y, float a_Z=0);
	//! Set object's position.
	void setPositon(float* pos);
	//! Get last rendering's average penetration length.
	float getLastAvPenetrLen(void) {return m_lastAvPenetrLen;}
	//! Get last rendering's average dip angle.
	float getLastAvDipAngle(void) {return m_lastAvDipAngle;}
	//! Set pointer on list of cut AABBs.
	//! @param cutAABBList the list of cut AABB's
	void SetCutAABBList(std::vector<iAaabb*> * cutAABBList)
	{
		if(cutAABBList==0)
		{
			m_cutAABBList = 0;
			m_cutAABBListSize = 0;
		}
		else
		{
			m_cutAABBList = cutAABBList;
			m_cutAABBListSize = (unsigned int) m_cutAABBList->size();
			if(m_cut_AABBs)
			{
				delete [] m_cut_AABBs;
				m_cut_AABBs = 0;
			}
			m_cut_AABBs = new iAaabb[m_cutAABBListSize];
			for (unsigned int i=0; i<m_cutAABBListSize; i++)
			{
				m_cut_AABBs[i].setData( *( (*cutAABBList)[i] ) );
			}
		}
	}
	iARenderFromPosition curRender; //!< statistical data about current(last) scene render
	iARenderFromPosition * curBatchRenders; //!< statistical data about current(last) scene render
protected:
	// renderer data
	float m_WX1, m_WY1, m_WX2, m_WY2, m_DX, m_DY, m_PLANE_Z, m_ORIGIN_Z;//, m_SX, m_SY;
	//TODO: merge
	std::vector<iAaabb*> * m_cutAABBList;
	unsigned int m_cutAABBListSize;
	iAaabb* m_cut_AABBs;
	iAScene* m_Scene; //!< engine's scene
	unsigned int* m_Dest;  //!< ray traced image pixel buffer
	int m_Width, m_Height;
	//threading
	int msecs;
	float rotX, rotY, rotZ;  //!< camera's rotation about x, y and z axis
	iAVec3f position;        //!< object's position
	float m_lastAvPenetrLen; //!< last rendering's av. penetration length
	float m_lastAvDipAngle;  //!< last rendering's av. dip angle
	unsigned int m_batchSize;
	float * cuda_avpl_buff;  //!<float buffer used by cuda to store the results recieved from DreamCaster
	float * cuda_dipang_buff;//!<float buffer used by cuda to store the results recieved from DreamCaster
	iADreamCasterSettings * s;

//! Properties and methods for OpenCL raycasting
private://properties
	//OpenCL
	cl::Device m_device;
	cl::Context m_context;
	cl::CommandQueue m_queue;

	//Buffers
	cl::Buffer nodes;//unsigned int
	cl::Buffer tris;//float4
	cl::Buffer ids;//unsigned int

	//kernels
	cl::Kernel K_raycast_batch;
	cl::Buffer os, cs, dxs, dys, cl_aabb, cut_aabbs;//K_raycast_batch in
	cl::Buffer device_out_data, device_out_dip;//K_raycast_batch out

private://methods
	void InitOpenCL();
public://TODO: qndh
	void AllocateOpenCLBuffers();
	void setup_nodes( void * data );
	void setup_tris ( void * data );
	void setup_ids  ( void * data );
private:
	// Raycast batch GPU
	void raycast_batch(
		const void *a_aabb,
		const void * a_o,
		const void * a_c,
		const void * a_dx,
		const void * a_dy,
		const int w, const int h,
		const unsigned int batchSize,
		const void * a_cut_aabbs,
		const unsigned int a_cut_aabbs_count,
		float* out_res,
		float * out_dip_res );

	// Raycast GPU
	void raycast_single(
		const void *a_aabb,
		const void * a_o,
		const void * a_c,
		const void * a_dx,
		const void * a_dy,
		const int w, const int h,
		const void * a_cut_aabbs,
		const unsigned int a_cut_aabbs_count,
		float* out_res,
		float * out_dip_res );
};

//! Class in charge of raycasting process. Executes raycastring on tile prescribed by X,Y screen coordinates ranges(in pixels).
//! Used directly for rendering process. Has a pointer on parent Engine class, which is used to obtain scene data and to store some results.
class iARaycastingThread : public QThread
{
public:
	iARaycastingThread(iAEngine* a_engine){
		e = a_engine;
	};
	iARaycastingThread(){
		e = 0;
		rays = 0;
	};
	~iARaycastingThread(){
	};
	//! Sets parent Engine pointer.
	inline void setEngine(iAEngine* a_e)
	{
		e=a_e;
	}
	//! Sets position of plane and ray's origin.
	inline void setPlaneAndOrigin( const iAVec3f *vp_corners, const iAVec3f *vp_delta, const iAVec3f *o)
	{
		m_o = o;
		m_vp_corners = vp_corners;
		m_vp_delta = vp_delta;
	}
	//! Gets rays' penetrations data.
	inline iARayPenetration * getRays()
	{
		return rays;
	}
	//! Gets intersections data.
	inline std::vector<iAIntersection*> * getIntersections()
	{
		return &intersections;
	}
	//! Runs thread, executes raycasting of thread's tile.
	void run();
	void stop()
	{
		stopped = true;
	}
	//! Free memory that has been allocated for rays and intersections.
	void clearMemory()
	{
		if(rays)
			delete [] rays;
		for (unsigned int i=0; i<intersections.size(); i++)
			delete intersections[i];
	}
	int x1,x2,y1,y2;
	int rayCount;    //!< number of casted rays
	bool dipAsColor; //!< image colored corresponding to dip angles
private:
	const iAVec3f *m_o;                       //!< rays' origin point
	const iAVec3f *m_vp_corners;              //!< plane's corners in 3d
	const iAVec3f *m_vp_delta;                //!< plane's x and y axes' directions in 3D
	iAEngine* e;                                //!< parent Engine
	iARayPenetration * rays;                    //!< rays' penetrations data
	std::vector<iAIntersection*> intersections; //!< intersections data
	volatile bool stopped;
};

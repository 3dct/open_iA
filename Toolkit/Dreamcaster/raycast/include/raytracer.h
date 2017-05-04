/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include <QVariant>
#include <QThread>

#include "common.h"
#include "DataFormat.h"

#include "cl_common.h"

/**	\class Ray.
	\brief Class representing ray in 3D.

	Detailed description.	
*/
class Ray
{
public:
	Ray() : m_Origin( iAVec3( 0, 0, 0 ) ), m_Direction( iAVec3( 0, 0, 0 ) ) {};
	Ray( iAVec3& a_Origin, iAVec3& a_Dir );
	Ray( const iAVec3 * a_Origin, iAVec3& a_Dir );
	void SetOrigin( iAVec3& a_Origin ) { m_Origin = a_Origin; }
	void SetDirection( iAVec3& a_Direction ) { m_Direction = a_Direction; }
	inline const iAVec3& GetOrigin() const { return m_Origin; }
	inline const iAVec3& GetDirection() const { return m_Direction; }
private:
	iAVec3 m_Origin;		///< ray origin's position
	iAVec3 m_Direction;	///< ray direction vector
};


class Scene;
class RaycastingThread;
struct traverse_stack;
/**	\class Engine.
	\brief A class is in charge with raycasting process.

	Is used to init render system, start rendering process. Also contains all scene data.	
*/
class Engine
{
	friend class RaycastingThread;
public:
	Engine( SETTINGS * settings, float * dc_cuda_avpl_buff,	float * dc_cuda_dipang_buff );
	~Engine();
	/**
	* Sets the render target canvas.
	* @param a_Dest image pixel buffer.
	* @param a_Width pixel buffer width.
	* @param a_Height pixel buffer height.
	*/
	void SetTarget( unsigned int* a_Dest);
	/**
	* Get engine's scene.
	* @return pointer to scene class
	*/
	Scene* GetScene() { return m_Scene; }
	/**
	* Raytrace single ray. 
	* @note not used (using thread->DepthRaytrace(...) instead)
	*/
	int DepthRaytrace ( Ray& a_Ray, iAVec3& a_Acc, int a_Depth, float a_RIndex, float& a_Dist, RayPenetration * ray_p, std::vector<Intersection*> &vecIntersections, traverse_stack * stack, bool dipAsColor=false ); 
	/**
	* Initializes the renderer, by resetting render parameters and precalculating some values.
	* Prepares transformation matrix which is applied to origin and screen plane.
	* @param vp_corners [out] plane's corners in 3d
	* @param vp_delta [out] plane's x and y axes' directions in 3D
	* @param o [out] ray's origin point in world coordinates
	*/
	void InitRender(iAVec3 * vp_corners, iAVec3 * vp_delta, iAVec3 * o);
	/**
	* Transforms vector corresponding to rotation and position
	*/
	void Transform(iAVec3 * vec);
	/**
	* Render engine's scene.
	* @param rememberData remember data.
	* @param dipAsColor draw image colored corresponding to dip angles.
	* @param cuda_enabled use cuda based code for rendering
	* @return true
	*/
	bool Render(const iAVec3 * vp_corners, const iAVec3 * vp_delta, const iAVec3 * o, bool rememberData = true, bool dipAsColor = false, bool cuda_enabled=false, bool rasterization = false);
	bool RenderCPU(const iAVec3 * vp_corners, const iAVec3 * vp_delta, const iAVec3 * o, bool rememberData = true, bool dipAsColor = false);
	bool RenderGPU(const iAVec3 * vp_corners, const iAVec3 * vp_delta, const iAVec3 * o, bool rememberData = true, bool dipAsColor = false, bool rasterization = false);
	bool RenderBatchGPU(unsigned int batchSize, iAVec3 *os, iAVec3 * corns, iAVec3 * deltaxs, iAVec3 * deltays, float * rotsX, float * rotsY, float * rotsZ, bool rememberData = true, bool dipAsColor = false);
	/**
	* Get ray traced image pixel buffer.
	*/
	unsigned int* getBuffer(){return m_Dest;}
	/**
	* Set camera rotations.
	*/
	void setRotations(float a_X, float a_Y, float a_Z=0);
	/**
	* Set object's position.
	*/
	void setPositon(float* pos);
	/**
	* Get last rendering's average penetration length.
	*/
	float getLastAvPenetrLen(void) {return m_lastAvPenetrLen;}
	/**
	* Get last rendering's average dip angle.
	*/
	float getLastAvDipAngle(void) {return m_lastAvDipAngle;}
	/**
	* Set pointer on list of cut AABBs.
	*/
	void SetCutAABBList(std::vector<aabb*> * cutAABBList) 
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
			m_cut_AABBs = new aabb[m_cutAABBListSize];
			for (unsigned int i=0; i<m_cutAABBListSize; i++)
			{
				m_cut_AABBs[i].setData( *( (*cutAABBList)[i] ) );
			}
		}
	}
	//
	RenderFromPosition curRender; ///< statistical data about current(last) scene render
	RenderFromPosition * curBatchRenders; ///< statistical data about current(last) scene render
	//std::vector<RenderFromPosition*> * renders;
// 	float WX1()   {return m_WX1;}
// 	float WY1()   {return m_WY1;}
// 	float Z()     {return m_Z;}
// 	float DX()    {return m_DX;}
// 	float DY()    {return m_DY;}
// 	int   Width() {return m_Width;}
// 	int   Height(){return m_Height;}
	//iAVec3 o; ///< rays' origin point
	//iAVec3 vp_corners[2];///< plane's corners in 3d
	//iAVec3 vp_delta[2];///< plane's x and y axes' directions in 3D
protected:
	// renderer data
	float m_WX1, m_WY1, m_WX2, m_WY2, m_DX, m_DY, m_PLANE_Z, m_ORIGIN_Z;//, m_SX, m_SY;
	//
	//Mat4 rot_mat;  ///< rotations matrix	
	//
	//TODO: merge
	std::vector<aabb*> * m_cutAABBList;
	unsigned int m_cutAABBListSize;
	aabb* m_cut_AABBs;
	Scene* m_Scene; ///< engine's scene
	unsigned int* m_Dest;  ///< ray traced image pixel buffer
	int m_Width, m_Height;
	//threading
	int msecs;
	float rotX;		///< camera's rotation about x axis
	float rotY;		///< camera's rotation about y axis
	float rotZ;		///< camera's rotation about z axis
	iAVec3 position;	///< object's position
	float m_lastAvPenetrLen;///< last rendering's av. penetration length
	float m_lastAvDipAngle;	///< last rendering's av. dip angle
	unsigned int m_batchSize;
	float * cuda_avpl_buff;///<float buffer used by cuda to store the results recieved from DreamCaster
	float * cuda_dipang_buff;///<float buffer used by cuda to store the results recieved from DreamCaster
	SETTINGS * s;

/**
* Prpoperties and methods for OpenCL raycasting
*/
private://properties
	//OpenCL
	cl::Device			m_device;
	cl::Context			m_context;
	cl::CommandQueue	m_queue;

	//Buffers
	cl::Buffer		nodes;//unsigned int
	cl::Buffer		tris;//float4
	cl::Buffer		ids;//unsigned int

	//kernels
	cl::Kernel		K_raycast_batch;
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
	//**	Raycast batch GPU
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

	//**	Raycast GPU
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

/**	\class RaycastingThread.
\brief A class is in charge with raycasting process.
  Class ingerited from QThread. Executes raycastring on tile prescribed by X,Y screen coordinates ranges(in pixels).

  Used directly for rendering process. Has a pointer on parent Engine class, which is used to obtain scene data and to store some results.	
*/
class RaycastingThread : public QThread 
{ 
public: 
	RaycastingThread(Engine* a_engine){
		e = a_engine;
	}; 
	RaycastingThread(){
		e = 0;
		rays = 0;
	}; 
	~RaycastingThread(){
		//if(rays)
		//	delete[]rays;
	}; 
	/**
	* Sets parent Engine pointer.
	*/
	inline void setEngine(Engine* a_e)
	{
		e=a_e;
	}
	/**
	* Sets position of plane and ray's origin.
	*/
	inline void setPlaneAndOrigin( const iAVec3 *vp_corners, const iAVec3 *vp_delta, const iAVec3 *o)
	{
		m_o = o;
		m_vp_corners = vp_corners;
		m_vp_delta = vp_delta;
	}
	/**
	* Gets rays' penetrations data.
	*/
	inline RayPenetration * getRays()
	{
		return rays;
	}
	/**
	* Gets intersections data.
	*/
	inline std::vector<Intersection*> * getIntersections()
	{
		return &intersections;
	}
	/**
	* Runs thread, executes raycasting of thread's tile.
	*/
	void run();
	void stop()
	{
		stopped = true;
	}
	/**
	* Free memory that have been allocated for rays and intersections.
	*/
	void clearMemory()
	{
		if(rays)
			delete [] rays;
		for (unsigned int i=0; i<intersections.size(); i++)
			delete intersections[i];
	}
	int x1,x2,y1,y2;
	int rayCount;  ///< number of casted rays
	bool dipAsColor;  ///< image colored corresponding to dip angles
private: 
	const iAVec3 *m_o; ///< rays' origin point
	const iAVec3 *m_vp_corners;///< plane's corners in 3d
	const iAVec3 *m_vp_delta;///< plane's x and y axes' directions in 3D
	Engine* e; ///< parent Engine 
	RayPenetration * rays; ///< rays' penetrations data
	std::vector<Intersection*> intersections; ///< intersections data
	volatile bool stopped; 
}; //Thread

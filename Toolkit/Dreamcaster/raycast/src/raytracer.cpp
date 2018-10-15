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
#include "../include/raytracer.h"
#include "../include/scene.h"
#include "../include/common.h"
#include "../include/BSPTree.h"
#include <algorithm>
#include <vector>
#include <QFile>

#define MAX_CUT_AAB_COUNT 10

///#include "../../enable_memleak.h"
static const char * clDreamcaster_Source[] = {
#include "../../OpenCL/dreamcaster_embedded.txt"
};


//namespace Raytracer {

Ray::Ray( iAVec3& a_Origin, iAVec3& a_Dir ) : 
	m_Origin( a_Origin ), 
	m_Direction( a_Dir )
{}

Ray::Ray(const iAVec3 * a_Origin, iAVec3& a_Dir ) : 
	m_Origin( *a_Origin ), 
	m_Direction( a_Dir )
{}

Engine::Engine(SETTINGS * settings, float * dc_cuda_avpl_buff,	float * dc_cuda_dipang_buff  )
{
	cuda_avpl_buff = dc_cuda_avpl_buff;
	cuda_dipang_buff = dc_cuda_dipang_buff;
	m_batchSize = settings->BATCH_SIZE;
	m_Scene = new Scene();
	//renders = a_renders;
	position[0] = position[1] = position[2] = 0.0f;
	curBatchRenders = new RenderFromPosition[m_batchSize];
	m_cut_AABBs = 0;
	m_cutAABBList = 0;
	m_cutAABBListSize = 0;
	s = settings;
	InitOpenCL();
}

Engine::~Engine()
{
	if(m_Scene)
		delete m_Scene;
	if(curBatchRenders)
		delete [] curBatchRenders;
	if(m_cut_AABBs)
		delete [] m_cut_AABBs;
}
// -----------------------------------------------------------
// Engine::SetTarget
// Sets the render target canvas
// -----------------------------------------------------------
void Engine::setRotations(float a_X, float a_Y, float a_Z)
{
	//inverted because we use origin and plane rotations instead of model rotation
	rotX = a_X;
	rotY = a_Y;
	rotZ = a_Z;
}
void Engine::setPositon( float* pos )
{
	for(unsigned int i=0; i<3; i++)
		position[i] = pos[i];
}
// -----------------------------------------------------------
// Engine::SetTarget
// Sets the render target canvas
// -----------------------------------------------------------
void Engine::SetTarget( unsigned int* a_Dest)
{
	// set pixel buffer address & size
	m_Dest = a_Dest;
	m_Width = s->RFRAME_W;
	m_Height = s->RFRAME_H;
	// screen plane in world space coordinates
	m_WX1 = -s->PLANE_H_W, m_WX2 = s->PLANE_H_W, m_WY1 = s->PLANE_H_H, m_WY2 = -s->PLANE_H_H;
	m_PLANE_Z = s->PLANE_Z;
	m_ORIGIN_Z = s->ORIGIN_Z;
	// calculate deltas for interpolation
	m_DX = (m_WX2 - m_WX1) / m_Width;
	m_DY = (m_WY2 - m_WY1) / m_Height;
}

// -----------------------------------------------------------
// Engine::Raytrace
// Naive ray tracing: Intersects the ray with every primitive
// in the scene to determine the closest intersection
// -----------------------------------------------------------
bool intersectionCompare( intersection *e1, intersection *e2 )
{
	return e1->dist < e2->dist;
}

int Engine::DepthRaytrace( Ray& a_Ray, iAVec3& a_Acc, int a_Depth, float a_RIndex, float& a_Dist, RayPenetration * ray_p, std::vector<Intersection*> &vecIntersections, traverse_stack * stack, bool dipAsColor )
{
	if (a_Depth > s->TRACEDEPTH) return 0;
	// trace primary ray
	a_Dist = 1000000.0f;
	iAVec3 pi;
	std::vector<intersection*> intersections;
	// find intersections
	unsigned int cutAABBListSize;
	if(m_cutAABBList)
		cutAABBListSize = m_cutAABBListSize;
	else 
		cutAABBListSize = 0;
	if(cutAABBListSize)
	{
		bool intersects = false;
		for (unsigned int i=0; i<cutAABBListSize; i++)
		{
			float a,b;
			//if(IntersectCyl(a_Ray, *((*m_cutAABBList)[i]), a, b, 1)) 
			if(IntersectAABB(a_Ray, *((*m_cutAABBList)[i]), a, b)) 
			{
				intersects = true;
				break;
			}
		}
		if(!intersects)
			return 0;
	}
	m_Scene->getBSPTree()->GetIntersectionsNR(a_Ray, intersections,stack);
	if(intersections.size()==0)
		return 0;
	std::sort(intersections.begin(), intersections.end(), intersectionCompare);
	//delete coincident intersections
	//it happens when ray hits common edge of 2 neighboring triangles
	ray_p->penetrationsSize=0;
	ray_p->avDipAng=0;
	for (int i=0; i<(int)intersections.size()-1; ) 
	{
		/*if( intersections[i+1]->dist == intersections[i]->dist )
		{
			delete intersections[i]; // Remove the object
			intersections[i]=0;
			intersections.erase(intersections.begin() + i);
		} 
		else */
		//TODO: no triangles repeated?
		if( intersections[i+1]->tri->GetIndex() == intersections[i]->tri->GetIndex() )
		{
			delete intersections[i]; // Remove the object
			intersections[i]=0;
			intersections.erase(intersections.begin() + i);
		} 
		else 
			++i;
	}
	//
	unsigned int intetsectSize = (unsigned int) intersections.size();
	float penetrationDepth = 0;
	//Sometimes it happens, yet lets have this workaround
	if(intetsectSize%2 == 0)//TODO: temporary workaround
	for (unsigned int i=0; i<intetsectSize; i++)
	{
		if(i%2==1)
		{
			float dist = intersections[i]->dist - intersections[i-1]->dist;
			penetrationDepth += dist; 
			//ray_p->penetrations[ray_p->penetrationsSize] = dist;
			ray_p->penetrationsSize++;
			ray_p->totalPenetrLen += dist;
		}
		//add intersection in intersections vector
		TriPrim* tri = intersections[i]->tri;
		Intersection *isec = new Intersection(tri->GetIndex(), tri->GetAngleCos(a_Ray));
		vecIntersections.push_back(isec);
		ray_p->avDipAng+=fabs(isec->dip_angle);
	}
	ray_p->avDipAng/=intersections.size();

	//rayPenetr->totalPenetrLen/=(rayPenetr->penetrations.size());
	for (unsigned int i=0; i<intersections.size(); i++)
		delete intersections[i];
	float coef = penetrationDepth*s->COLORING_COEF;
	if(dipAsColor)
	{
		a_Acc = iAVec3((s->COL_RANGE_MIN_R+s->COL_RANGE_DR*(1-ray_p->avDipAng))/255.0, 
			(s->COL_RANGE_MIN_G+s->COL_RANGE_DG*(1-ray_p->avDipAng))/255.0,
			(s->COL_RANGE_MIN_B+s->COL_RANGE_DB*(1-ray_p->avDipAng))/255.0);
	}
	else
	{
		a_Acc = iAVec3(coef, coef, coef);
	}
	// return pointer to primitive hit by primary ray
	return 1;
}
// -----------------------------------------------------------
// Engine::InitRender
// Initializes the renderer, by resetting the line / tile
// counters and precalculating some values
// -----------------------------------------------------------
void Engine::InitRender(iAVec3 * vp_corners, iAVec3 * vp_delta, iAVec3 * o)
{
	//!@note rotations and translations are inversed, because we rotating plane and origin instead of object
	float 
		irotX = -rotX,
		irotY = -rotY,
		irotZ = -rotZ;
	iAVec3 iposition = -position;
	iAMat4 mrotx, mroty, mrotz;
	mrotz = rotationZ(irotZ);
	mroty = rotation(mrotz*iAVec3(0,1,0), irotY);
	mrotx = rotation(mrotz*iAVec3(1,0,0), irotX);
	iAMat4 rot_mat = mrotx*mroty*mrotz;
	//iAMat4 rot_mat = rotationX(irotX)*rotationY(irotY)*rotationZ(irotZ);
	//if plate rotation is desired, should do translation after rotation
	vp_corners[0] = rot_mat*(iAVec3(m_WX1, m_WY1, m_PLANE_Z)+iposition);
	vp_corners[1] = rot_mat*(iAVec3(m_WX2, m_WY2, m_PLANE_Z)+iposition);
	(*o) = rot_mat*(iAVec3( 0, 0, m_ORIGIN_Z )+iposition);
	//no translations here
	vp_delta[0] = rot_mat*iAVec3(m_DX, 0, 0);
	vp_delta[1] = rot_mat*iAVec3(0, m_DY, 0);
}

bool Engine::Render(const iAVec3 * vp_corners, const iAVec3 * vp_delta, const iAVec3 * o,  bool rememberData, bool dipAsColor, bool cuda_enabled, bool rasterization )
{
	if(cuda_enabled)
		return RenderGPU(vp_corners, vp_delta, o, rememberData, dipAsColor, rasterization);
	else
		return RenderCPU(vp_corners, vp_delta, o, rememberData, dipAsColor);
}
// -----------------------------------------------------------
// Engine::RenderCPU
// Render scene
// -----------------------------------------------------------
bool Engine::RenderCPU(const iAVec3 * vp_corners, const iAVec3 * vp_delta, const iAVec3 * o, bool rememberData, bool dipAsColor )
{
	curRender.clear();
	curRender.rotX = rotX;
	curRender.rotY = rotY;
	curRender.rotZ = rotZ;
	for(unsigned int i=0; i<3; i++)
		curRender.pos[i] = position[i];
	curRender.avPenetrLen = 0.f;
	curRender.maxPenetrLen = 0.f;
	curRender.avDipAngle = 0.f;

	int threadCount = s->THREAD_GRID_X*s->THREAD_GRID_Y;
	RaycastingThread * threads = new RaycastingThread[threadCount];
	for (int x=0; x<s->THREAD_GRID_X; x++)
		for (int y=0; y<s->THREAD_GRID_Y; y++)
		{
			int tIndex = y*s->THREAD_GRID_X+x; 
			threads[tIndex].setEngine(this);
			threads[tIndex].setPlaneAndOrigin(vp_corners, vp_delta, o);
			threads[tIndex].dipAsColor=dipAsColor;
			threads[tIndex].x1 =  x   *m_Width /s->THREAD_GRID_X;
			threads[tIndex].x2 = (x+1)*m_Width /s->THREAD_GRID_X;
			threads[tIndex].y1 =  y   *m_Height/s->THREAD_GRID_Y;
			threads[tIndex].y2 = (y+1)*m_Height/s->THREAD_GRID_Y;
		}
	for (int i=0; i<threadCount; i++)
		threads[i].start();
	for (int i=0; i<threadCount; i++)
		threads[i].wait();
	for (int i=0; i<threadCount; i++)
		threads[i].stop();
	//now extract all penetration data from threads
	float avPenetrLen=0;
	float avDipAngle=0;
	float maxPenetrLen=0;
	float raysCount=0;
	float isecCount=0;
	for (int i=0; i<threadCount; i++)
	{
		if(rememberData)
			curRender.rawPtrRaysVec.push_back(threads[i].getRays());
		for (int j=0; j<threads[i].rayCount; j++)
		{
			if(threads[i].getRays()[j].penetrationsSize!=0)
			{
				raysCount++;
				if(rememberData)
					curRender.rays.push_back(&threads[i].getRays()[j]);
				//(*renders)[curRend]->m_avPenetrLen+=threads[i].getRays()[j].totalPenetrLen;
				float curPenetrLen = threads[i].getRays()[j].totalPenetrLen;
				avPenetrLen+=curPenetrLen;
				if(curPenetrLen > maxPenetrLen)
					maxPenetrLen = curPenetrLen;
			}
		}
		size_t intersectionsCount = threads[i].getIntersections()->size();
		for (size_t j=0; j<intersectionsCount; j++)
		{
			isecCount++;
			if(rememberData)
				curRender.intersections.push_back( ( *(threads[i].getIntersections()) )[j] );
			avDipAngle+= fabs(( *(threads[i].getIntersections()) )[j]->dip_angle);
		}
	}
	curRender.raysSize = (unsigned int) curRender.rays.size();
	curRender.intersectionsSize = (unsigned int) curRender.intersections.size();
	avPenetrLen/=raysCount;
	avDipAngle/=isecCount;
	m_lastAvPenetrLen  = avPenetrLen;
	m_lastAvDipAngle = avDipAngle;
	//(*renders)[curRend]->m_avPenetrLen/=(*renders)[curRend]->rays.size();
	if(rememberData)
	{
		curRender.avPenetrLen=avPenetrLen;
		curRender.avDipAngle=avDipAngle;
		curRender.maxPenetrLen=maxPenetrLen;
		curRender.raysSize = (unsigned int) curRender.rays.size();
	}
	else
	{
		for (int i=0; i<threadCount; i++)
			threads[i].clearMemory();
	}
	delete [] threads;
	return true;
}
// -----------------------------------------------------------
// Engine::RenderCPU
// Render scene
// -----------------------------------------------------------
bool Engine::RenderGPU(const iAVec3 * vp_corners, const iAVec3 * vp_delta, const iAVec3 * o, bool rememberData, bool dipAsColor, bool rasteriztion )
{
	curRender.clear();
	curRender.rotX = rotX;
	curRender.rotY = rotY;
	curRender.rotZ = rotZ;
	for(unsigned int i=0; i<3; i++)
		curRender.pos[i] = position[i];
	curRender.avPenetrLen = 0.f;
	curRender.maxPenetrLen = 0.f;
	curRender.avDipAngle = 0.f;
	if (rasteriztion)
	{
/*
		iAMat4 mrotx, mroty, mrotz;
		mroty = rotationY(rotY);
		mrotx = rotation(mroty*iAVec3(1,0,0), rotX);
		mrotz = rotation(mroty*iAVec3(0,0,1), rotZ);
		iAMat4 rot_mat = mrotz*mrotx*mroty;
*/
/*
		float
			irotX = -rotX,
			irotY = -rotY,
			irotZ = -rotZ;
		//iAVec3 iposition = -position;
		iAMat4 mrotx, mroty, mrotz;
		mrotz = rotationZ(irotZ);
		mroty = rotation(mrotz*iAVec3(0,1,0), irotY);
		mrotx = rotation(mrotz*iAVec3(1,0,0), irotX);
		iAMat4 rot_mat = mrotx*mroty*mrotz;
		rot_mat.invert();
		//TODO!!! add translation
		iAVec3 origin = iAVec3( 0, 0, ORIGIN_Z );
		cuda_rasterize(&origin, &PLANE_Z, &PLANE_H_W, &rot_mat, &RFRAME_W, &RFRAME_H, (int)GetScene()->getNrTriangles(), cuda_avpl_buff);
*/
	}
	else
		raycast_single(
				&(GetScene()->getBSPTree()->m_aabb), 
				o, 
				vp_corners, 
				&vp_delta[0], 
				&vp_delta[1], 
				s->RFRAME_W, s->RFRAME_H, 
				m_cut_AABBs, 
				m_cutAABBListSize,
				cuda_avpl_buff, 
				cuda_dipang_buff);
	unsigned int col;
	unsigned int* buffer=getBuffer();
	float av_pl=0;
	unsigned int active_rays_count=0;
	for (int i=0; i<s->RFRAME_W*s->RFRAME_H; i++)
	{
		if(cuda_avpl_buff[i])
		{
			av_pl+=cuda_avpl_buff[i];
			active_rays_count++;
			if(dipAsColor)
			{
				*buffer++ = (
					((unsigned int)(s->COL_RANGE_MIN_R+s->COL_RANGE_DR*(1-cuda_dipang_buff[i])) << 16) +
					((unsigned int)(s->COL_RANGE_MIN_G+s->COL_RANGE_DG*(1-cuda_dipang_buff[i])) << 8) + 
					(unsigned int)(s->COL_RANGE_MIN_B+s->COL_RANGE_DB*(1-cuda_dipang_buff[i]))
					);
			}
			else
			{
				col = (unsigned int)(cuda_avpl_buff[i] * s->COLORING_COEF * 255);
				if(col>255)
					col = 255;
				col = 255 - col;
				*buffer++ = (col << 16) + (col << 8) + col;
			}
		}
		else
		{
			*buffer++ = 0xffffff;
		}
	}
	av_pl/=active_rays_count;
	//now extract all penetration data
	float s1_avPenetrLen=0;
	float s1_avDipAngle=0;
	float s1_maxPenetrLen=0;
	float raysCount=0;
	RayPenetration * rays = new RayPenetration[s->RFRAME_W*s->RFRAME_H];
	if(rememberData)
		curRender.rawPtrRaysVec.push_back(rays);
	for (int i=0; i<s->RFRAME_W*s->RFRAME_H; i++)
	{
		rays[i].m_X=i%s->RFRAME_W;
		rays[i].m_Y=i/s->RFRAME_W;
		rays[i].totalPenetrLen=0;
		rays[i].avDipAng=0;
		if(cuda_avpl_buff[i])
		{
			if(rememberData)
			{
				rays[i].totalPenetrLen = cuda_avpl_buff[i];
				rays[i].avDipAng = cuda_dipang_buff[i];
				curRender.rays.push_back(&rays[i]);
				s1_avPenetrLen+=cuda_avpl_buff[i];
				s1_avDipAngle+=cuda_dipang_buff[i];
				if(cuda_avpl_buff[i] > s1_maxPenetrLen)
					s1_maxPenetrLen = cuda_avpl_buff[i];
				if(cuda_dipang_buff[i]<0)
					cuda_dipang_buff[i]=cuda_dipang_buff[i];
			}
			raysCount++;		
		}
	}
	curRender.raysSize = (unsigned int) curRender.rays.size();
	curRender.intersectionsSize = (unsigned int) curRender.intersections.size();
	s1_avPenetrLen/=raysCount;
	s1_avDipAngle/=raysCount;
	m_lastAvPenetrLen  = s1_avPenetrLen;
	m_lastAvDipAngle = s1_avDipAngle;
	if(rememberData)
	{
		curRender.avPenetrLen=s1_avPenetrLen;
		curRender.avDipAngle=s1_avDipAngle;
		curRender.maxPenetrLen=s1_maxPenetrLen;
		curRender.raysSize = (unsigned int) curRender.rays.size();
	}
	else
	{
		//cleanup some not saved data
		delete [] rays;
	}
	return true;
}

bool Engine::RenderBatchGPU( unsigned int batchSize, iAVec3 *os, iAVec3 * corns, iAVec3 * deltaxs, iAVec3 * deltays, float * rotsX, float * rotsY, float * rotsZ, bool rememberData /*= true*/, bool dipAsColor /*= false*/)
{
	for (unsigned int batch=0; batch<batchSize; batch++)
	{
		curBatchRenders[batch].clear();
		curBatchRenders[batch].rotX = rotsX[batch];
		curBatchRenders[batch].rotY = rotsY[batch];
		curBatchRenders[batch].rotZ = rotsZ[batch];
		for(unsigned int i=0; i<3; i++)
			curBatchRenders[batch].pos[i] = position[i];
		curBatchRenders[batch].avPenetrLen = 0.f;
		curBatchRenders[batch].maxPenetrLen = 0.f;
		curBatchRenders[batch].avDipAngle = 0.f;
	}

	raycast_batch(
		&(GetScene()->getBSPTree()->m_aabb), 
		os, 
		corns, 
		deltaxs, deltays, 
		s->RFRAME_W, s->RFRAME_H,  
		batchSize, 
		m_cut_AABBs, 
		m_cutAABBListSize,
		cuda_avpl_buff, 
		cuda_dipang_buff);
	unsigned int col;
	unsigned int* buffer=getBuffer();
	float av_pl=0;
	unsigned int active_rays_count=0;
	unsigned int offset=0;
	unsigned int buff_start, buff_end;
	buff_start = s->RFRAME_W*s->RFRAME_H*(batchSize-1); buff_end = s->RFRAME_W*s->RFRAME_H*batchSize;
	for (unsigned int i=buff_start; i<buff_end; i++)
	{
		if(cuda_avpl_buff[i])
		{
			av_pl+=cuda_avpl_buff[i];
			active_rays_count++;
			if(dipAsColor)
			{
				*buffer++ = (
					((unsigned int)(s->COL_RANGE_MIN_R+s->COL_RANGE_DR*(1-cuda_dipang_buff[i])) << 16) +
					((unsigned int)(s->COL_RANGE_MIN_G+s->COL_RANGE_DG*(1-cuda_dipang_buff[i])) << 8) + 
					(unsigned int)(s->COL_RANGE_MIN_B+s->COL_RANGE_DB*(1-cuda_dipang_buff[i]))
					);
			}
			else
			{
				col = (unsigned int)(cuda_avpl_buff[i] * s->COLORING_COEF * 255);
				if(col>255)
					col = 255;
				col = 255 - col;
				*buffer++ = (col << 16) + (col << 8) + col;
			}
		}
		else
		{
			*buffer++ = 0xffffff;
		}
	}
	av_pl/=active_rays_count;
	//now extract all penetration data of all renderings
	for (unsigned int batch=0; batch<batchSize; batch++)
	{
		float s2_avPenetrLen=0;
		float s2_avDipAngle=0;
		float s2_maxPenetrLen=0;
		float raysCount=0;
		RayPenetration * rays=new RayPenetration[s->RFRAME_W*s->RFRAME_H];
		if(rememberData)
			curBatchRenders[batch].rawPtrRaysVec.push_back(rays);
		for (int i=0, ioffset=offset; i<s->RFRAME_W*s->RFRAME_H; i++, ioffset++)
		{
			rays[i].m_X=i%s->RFRAME_W;
			rays[i].m_Y=i/s->RFRAME_W;
			rays[i].totalPenetrLen=0;
			rays[i].avDipAng=0;
			if(cuda_avpl_buff[ioffset])
			{
				if(rememberData)
				{
					rays[i].totalPenetrLen = cuda_avpl_buff[ioffset];
					rays[i].avDipAng = cuda_dipang_buff[ioffset];
					curBatchRenders[batch].rays.push_back(&rays[i]);
					s2_avPenetrLen+=cuda_avpl_buff[ioffset];
					if(cuda_avpl_buff[ioffset] > s2_maxPenetrLen)
						s2_maxPenetrLen = cuda_avpl_buff[ioffset];
					s2_avDipAngle+=cuda_dipang_buff[ioffset];
					//if(cuda_dipang_buff[ioffset]<0)
					//	cuda_dipang_buff[ioffset]=cuda_dipang_buff[ioffset];
				}
				raysCount++;		
			}
		}
		curBatchRenders[batch].raysSize = (unsigned int) curBatchRenders[batch].rays.size();
		curBatchRenders[batch].intersectionsSize = (unsigned int) curBatchRenders[batch].intersections.size();
		s2_avPenetrLen /= raysCount;
		s2_avDipAngle /= raysCount;
		m_lastAvPenetrLen  = s2_avPenetrLen;
		m_lastAvDipAngle = s2_avDipAngle;
		if(rememberData)
		{
			curBatchRenders[batch].avPenetrLen = s2_avPenetrLen;
			curBatchRenders[batch].avDipAngle = s2_avDipAngle;
			curBatchRenders[batch].maxPenetrLen = s2_maxPenetrLen;
		}
		else
		{
			//cleanup some not saved data
			delete [] rays;
		}
		offset += s->RFRAME_W * s->RFRAME_H;
	}
	return true;
}

void Engine::Transform( iAVec3 * vec )
{
	//!@note rotations and translations are inversed, because we rotating plane and origin instead of object
	float 
		irotX = -rotX,
		irotY = -rotY,
		irotZ = -rotZ;
	iAVec3 iposition = -position;
	iAMat4 mrotx, mroty, mrotz;
	mrotz = rotationZ(irotZ);
	mroty = rotation(mrotz*iAVec3(0,1,0), irotY);
	mrotx = rotation(mrotz*iAVec3(1,0,0), irotX);
	iAMat4 rot_mat = mrotx*mroty*mrotz;
	//if plate rotation is desired, should do translation after rotation
	(*vec) = rot_mat*((*vec)+iposition);
}

void Engine::InitOpenCL()
{
	cl_init(m_device, m_context, m_queue);
	std::string				clDC(clDreamcaster_Source[0]);
	cl::Program::Sources	clDCSrc = cl::Program::Sources(1, std::make_pair(clDC.c_str(), clDC.size() )  );

	cl::Program	clDCProg(m_context, clDCSrc);
	cl_int error;
	std::vector<cl::Device>	devices; devices.push_back(m_device);
	error = clDCProg.build(devices);//, clOptions);
	if (error != CL_SUCCESS) 
	{
		cl_int error2;
		std::string str = clDCProg.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device, &error2);
		itk_clSafeCall( error2 );
		itk_clThrowBuildLog( str );
	}
	itk_clSafeCall(error);

	// create cl kernels
	K_raycast_batch = cl::Kernel(clDCProg, "raycast_batch", &error);
	itk_clSafeCall(error);
}

void Engine::AllocateOpenCLBuffers()
{
	const size_t tri_count = m_Scene->getNrTriangles();
	const size_t nodes_count = m_Scene->getBSPTree()->nodes.size();
	const size_t id_count = m_Scene->getBSPTree()->tri_ind.size();

	const size_t batchSize = s->BATCH_SIZE;
	const size_t out_size = s->RFRAME_W * s->RFRAME_H * s->BATCH_SIZE;
	
	cl_int error;
	nodes = cl::Buffer( m_context, CL_MEM_READ_ONLY, nodes_count*sizeof(BSPNode), 0, &error );
	itk_clSafeCall( error );
	tris = cl::Buffer( m_context, CL_MEM_READ_ONLY, tri_count*sizeof(wald_tri), 0, &error );
	itk_clSafeCall( error );
	ids = cl::Buffer( m_context, CL_MEM_READ_ONLY, id_count*sizeof(unsigned int), 0, &error );
	itk_clSafeCall( error );


	//in
	os = cl::Buffer( m_context, CL_MEM_READ_ONLY, batchSize * 3*sizeof(cl_float), 0, &error );
	itk_clSafeCall( error );
	cs = cl::Buffer( m_context, CL_MEM_READ_ONLY, batchSize * 3*sizeof(cl_float), 0, &error );
	itk_clSafeCall( error );
	dxs = cl::Buffer( m_context, CL_MEM_READ_ONLY, batchSize * 3*sizeof(cl_float), 0, &error );
	itk_clSafeCall( error );
	dys = cl::Buffer( m_context, CL_MEM_READ_ONLY, batchSize * 3*sizeof(cl_float), 0, &error );
	itk_clSafeCall( error );
	cl_aabb = cl::Buffer(m_context, CL_MEM_READ_ONLY, sizeof(aabb), 0, &error );
	itk_clSafeCall( error );

	//out
	device_out_data = cl::Buffer(m_context, CL_MEM_READ_WRITE, 
		out_size*sizeof(float), 0, &error );
	itk_clSafeCall( error );
	device_out_dip = cl::Buffer(m_context, CL_MEM_READ_WRITE, 
		out_size*sizeof(float), 0, &error );
	itk_clSafeCall( error );
}

void Engine::setup_nodes( void * data )
{
	const size_t size = m_Scene->getBSPTree()->nodes.size();
	itk_clSafeCall(
		m_queue.enqueueWriteBuffer(	nodes, cl_bool(true), 0, size * sizeof(BSPNode), data )  
	);
	itk_clSafeCall( m_queue.finish() );	
}

void Engine::setup_tris( void * data )
{
	const size_t size = m_Scene->getNrTriangles();
	itk_clSafeCall(
		m_queue.enqueueWriteBuffer(	tris, cl_bool(true), 0, size*sizeof(wald_tri),	data )  
	);
	itk_clSafeCall( m_queue.finish() );	
}

void Engine::setup_ids( void * data )
{
	const size_t size = m_Scene->getBSPTree()->tri_ind.size();
	itk_clSafeCall(
		m_queue.enqueueWriteBuffer(	ids, cl_bool(true), 0, size*sizeof(unsigned int), data )  
	);
	itk_clSafeCall( m_queue.finish() );	
}

void Engine::raycast_batch(	
	const void *a_aabb, 
	const void * a_o, const void * a_c, 
	const void * a_dx, const void * a_dy, 
	const int w, const int h, 
	const unsigned int batchSize, 
	const void * a_cut_aabbs, 
	const unsigned int a_cut_aabbs_count,
	float * out_res, 
	float * out_dip_res )
{
 	cl_int error;
 	size_t out_size = w*h*batchSize;
	//in
	itk_clSafeCall(
		m_queue.enqueueWriteBuffer(	os, cl_bool(true), 0, batchSize * 3*sizeof(cl_float), (void*)a_o )  
	);
	itk_clSafeCall(
		m_queue.enqueueWriteBuffer(	cs, cl_bool(true), 0, batchSize * 3*sizeof(cl_float), (void*)a_c )  
	);
	itk_clSafeCall(
		m_queue.enqueueWriteBuffer(	dxs, cl_bool(true), 0, batchSize * 3*sizeof(cl_float), (void*)a_dx )  
	);
	itk_clSafeCall(
		m_queue.enqueueWriteBuffer(	dys, cl_bool(true), 0, batchSize * 3*sizeof(cl_float), (void*)a_dy )  
	);
	itk_clSafeCall(
		m_queue.enqueueWriteBuffer(	cl_aabb, cl_bool(true), 0, sizeof(aabb), (void*)a_aabb )  
	);
	if(a_cut_aabbs_count > 0)
	{
		cut_aabbs = cl::Buffer(m_context, CL_MEM_READ_ONLY, a_cut_aabbs_count*sizeof(aabb), 0, &error );
		itk_clSafeCall( error );
		itk_clSafeCall(
			m_queue.enqueueWriteBuffer(	cut_aabbs, cl_bool(true), 0, a_cut_aabbs_count*sizeof(aabb), (void*)a_cut_aabbs )  
			);
	}
	itk_clSafeCall( m_queue.finish() );	

	//setup and run kernel
	cl_uint idx = 0;
	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	w)					);
	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	h)					);
	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	device_out_data)	);
	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	device_out_dip)		);
	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	os)					);
	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	cs)					);
	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	dxs)				);
	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	dys)				);
	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	cl_aabb)			);
	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	cut_aabbs)			);
	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	a_cut_aabbs_count)	);
 	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	nodes)				);
 	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	tris)				);
	itk_clSafeCall(  K_raycast_batch.setArg(idx++,	ids)				);

	size_t global = w * h * batchSize;
	size_t local = GetLocalForKernel( K_raycast_batch, global, m_device);

	error = m_queue.enqueueNDRangeKernel(
		K_raycast_batch,
		cl::NullRange, 
		cl::NDRange(global), 
		cl::NDRange(local) );
	itk_clSafeCall( error );
	itk_clSafeCall( m_queue.finish() );

	//copy results from GPU to CPU
	itk_clSafeCall(
		m_queue.enqueueReadBuffer(	device_out_data,
		cl_bool(true),
		0, 
		out_size*sizeof(float),
		out_res )  );
	itk_clSafeCall( m_queue.finish() );
	itk_clSafeCall(
		m_queue.enqueueReadBuffer(	device_out_dip,
		cl_bool(true),
		0, 
		out_size*sizeof(float),
		out_dip_res )  );
	itk_clSafeCall( m_queue.finish() );
}

void Engine::raycast_single(
	const void *a_aabb, 
	const void * a_o, const void * a_c, 
	const void * a_dx, const void * a_dy, 
	const int w, const int h, 
	const void * a_cut_aabbs, 
	const unsigned int a_cut_aabbs_count, 
	float * out_res, 
	float * out_dip_res	)
{
	this->raycast_batch(a_aabb, a_o, a_c, a_dx, a_dy, w, h, 1, a_cut_aabbs, a_cut_aabbs_count, out_res, out_dip_res);
}

//////////////////////////////////////////////////////////////////////////
//RaycastingThread implementation
//////////////////////////////////////////////////////////////////////////
void RaycastingThread::run()
{
	traverse_stack * tr_stack = new traverse_stack(e->GetScene()->getBSPTree()->splitLevel+1);
	rayCount = (x2-x1)*(y2-y1);
	delete [] rays;
	rays = new RayPenetration[rayCount];
	unsigned int rayInd=0;
	for(int x=x1; x<x2; x++)
		for(int y=y1; y<y2; y++)
		{
			rays[rayInd].m_X=x;
			rays[rayInd].m_Y=y;
			rays[rayInd].totalPenetrLen=0.0f;
			// fire primary ray
			iAVec3 acc( 0, 0, 0 );
			iAVec3 dir = (m_vp_corners[0] + x*m_vp_delta[0] + y*m_vp_delta[1]) - (*m_o);
			normalize( dir );
			Ray r( m_o, dir );
			float dist;
			e->DepthRaytrace( r, acc, 1, 1.0f, dist, &rays[rayInd], intersections, tr_stack, dipAsColor );
			int red = (int)(acc[0] * 255);
			int green = (int)(acc[1] * 255);
			int blue = (int)(acc[2] * 255);
			if (red > 255) red = 255;
			if (green > 255) green = 255;
			if (blue > 255) blue = 255;

			//e->destMutex.lock();
			//invert by y axis
			e->m_Dest[y*e->m_Width+(e->m_Width-x-1)] = (red << 16) + (green << 8) + blue;
			//e->destMutex.unlock();
			rayInd++;
		}
	if(tr_stack)
		delete tr_stack;
}
//}; // namespace Raytracer

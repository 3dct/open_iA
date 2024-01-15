// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iARayTracer.h"

// Intersection method return values
#define HIT		 1		// Ray hit primitive
#define MISS	 0		// Ray missed primitive
#define INPRIM	-1		// Ray started inside primitive

class iATriPrim;

//! Structure representing intersection data, contains data about primitives.
struct iAintersection
{
	iATriPrim* tri;
	float dist;
	iAintersection(iATriPrim* a_tri, float a_dist)
	{
		tri = a_tri;
		dist = a_dist;
	}
};

//! Ray-AABB intersection routine.
//! @param ray ray class.
//! @param box axis aligned bounding box structure.
//! @param tmin parent AABB min t.
//! @param tmax parent AABB max t.
//! @return
//!    1 - if ray intersects AABB
//!    0 - otherwise
inline int IntersectAABB(const iARay &ray, const iAaabb& box, float &tmin, float&tmax)
{
	float txmin, txmax, tymin, tymax;
	float ddx = 1.0f/ray.GetDirection().x();
	float ddy = 1.0f/ray.GetDirection().y();
	if(ddx>=0)
	{
		txmin = (box.x1 - ray.GetOrigin().x()) * ddx;
		txmax = (box.x2 - ray.GetOrigin().x()) * ddx;
	}
	else
	{
		txmin = (box.x2 - ray.GetOrigin().x()) * ddx;
		txmax = (box.x1 - ray.GetOrigin().x()) * ddx;
	}
	if(ddy>=0)
	{
		tymin = (box.y1 - ray.GetOrigin().y()) * ddy;
		tymax = (box.y2 - ray.GetOrigin().y()) * ddy;
	}
	else
	{
		tymin = (box.y2 - ray.GetOrigin().y()) * ddy;
		tymax = (box.y1 - ray.GetOrigin().y()) * ddy;
	}
	if( (txmin>tymax) || (tymin>txmax) ) return 0;
	if( tymin>txmin ) txmin=tymin;
	if( tymax<txmax ) txmax=tymax;

	float tzmin, tzmax;
	float ddz = 1.0f/ray.GetDirection().z();
	if(ddz>=0)
	{
		tzmin = (box.z1 - ray.GetOrigin().z()) * ddz;
		tzmax = (box.z2 - ray.GetOrigin().z()) * ddz;
	}
	else
	{
		tzmin = (box.z2 - ray.GetOrigin().z()) * ddz;
		tzmax = (box.z1 - ray.GetOrigin().z()) * ddz;
	}
	if( (txmin>tzmax) || (tzmin>txmax) ) return 0;
	if( tzmin>txmin ) txmin=tzmin;
	if( tzmax<txmax ) txmax=tzmax;
	tmin=txmin;
	tmax=txmax;
	return 1;
}

//! Ray-AABB intersection routine.
//! @param ro ray's origin position.
//! @param rd ray's distance.
//! @param box axis aligned bounding box structure.
//! @return
//!    1 - if ray intersects AABB
//!    0 - otherwise
inline int Intersect(iAVec3f & ro, iAVec3f rd, const iAaabb& box)
{
	float txmin, txmax, tymin, tymax;
	float ddx = 1.0f/(ro.x()-rd.x());
	float ddy = 1.0f/(ro.y()-rd.y());
	if(ddx>=0)
	{
		txmin = (box.x1 - ro.x()) * ddx;
		txmax = (box.x2 - ro.x()) * ddx;
	}
	else
	{
		txmin = (box.x2 - ro.x()) * ddx;
		txmax = (box.x1 - ro.x()) * ddx;
	}
	if(ddy>=0)
	{
		tymin = (box.y1 - ro.y()) * ddy;
		tymax = (box.y2 - ro.y()) * ddy;
	}
	else
	{
		tymin = (box.y2 - ro.y()) * ddy;
		tymax = (box.y1 - ro.y()) * ddy;
	}
	if( (txmin>tymax) || (tymin>txmax) ) return 0;
	if( tymin>txmin ) txmin=tymin;
	if( tymax<txmax ) txmax=tymax;

	float tzmin, tzmax;
	float ddz = 1.0f/(ro.z()-rd.z());
	if(ddz>=0)
	{
		tzmin = (box.z1 - ro.z()) * ddz;
		tzmax = (box.z2 - ro.z()) * ddz;
	}
	else
	{
		tzmin = (box.z2 - ro.z()) * ddz;
		tzmax = (box.z1 - ro.z()) * ddz;
	}
	if( (txmin>tzmax) || (tzmin>txmax) ) return 0;
	return 1;
 }

//! Ray-Axis-oriented-cylinder intersection routine.
//! @param ray ray class.
//! @param box axis aligned bounding box structure.
//! @param tmin parent AABB min t.
//! @param tmax parent AABB max t.
//! @param ind cylinder axis index.
//! @return
//!    1 - if ray intersects AABB
//!    0 - otherwise
int IntersectCyl(const iARay & ray, const iAaabb& box, float &tmin, float&tmax, int ind);

//! Ray-AABB intersection routine, checks which subnodes' AABBs are intersected by ray
//! @param ray ray class.
//! @param tmin parent AABB min t.
//! @param tmax parent AABB max t.
//! @param split split plane's coordinates
//! @param splitIndex index of splitting axis
//! @param t [out] split plane's t
//! @return
//!    0 - left node intersected
//!    1 - both nodes intersected
//!    2 - right node intersected
inline int GetIntersectionState(const iARay &ray, float &tmin, float &tmax, float &split, int splitIndex, float &t)
{
	float rd = ray.GetDirection()[splitIndex];
	if(!rd)
		rd=0.00000001f;
	t = (split - ray.GetOrigin()[splitIndex]) / rd;
	const unsigned int sign = (rd >= 0.0f);
	if(t<tmin) return (int)(sign^0);
	if(t>tmax) return (int)(sign^1);
	return 2;
}

//! Triangle primitive class.
class iATriPrim final
{
public:
	inline unsigned int GetIndex() {return m_index;}
	iATriPrim( iAVec3f* a_V1, iAVec3f* a_V2, iAVec3f* a_V3, unsigned int index=0 );
	iATriPrim( iAtriangle *a_Tri, unsigned int index=0);
	iAVec3f& normal() { return m_Tri.N; }
	float &surface() {return m_Surface;}
	inline iAVec3f * getVertex(int i)
	{
		if (i >= 0 && i < 3)
		{
			return m_Tri.vertices[i];
		}
		return 0;
	}
	float &d() {return m_d;}
	int Intersect(iARay& a_Ray, float& a_Dist ) const;
	int Intersect(iAaabb &a_aabb, iAVec3f & a_BoxCentre, iAVec3f & a_BoxHalfsize) const;
	int CenterInside(iAaabb &a_aabb) const;
	inline float GetAngleCos(iARay& a_Ray){ return a_Ray.GetDirection()&m_Tri.N; }
	iAwald_tri GetWaldTri() {return m_WaldTri;}
	//! recalculate d coefficient when translation vector is given.
	void recalculateD(iAVec3f *translate);
	inline float getMinX() const
	{
		return min_macro( m_Tri.vertices[0]->x(), min_macro(m_Tri.vertices[1]->x(), m_Tri.vertices[2]->x()) );
	}
	inline float getMinY() const
	{
		return min_macro( m_Tri.vertices[0]->y(), min_macro(m_Tri.vertices[1]->y(), m_Tri.vertices[2]->y()) );
	}
	inline float getMinZ() const
	{
		return min_macro( m_Tri.vertices[0]->z(), min_macro(m_Tri.vertices[1]->z(), m_Tri.vertices[2]->z()) );
	}
	inline float getMaxX() const
	{
		return max_macro( m_Tri.vertices[0]->x(), max_macro(m_Tri.vertices[1]->x(), m_Tri.vertices[2]->x()) );
	}
	inline float getMaxY() const
	{
		return max_macro( m_Tri.vertices[0]->y(), max_macro(m_Tri.vertices[1]->y(), m_Tri.vertices[2]->y()) );
	}
	inline float getMaxZ() const
	{
		return max_macro( m_Tri.vertices[0]->z(), max_macro(m_Tri.vertices[1]->z(), m_Tri.vertices[2]->z()) );
	}
	inline const iAtriangle * getTri() const
	{
		return &m_Tri;
	}
	inline float getAxisBound(unsigned int axis_int, unsigned int maximum) const
	{
		switch(axis_int)
		{
		case 0://x
			return (maximum == 0) ? getMinX() : getMaxX();
			break;
		case 1://y
			return (maximum == 0) ? getMinY() : getMaxY();
		    break;
		case 2://z
			return (maximum == 0) ? getMinZ() : getMaxZ();
		    break;
		}
		return 0.f;
	}
private:
	//! Precompute some triangle's parameters as barycentric coords.
	void precompute();

	iAtriangle m_Tri;
	float m_Surface;      //!< surface area of triangle
	float m_d;            //!< distance from plane to origin
	iAwald_tri m_WaldTri;
	unsigned int m_index; //!< triangle's index in mesh

};


class iABSPTree;

//! Class representing scene data, organized as BSP tree. Also list of all primitives is available.
class iAScene
{
public:
	iAScene(){}
	~iAScene();
	//! Inits scene. BSP tree is created and build on current loaded mesh's data.
	int initScene(iAModelData & mdata, iADreamCasterSettings * s, QString const & filename = QString());
	//! Get number of primitives in scene.
	unsigned int getNrTriangles() { return (unsigned int)m_tris.size(); }
	//! Get primitive by its index.
	inline iATriPrim* getTriangle( int a_Idx ) { return m_tris[a_Idx]; }
	//! Get scene's BSP tree.
	iABSPTree* getBSPTree(void){return m_bsp;}
	//! recalculate d coefficient when translation vector is given for every triangle
	void recalculateD( iAVec3f *translate );
private:
	std::vector<iATriPrim*> m_tris; //!< list of all scene's primitives
	iABSPTree *m_bsp;               //!< scene's BSP-tree
};

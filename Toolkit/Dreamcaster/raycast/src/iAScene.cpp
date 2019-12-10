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
#include "../include/iABSPTree.h"
#include "../include/iADreamCasterCommon.h"
#include "../include/iARayTracer.h"
#include "../include/iAScene.h"
#include "../include/iASTLLoader.h"

//namespace Raytracer {

#define FINDMINMAX( x0, x1, x2, min, max ) \
	min = max = x0; if(x1<min) min=x1; if(x1>max) max=x1; if(x2<min) min=x2; if(x2>max) max=x2;
// X-tests
#define AXISTEST_X01( a, b, fa, fb )                                            \
	p0 = a * v0.y() - b * v0.z(), p2 = a * v2.y() - b * v2.z();                 \
	if (p0 < p2) { min = p0; max = p2;} else { min = p2; max = p0; }            \
	rad = fa * a_BoxHalfsize.y() + fb * a_BoxHalfsize.z();                      \
	if (min > rad || max < -rad) return 0;
#define AXISTEST_X2( a, b, fa, fb )                                             \
	p0 = a * v0.y() - b * v0.z(), p1 = a * v1.y() - b * v1.z();                 \
	if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0;}            \
	rad = fa * a_BoxHalfsize.y() + fb * a_BoxHalfsize.z();                      \
	if(min>rad || max<-rad) return 0;
// Y-tests
#define AXISTEST_Y02( a, b, fa, fb )                                            \
	p0 = -a * v0.x() + b * v0.z(), p2 = -a * v2.x() + b * v2.z();               \
	if(p0 < p2) { min = p0; max = p2; } else { min = p2; max = p0; }            \
	rad = fa * a_BoxHalfsize.x() + fb * a_BoxHalfsize.z();                      \
	if (min > rad || max < -rad) return 0;
#define AXISTEST_Y1( a, b, fa, fb )                                             \
	p0 = -a * v0.x() + b * v0.z(), p1 = -a * v1.x() + b * v1.z();               \
	if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; }           \
	rad = fa * a_BoxHalfsize.x() + fb * a_BoxHalfsize.z();                      \
	if (min > rad || max < -rad) return 0;
// Z-tests
#define AXISTEST_Z12( a, b, fa, fb )                                            \
	p1 = a * v1.x() - b * v1.y(), p2 = a * v2.x() - b * v2.y();                 \
	if(p2 < p1) { min = p2; max = p1; } else { min = p1; max = p2; }            \
	rad = fa * a_BoxHalfsize.x() + fb * a_BoxHalfsize.y();                      \
	if (min > rad || max < -rad) return 0;
#define AXISTEST_Z0( a, b, fa, fb )                                             \
	p0 = a * v0.x() - b * v0.y(), p1 = a * v1.x() - b * v1.y();                 \
	if(p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; }            \
	rad = fa * a_BoxHalfsize.x() + fb * a_BoxHalfsize.y();                      \
	if (min > rad || max < -rad) return 0;

//------------------------------------------------------------
//iATriPrim class implementation
//------------------------------------------------------------
iATriPrim::iATriPrim( iAtriangle *a_Tri, unsigned int index) :m_Tri(a_Tri->vertices[0],a_Tri->vertices[1],a_Tri->vertices[2])
{
	m_index = index;
	precompute();
}
iATriPrim::iATriPrim( iAVec3f* a_V1, iAVec3f* a_V2, iAVec3f* a_V3, unsigned int index ) :m_Tri(a_V1, a_V2, a_V3)
{
	m_index = index;
	precompute();
}

unsigned int modulo[] = { 0, 1, 2, 0, 1 };
int iATriPrim::Intersect( iARay& a_Ray, float& a_Dist ) const
{
	#define ku modulo[m_WaldTri.k + 1]
	#define kv modulo[m_WaldTri.k + 2]
	iAVec3f O = a_Ray.GetOrigin(), D = a_Ray.GetDirection();
	const float lnd = 1.0f / (D[m_WaldTri.k] + m_WaldTri.nu * D[ku] + m_WaldTri.nv * D[kv]);
	const float t = (m_WaldTri.nd - O[m_WaldTri.k] - m_WaldTri.nu * O[ku] - m_WaldTri.nv * O[kv]) * lnd;
	if (!(a_Dist > t && t > 0)) return MISS;
	float hu = O[ku] + t * D[ku] - m_WaldTri.m_A[ku];
	float hv = O[kv] + t * D[kv] - m_WaldTri.m_A[kv];
	float beta = hv * m_WaldTri.bnu + hu * m_WaldTri.bnv;//=m_WaldTri.m_U=//èëè íàîáîðîò
	if (beta < 0) return MISS;
	float gamma = hu * m_WaldTri.cnu + hv * m_WaldTri.cnv;//=m_WaldTri.m_V=
	if (gamma < 0) return MISS;
	if ((beta + gamma) > 1) return MISS;
	a_Dist = t;
	return ((D&m_WaldTri.m_N ) > 0)? INPRIM : HIT;
}

bool PlaneBoxOverlap( iAVec3f & a_Normal, iAVec3f & a_Vert, iAVec3f & a_MaxBox )
{
	iAVec3f vmin, vmax;
	for( int q = 0; q < 3; q++ )
	{
		float v = a_Vert[q];
		if (a_Normal[q] > 0.0f)
		{
			vmin[q] = -a_MaxBox[q] - v;
			vmax[q] =  a_MaxBox[q] - v;
		}
		else
		{
			vmin[q] =  a_MaxBox[q] - v;
			vmax[q] = -a_MaxBox[q] - v;
		}
	}
	if (( a_Normal&vmin) > 0.0f) return false;
	if (( a_Normal&vmax) >= 0.0f) return true;
	return false;
}
int iATriPrim::Intersect(iAaabb &a_aabb, iAVec3f & a_BoxCentre, iAVec3f & a_BoxHalfsize) const
{
	iAVec3f * a_V0 = m_Tri.vertices[0];
	iAVec3f * a_V1 = m_Tri.vertices[1];
	iAVec3f * a_V2 = m_Tri.vertices[2];
	
	iAVec3f v0, v1, v2, normal, e0, e1, e2;
	float min, max, p0, p1, p2, rad, fex, fey, fez;
	v0 = *a_V0 - a_BoxCentre;
	v1 = *a_V1 - a_BoxCentre;
	v2 = *a_V2 - a_BoxCentre;
	e0 = v1 - v0, e1 = v2 - v1, e2 = v0 - v2;
	fex = fabsf( e0.x() );
	fey = fabsf( e0.y() );
	fez = fabsf( e0.z() );
	AXISTEST_X01( e0.z(), e0.y(), fez, fey );
	AXISTEST_Y02( e0.z(), e0.x(), fez, fex );
	AXISTEST_Z12( e0.y(), e0.x(), fey, fex );
	fex = fabsf( e1.x() );
	fey = fabsf( e1.y() );
	fez = fabsf( e1.z() );
	AXISTEST_X01( e1.z(), e1.y(), fez, fey );
	AXISTEST_Y02( e1.z(), e1.x(), fez, fex );
	AXISTEST_Z0 ( e1.y(), e1.x(), fey, fex );
	fex = fabsf( e2.x() );
	fey = fabsf( e2.y() );
	fez = fabsf( e2.z() );
	AXISTEST_X2 ( e2.z(), e2.y(), fez, fey );
	AXISTEST_Y1 ( e2.z(), e2.x(), fez, fex );
	AXISTEST_Z12( e2.y(), e2.x(), fey, fex );
	FINDMINMAX( v0.x(), v1.x(), v2.x(), min, max );
	if (min > a_BoxHalfsize.x() || max < -a_BoxHalfsize.x()) return false;
	FINDMINMAX( v0.y(), v1.y(), v2.y(), min, max );
	if (min > a_BoxHalfsize.y() || max < -a_BoxHalfsize.y()) return false;
	FINDMINMAX( v0.z(), v1.z(), v2.z(), min, max );
	if (min > a_BoxHalfsize.z() || max < -a_BoxHalfsize.z()) return false;
	normal = ( e0^e1 );
	if (!PlaneBoxOverlap(normal, v0, a_BoxHalfsize )) return false;
	return true;
}

void iATriPrim::precompute()
{
	// init precomp
	//normal
	iAVec3f A = *m_Tri.vertices[0];
	iAVec3f B = *m_Tri.vertices[1];
	iAVec3f C = *m_Tri.vertices[2];
	iAVec3f c = B - A;
	iAVec3f b = C - A;
	m_Tri.N = b^c;
	int u, v;
	if (fabsf( m_Tri.N.x() ) > fabsf( m_Tri.N.y()))
	{
		if (fabsf( m_Tri.N.x() ) > fabsf( m_Tri.N.z() )) m_WaldTri.k = 0; else m_WaldTri.k = 2;
	}
	else
	{
		if (fabsf( m_Tri.N.y() ) > fabsf( m_Tri.N.z() )) m_WaldTri.k = 1; else m_WaldTri.k = 2;
	}
	u = (m_WaldTri.k + 1) % 3;
	v = (m_WaldTri.k + 2) % 3;
	// precomp
	float krec = 1.0f / m_Tri.N[m_WaldTri.k];
	m_WaldTri.nu = m_Tri.N[u] * krec;
	m_WaldTri.nv = m_Tri.N[v] * krec;
	m_WaldTri.nd = (m_Tri.N&A) * krec;
	// first line equation
	float reci = 1.0f / (b[u] * c[v] - b[v] * c[u]);
	m_WaldTri.bnu = b[u] * reci;
	m_WaldTri.bnv = -b[v] * reci;
	// second line equation
	m_WaldTri.cnu = c[v] * reci;
	m_WaldTri.cnv = -c[u] * reci;
	// finalize normal
	m_Surface = 0.5f * m_Tri.N.length();
	m_Tri.N.normalize();
	m_d = (m_Tri.N)&(*m_Tri.vertices[0]);
	m_WaldTri.m_A = *m_Tri.vertices[0];
	m_WaldTri.m_N = m_Tri.N;
}

int iATriPrim::CenterInside(iAaabb &a_aabb ) const
{
	float center[3] =	{0.5f*(getAxisBound(0,0) + getAxisBound(0,1)),
						 0.5f*(getAxisBound(1,0) + getAxisBound(1,1)),
						 0.5f*(getAxisBound(2,0) + getAxisBound(2,1))};
	return (int)(center[0]>a_aabb.x1 && center[0]<a_aabb.x2 && center[1]>a_aabb.y1 && center[1]<a_aabb.y2 && center[2]>a_aabb.z1 && center[2]<a_aabb.z2);
}

void iATriPrim::recalculateD( iAVec3f *translate )
{
	m_d = (m_Tri.N) & ((*m_Tri.vertices[0]) - (*translate));
}

// -----------------------------------------------------------
// iAScene class implementation
// -----------------------------------------------------------

iAScene::~iAScene()
{
	for (unsigned int i=0; i<m_tris.size(); i++)
	{
		if(m_tris[i])
			delete m_tris[i];
	}
	if(m_bsp)
		delete m_bsp;
}

int iAScene::initScene(iAModelData & mdata, iADreamCasterSettings * s, QString const & filename)
{
	unsigned int i=0;
	iATriPrim* pr;
	for (i=0; i<mdata.stlMesh.size(); i++)
	{
		pr = new iATriPrim(mdata.stlMesh[i], i);
		m_tris.push_back(pr);
	}
	m_bsp = new iABSPTree;
	if (filename.isEmpty())
	{
		if(m_tris.size()>(unsigned int)s->TREE_SPLIT2)
			m_bsp->BuildTree(s->TREE_L3, mdata.box);
		else if(m_tris.size()>(unsigned int)s->TREE_SPLIT1)
			m_bsp->BuildTree(s->TREE_L2, mdata.box);
		else
			m_bsp->BuildTree(s->TREE_L1, mdata.box);
		//
		m_bsp->FillTree(m_tris);	
	}
	else
	{
		dcast->log("Loading existing KD-tree...............");
		if(!m_bsp->LoadTree(filename))
		{
			dcast->log("Creating a new tree.");
			
			//building new tree
			if(m_tris.size()>(unsigned int)s->TREE_SPLIT2)
				m_bsp->BuildTree(s->TREE_L3, mdata.box);
			else if(m_tris.size()>(unsigned int)s->TREE_SPLIT1)
				m_bsp->BuildTree(s->TREE_L2, mdata.box);
			else
				m_bsp->BuildTree(s->TREE_L1, mdata.box);
			//
			m_bsp->FillTree(m_tris);
			//save the tree
			m_bsp->SaveTree(filename);
			return 1;
		}
		m_bsp->FillLoadedTree(m_tris);
	}
	return 1;
}

void iAScene::recalculateD( iAVec3f *translate )
{
	for (unsigned int i = 0; i < m_tris.size(); i++)
	{
		m_tris[i]->recalculateD(translate);
	}
}

int IntersectCyl(const iARay & ray, const iAaabb& box, float &tmin, float&tmax, int ind)
{
	iAVec3f ro = ray.GetOrigin();
	iAVec3f rd = ray.GetDirection();
	iAVec3f bcenter = box.center();
	iAVec3f cylDir = iAVec3f(0,0,1);
	//cylDir[ind] = 1;
	float cylRad = max_macro(max_macro(box.half_size()[0], box.half_size()[1]), box.half_size()[2]);
	float l2ldist = distLineToLine(ro, rd, bcenter, cylDir);
	if(l2ldist>cylRad)
		return 0;
	float a = rd.x()*rd.x() + rd.y()*rd.y();
	float b = 2*(ro.x()*rd.x() + ro.y()*rd.y());
	float c = ro.x()*ro.x() + ro.y()*ro.y() - cylRad*cylRad;
	float sqD = sqrt(b*b-4*a*c);
	float t[2] = {(-b - sqD)/(2*a), (-b + sqD)/(2*a)};
	float z[2] = {ro.z()+t[0]*rd.z(), ro.z()+t[1]*rd.z()};

	//sides of cylinder
	if((z[0]>box.z1 && z[0]<box.z2) || (z[1]>box.z1 && z[1]<box.z2))
		return 1;
	//first cap
	iAVec3f A = bcenter - cylDir*box.half_size()[2];
	float h = (ro-A)&cylDir; 
	float dist = (A - ro + rd*h/(rd&cylDir)).length();
	if(dist<=cylRad)
		return 1;
	//second cap
	A = bcenter + cylDir*box.half_size()[2];
	h = (ro-A)&cylDir; 
	dist = (A - ro + rd*h/(rd&cylDir)).length(); 
	if(dist<=cylRad)
		return 1;
	return 0;
}
//}; // namespace Raytracer

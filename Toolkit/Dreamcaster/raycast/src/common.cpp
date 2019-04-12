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
#include "../include/common.h"

#include <QSettings>

#include <cassert>
#include <cstdlib>

inline float Rand( float a_Range ) { return ((float)rand() / RAND_MAX) * a_Range; }

int ParseConfigFile(iADreamCasterSettings * s)
{
	QSettings settings;
	s->THREAD_GRID_X   = settings.value("DreamCaster/THREAD_GRID_X", s->THREAD_GRID_X).toInt();
	s->THREAD_GRID_Y   = settings.value("DreamCaster/THREAD_GRID_Y", s->THREAD_GRID_Y ).toInt();
	s->SCALE_COEF      = settings.value("DreamCaster/SCALE_COEF", s->SCALE_COEF ).value<float>();
	s->COLORING_COEF   = settings.value("DreamCaster/COLORING_COEF", s->COLORING_COEF ).value<float>();
	s->RFRAME_W        = settings.value("DreamCaster/RFRAME_W", s->RFRAME_W).toInt();
	s->RFRAME_H        = settings.value("DreamCaster/RFRAME_H", s->RFRAME_H).toInt();
	s->VFRAME_W        = settings.value("DreamCaster/VFRAME_W", s->VFRAME_W).toInt();
	s->VFRAME_H        = settings.value("DreamCaster/VFRAME_H", s->VFRAME_H).toInt();
	s->ORIGIN_Z        = settings.value("DreamCaster/ORIGIN_Z", s->ORIGIN_Z).value<float>();
	s->PLANE_Z         = settings.value("DreamCaster/PLANE_Z", s->PLANE_Z).value<float>();
	s->PLANE_H_W       = settings.value("DreamCaster/PLANE_H_W", s->PLANE_H_W).value<float>();
	s->PLANE_H_H       = settings.value("DreamCaster/PLANE_H_H", s->PLANE_H_H).value<float>();
	s->TREE_L1         = settings.value("DreamCaster/TREE_L1", s->TREE_L1).toInt();
	s->TREE_L2         = settings.value("DreamCaster/TREE_L2", s->TREE_L2).toInt();
	s->TREE_L3         = settings.value("DreamCaster/TREE_L3", s->TREE_L3).toInt();
	s->TREE_SPLIT1     = settings.value("DreamCaster/TREE_SPLIT1", s->TREE_SPLIT1).toInt();
	s->TREE_SPLIT2     = settings.value("DreamCaster/TREE_SPLIT2", s->TREE_SPLIT2).toInt();
	s->BG_COL_R        = settings.value("DreamCaster/BG_COL_R", s->BG_COL_R).toInt();
	s->BG_COL_G        = settings.value("DreamCaster/BG_COL_G", s->BG_COL_G).toInt();
	s->BG_COL_B        = settings.value("DreamCaster/BG_COL_B", s->BG_COL_B).toInt();
	s->PLATE_COL_R     = settings.value("DreamCaster/PLATE_COL_R", s->PLATE_COL_R).toInt();
	s->PLATE_COL_G     = settings.value("DreamCaster/PLATE_COL_G", s->PLATE_COL_G).toInt();
	s->PLATE_COL_B     = settings.value("DreamCaster/PLATE_COL_B", s->PLATE_COL_B).toInt();
	s->COL_RANGE_MIN_R = settings.value("DreamCaster/COL_RANGE_MIN_R", s->COL_RANGE_MIN_R).toInt();
	s->COL_RANGE_MIN_G = settings.value("DreamCaster/COL_RANGE_MIN_G", s->COL_RANGE_MIN_G).toInt();
	s->COL_RANGE_MIN_B = settings.value("DreamCaster/COL_RANGE_MIN_B", s->COL_RANGE_MIN_B).toInt();
	s->COL_RANGE_MAX_R = settings.value("DreamCaster/COL_RANGE_MAX_R", s->COL_RANGE_MAX_R).toInt();
	s->COL_RANGE_MAX_G = settings.value("DreamCaster/COL_RANGE_MAX_G", s->COL_RANGE_MAX_G).toInt();
	s->COL_RANGE_MAX_B = settings.value("DreamCaster/COL_RANGE_MAX_B", s->COL_RANGE_MAX_B).toInt();
	s->USE_SAH         = settings.value("DreamCaster/USE_SAH", s->USE_SAH).toInt();
	s->BATCH_SIZE      = settings.value("DreamCaster/BATCH_SIZE", s->BATCH_SIZE).toInt();
	s->COLORING_COEF *= s->SCALE_COEF;
	s->ORIGIN_Z  /= s->SCALE_COEF;
	s->PLANE_Z   /= s->SCALE_COEF;
	s->PLANE_H_W /= s->SCALE_COEF;
	s->PLANE_H_H /= s->SCALE_COEF;
		
	s->COL_RANGE_DR = s->COL_RANGE_MAX_R - s->COL_RANGE_MIN_R;
	s->COL_RANGE_DG = s->COL_RANGE_MAX_G - s->COL_RANGE_MIN_G;
	s->COL_RANGE_DB = s->COL_RANGE_MAX_B - s->COL_RANGE_MIN_B;

	return 1;
}

iAMat4 ScaleAndCentreBBox(iAaabb &box, float *scale_coef_out, float* translate3f_out)
{
	float scale_coeff = 1.0f;
	if(scale_coef_out!=0)
		(*scale_coef_out) = scale_coeff;

	if(translate3f_out!=0)
		for (unsigned int i = 0; i<3; i++)
		{
			translate3f_out[i] = -box.center()[i];
		}
			
	return scale(iAVec3f(scale_coeff,scale_coeff,scale_coeff))*translate(-box.center());
}

iAVec3f projectPtOnLine(iAVec3f & o, iAVec3f & dir, iAVec3f & pt)
{
	iAVec3f o2pt = pt - o;
	float o2ptLen = o2pt.length();
	o2pt.normalize();
	dir.normalize();
	float cos_dir_o2pt = o2pt & dir;
	iAVec3f ptproj = o + dir*o2ptLen*cos_dir_o2pt;
	return ptproj;
}

float distPointToLine( iAVec3f & o, iAVec3f & dir, iAVec3f & pt )
{
	iAVec3f o2pt = pt - o;
	float o2ptLen = o2pt.length();
	o2pt.normalize();
	dir.normalize();
	float cos_dir_o2pt = o2pt & dir;
	iAVec3f ptproj = o + dir*o2ptLen*cos_dir_o2pt;
	return (pt-ptproj).length();
}

float distLineToLine( iAVec3f & o1, iAVec3f & d1, iAVec3f & o2, iAVec3f & d2 )
{
	iAVec3f perpendicular = (d1^d2);
	perpendicular.normalize();
	return fabs((o1-o2)&perpendicular);
}

//iAaabb impl
iAaabb::iAaabb()
{
	x1=x2=y1=y2=z1=z2=0.f;
}

iAaabb::iAaabb(iAaabb& el)
{
	x1=el.x1;
	x2=el.x2;
	y1=el.y1;
	y2=el.y2;
	z1=el.z1;
	z2=el.z2;
}

iAaabb::iAaabb(float a_x1, float a_x2, float a_y1, float a_y2, float a_z1, float a_z2)
{
	x1=a_x1;
	x2=a_x2;
	y1=a_y1;
	y2=a_y2;
	z1=a_z1;
	z2=a_z2;
}

void iAaabb::setData(float a_x1, float a_x2, float a_y1, float a_y2, float a_z1, float a_z2)
{
	x1=a_x1;
	x2=a_x2;
	y1=a_y1;
	y2=a_y2;
	z1=a_z1;
	z2=a_z2;
}

void iAaabb::setData(const iAaabb & el)
{
	x1=el.x1;
	x2=el.x2;
	y1=el.y1;
	y2=el.y2;
	z1=el.z1;
	z2=el.z2;
}

iAVec3f iAaabb::center() const
{
	return iAVec3f((x2+x1)*0.5f, (y2+y1)*0.5f, (z2+z1)*0.5f );
}

iAVec3f iAaabb::half_size() const
{
	#ifdef _DEBUG
		assert( x2>=x1 && y2>=y1 && z2>=z1);
	#endif
	return iAVec3f( (x2-x1)*0.5f, (y2-y1)*0.5f, (z2-z1)*0.5f );
}

int iAaabb::mainDim() const
{
	if(fabs(x2-x1)>=fabs(y2-y1))
	{
		if(fabs(x2-x1)>=fabs(z2-z1))
			return 0;//X
		else
			return 2;//Z
	}
	else
	{
		if(fabs(y2-y1)>=fabs(z2-z1))
			return 1;//Y
		else
			return 2;//Z
	}
}

float iAaabb::surfaceArea()
{
	float dims[3] = {x2-x1, y2-y1, z2-z1};
	return 2.0f * ( dims[0]*dims[1] + dims[0]*dims[2] + dims[1]*dims[2] );
}

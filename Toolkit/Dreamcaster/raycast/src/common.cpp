#include "../include/common.h"

#include <QSettings>
//#include "../../ConfigFile/ConfigFile.h"
#include <assert.h>
#include <stdlib.h>

inline float Rand( float a_Range ) { return ((float)rand() / RAND_MAX) * a_Range; }

//namespace Raytracer {
	int ParseConfigFile(char const * cfg_filename, SETTINGS * s)
	{
		QSettings settings("FHW", "DreamCaster");
		s->THREAD_GRID_X = settings.value( "THREAD_GRID_X", s->THREAD_GRID_X).toInt();
		s->THREAD_GRID_Y = settings.value( "THREAD_GRID_Y", s->THREAD_GRID_Y ).toInt();
		s->SCALE_COEF = settings.value( "SCALE_COEF", s->SCALE_COEF ).value<float>();
		s->COLORING_COEF = settings.value( "COLORING_COEF", s->COLORING_COEF ).value<float>();
		s->COLORING_COEF *= s->SCALE_COEF;
		s->RFRAME_W = settings.value( "RFRAME_W", s->RFRAME_W ).toInt();
		s->RFRAME_H = settings.value( "RFRAME_H", s->RFRAME_H ).toInt();
		s->VFRAME_W = settings.value( "VFRAME_W", s->VFRAME_W ).toInt();
		s->VFRAME_H = settings.value( "VFRAME_H", s->VFRAME_H ).toInt();
		s->ORIGIN_Z = settings.value( "ORIGIN_Z", s->ORIGIN_Z ).value<float>();
		s->ORIGIN_Z  /= s->SCALE_COEF;
		s->PLANE_Z = settings.value( "PLANE_Z", s->PLANE_Z ).value<float>();
		s->PLANE_Z   /= s->SCALE_COEF;
		s->PLANE_H_W = settings.value( "PLANE_H_W", s->PLANE_H_W ).value<float>();
		s->PLANE_H_W /= s->SCALE_COEF;
		s->PLANE_H_H = settings.value( "PLANE_H_H", s->PLANE_H_H ).value<float>();
		s->PLANE_H_H /= s->SCALE_COEF;
		s->TREE_L1 = settings.value( "TREE_L1", s->TREE_L1 ).toInt();
		s->TREE_L2 = settings.value( "TREE_L2", s->TREE_L2 ).toInt();
		s->TREE_L3 = settings.value( "TREE_L3", s->TREE_L3 ).toInt();
		s->TREE_SPLIT1 = settings.value( "TREE_SPLIT1", s->TREE_SPLIT1 ).toInt();
		s->TREE_SPLIT2 = settings.value( "TREE_SPLIT2", s->TREE_SPLIT2 ).toInt();
		s->BG_COL_R = settings.value( "BG_COL_R", s->BG_COL_R ).toInt();
		s->BG_COL_G = settings.value( "BG_COL_G", s->BG_COL_G ).toInt();
		s->BG_COL_B = settings.value( "BG_COL_B", s->BG_COL_B ).toInt();
		s->PLATE_COL_R = settings.value( "PLATE_COL_R", s->PLATE_COL_R ).toInt();
		s->PLATE_COL_G = settings.value( "PLATE_COL_G", s->PLATE_COL_G ).toInt();
		s->PLATE_COL_B = settings.value( "PLATE_COL_B", s->PLATE_COL_B ).toInt();
		s->COL_RANGE_MIN_R = settings.value( "COL_RANGE_MIN_R", s->COL_RANGE_MIN_R ).toInt();
		s->COL_RANGE_MIN_G = settings.value( "COL_RANGE_MIN_G", s->COL_RANGE_MIN_G ).toInt();
		s->COL_RANGE_MIN_B = settings.value( "COL_RANGE_MIN_B", s->COL_RANGE_MIN_B ).toInt();
		s->COL_RANGE_MAX_R = settings.value( "COL_RANGE_MAX_R", s->COL_RANGE_MAX_R ).toInt();
		s->COL_RANGE_MAX_G = settings.value( "COL_RANGE_MAX_G", s->COL_RANGE_MAX_G ).toInt();
		s->COL_RANGE_MAX_B = settings.value( "COL_RANGE_MAX_B", s->COL_RANGE_MAX_B ).toInt();
		s->USE_SAH = settings.value( "USE_SAH", s->USE_SAH ).toInt();
		s->BATCH_SIZE = settings.value( "BATCH_SIZE", s->BATCH_SIZE ).toInt();
		
		s->COL_RANGE_DR = s->COL_RANGE_MAX_R - s->COL_RANGE_MIN_R;
		s->COL_RANGE_DG = s->COL_RANGE_MAX_G - s->COL_RANGE_MIN_G;
		s->COL_RANGE_DB = s->COL_RANGE_MAX_B - s->COL_RANGE_MIN_B;

		return 1;
	}


	void normalize(iAVec3& u)
	{
		float len = u.length();
		if(len>0)
			u = u / u.length();
	}

	iAMat4 ScaleAndCentreBBox(aabb &box, float *scale_coef_out, float* translate3f_out)
	{
		float scale_coeff = 1.0f;
		if(scale_coef_out!=0)
			(*scale_coef_out) = scale_coeff;

		if(translate3f_out!=0)
			for (unsigned int i = 0; i<3; i++)
			{
				translate3f_out[i] = -box.center()[i];
			}
			
		return scale(iAVec3(scale_coeff,scale_coeff,scale_coeff))*translate(-box.center());
	}

	iAVec3 projectPtOnLine(iAVec3 &o, iAVec3 &dir, iAVec3& pt)
	{
		iAVec3 o2pt = pt - o;
		float o2ptLen = o2pt.length();
		normalize(o2pt);
		normalize(dir);
		float cos_dir_o2pt = o2pt & dir;
		iAVec3 ptproj = o + dir*o2ptLen*cos_dir_o2pt;
		return ptproj;
	}

	float distPointToLine( iAVec3 &o, iAVec3 &dir, iAVec3& pt )
	{
		iAVec3 o2pt = pt - o;
		float o2ptLen = o2pt.length();
		normalize(o2pt);
		normalize(dir);
		float cos_dir_o2pt = o2pt & dir;
		iAVec3 ptproj = o + dir*o2ptLen*cos_dir_o2pt;
		return (pt-ptproj).length();
	}

	float distLineToLine( iAVec3 & o1, iAVec3 & d1, iAVec3 & o2, iAVec3 & d2 )
	{
		iAVec3 perpendicular = (d1^d2);
		normalize(perpendicular);
		return fabs((o1-o2)&perpendicular);
		//return (projectPtOnLine(o1, d1, o2) - o2).length();
		//return (projectPtOnLine(o2, d2, o1) - o1).length();
		/*iAVec3 norm = d1 ^ d2;
		return (projectPtOnLine(o1, d1, o2) - projectPtOnLine(o2, d2, o1)).length();*/
	}

	//aabb impl
	aabb::aabb()
	{
		x1=x2=y1=y2=z1=z2=0.f;
	}

	aabb::aabb(aabb& el)
	{
		x1=el.x1;
		x2=el.x2;
		y1=el.y1;
		y2=el.y2;
		z1=el.z1;
		z2=el.z2;
	}

	aabb::aabb(float a_x1, float a_x2, float a_y1, float a_y2, float a_z1, float a_z2)
	{
		x1=a_x1;
		x2=a_x2;
		y1=a_y1;
		y2=a_y2;
		z1=a_z1;
		z2=a_z2;
	}

	void aabb::setData(float a_x1, float a_x2, float a_y1, float a_y2, float a_z1, float a_z2)
	{
		x1=a_x1;
		x2=a_x2;
		y1=a_y1;
		y2=a_y2;
		z1=a_z1;
		z2=a_z2;
	}

	void aabb::setData(const aabb & el)
	{
		x1=el.x1;
		x2=el.x2;
		y1=el.y1;
		y2=el.y2;
		z1=el.z1;
		z2=el.z2;
	}

	iAVec3 aabb::center() const
	{
		return iAVec3((x2+x1)*0.5f, (y2+y1)*0.5f, (z2+z1)*0.5f );
	}

	iAVec3 aabb::half_size() const
	{
		#ifdef _DEBUG
			assert( x2>=x1 && y2>=y1 && z2>=z1);
		#endif
		return iAVec3( (x2-x1)*0.5f, (y2-y1)*0.5f, (z2-z1)*0.5f );
	}

	int aabb::mainDim()
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

	float aabb::surfaceArea()
	{
		float dims[3] = {x2-x1, y2-y1, z2-z1};
		return 2.0f * ( dims[0]*dims[1] + dims[0]*dims[2] + dims[1]*dims[2] );
	}
//}; // namespace Raytracer

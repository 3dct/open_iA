/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include <vector>
#include <QTextStream>


/**	\class Intersection.
	\brief Class representing intersection between triangle and ray.	
*/
class Intersection
{
public:
	unsigned int tri_index; ///< index of intersected triangle in object
	float dip_angle;		///< cos of dip angle btw intersected triangle and ray
	//
	Intersection():tri_index(0), dip_angle(0.0f)
	{
		m_write2FileSize = sizeof(tri_index)+sizeof(dip_angle);
	}
	Intersection(int a_tri_index, float a_dip_angle):tri_index(a_tri_index), dip_angle(a_dip_angle)
	{
		m_write2FileSize = sizeof(tri_index)+sizeof(dip_angle);
	}
	inline void setData(int a_tri_index, float a_dip_angle)
	{
		tri_index = a_tri_index;
		dip_angle = a_dip_angle;
	}
	/**
	* Writes intersection data into binary file by file descriptor.
	* @param fptr output file descriptor.
	*/
	inline void write2BinaryFile(FILE *fptr)
	{
		/*fwrite(&tri_index, sizeof(tri_index), 1, fptr);
		fwrite(&dip_angle, sizeof(dip_angle), 1, fptr);*/
		fwrite(this, m_write2FileSize, 1, fptr);
	}
	unsigned int getWrite2FileSize(){return m_write2FileSize;}
protected:
	unsigned int m_write2FileSize;
};

/**	\class RayPenetration.
	\brief Class representing total penetration of single ray into object.

	Detailed description.	
*/
class RayPenetration
{
public:
	int m_X;				///< ray X coordinate on plate
	int m_Y;				///< ray Y coordinate on plate
	float totalPenetrLen;	///< total length of all ray penetrations
	float avDipAng;			///< average dip angle cos of ray
	///float penetrations[MAX_PENETRATIONS_COUNT];
	unsigned int penetrationsSize;///< number of all ray penetrations 
	//
	RayPenetration(){
		penetrationsSize=0;
		m_write2FileSize = sizeof(m_X)+sizeof(m_Y)+sizeof(totalPenetrLen)+sizeof(avDipAng)+sizeof(penetrationsSize);
	}
	RayPenetration(int a_X, int a_Y, float a_totalPenetrLen, float a_avDip): 
		m_X(a_X),
		m_Y(a_Y),
		totalPenetrLen(a_totalPenetrLen),
		avDipAng(a_avDip)
	{}
	/**
	* Writes ray penetrations data into binary file by file descriptor.
	* @param fptr output file descriptor.
	*/
	inline void write2BinaryFile(FILE *fptr)
	{
		fwrite(this, m_write2FileSize, 1, fptr);
	}
	unsigned int getWrite2FileSize(){return m_write2FileSize;}
protected:
	unsigned int m_write2FileSize;
};

/**	\class RenderFromPosition.
	\brief Class representing single object render.

	Contains all data of single object render with some set of parameters.
	Parameters are: rotations about X and Y axes, object's position, average penetration length of render,
	average dip angle cos of render.
	Also contains all statistical data as: all intersections data, all rays' penetrations data.
*/
class RenderFromPosition
{
public:
	RenderFromPosition()
	{
		m_headerSize = sizeof(rotX)+sizeof(rotY)+sizeof(rotZ)+sizeof(pos)+sizeof(avPenetrLen)+sizeof(avDipAngle)+sizeof(maxPenetrLen)+sizeof(badAreaPercentage)+sizeof(raysSize);
		rawPtrRaysVec.clear();
	};
	~RenderFromPosition()
	{
		clear();
	}
	//order of declarations is important when writhing to file
	float rotX;		///< rotation about X axis
	float rotY;		///< rotation about Y axis
	float rotZ;		///< rotation about Z axis
	float pos[3];		///< object's position
	float avPenetrLen;///< average penetration length
	float avDipAngle;	///< average dip angle cos
	float maxPenetrLen;///< maximum penetration length in rendering
	float badAreaPercentage;///< percentage of bad surface area corresponding to radon space analysis
	unsigned int raysSize;///< number of rays
	std::vector<RayPenetration*> rays; ///<rays' penetrations data
	std::vector<RayPenetration*> rawPtrRaysVec;
	std::vector<Intersection*> intersections;///< intersections data
	unsigned int intersectionsSize;///< number of intersections
	/**
	* Clears all statistical data (penetrations and intersectoins data).
	*/
	void clear()
	{
		for (unsigned int i=0; i<rawPtrRaysVec.size(); i++)
		{
			delete [] rawPtrRaysVec[i];
		}
		rawPtrRaysVec.clear();
		rays.clear();
		for (unsigned int i=0; i<intersections.size(); i++)
		{
			delete intersections[i];
		}
		intersections.clear();
		intersectionsSize=0;
		raysSize=0;
	}
	/**
	* Writes all rendering data into binary file by file descriptor.
	* @param fptr output file descriptor.
	*/
	inline void write2BinaryFile(FILE *fptr, bool saveAdditionalData = true)
	{
		if(!saveAdditionalData)
		{
			size_t bufferSize = m_headerSize+sizeof(intersectionsSize);
			unsigned int *buffer = new unsigned int[bufferSize];
			memcpy (buffer, this, m_headerSize);
			buffer[bufferSize/sizeof(buffer[0])-2]=0;
			buffer[bufferSize/sizeof(buffer[0])-1]=0;
			fwrite(buffer, bufferSize, 1, fptr);
			delete [] buffer;
			return;
		}
		else
		{
			fwrite(this, m_headerSize, 1, fptr);
			for (unsigned int i=0; i<raysSize; i++)
				rays[i]->write2BinaryFile(fptr);
			fwrite(&intersectionsSize, sizeof(intersectionsSize), 1, fptr);
			for (unsigned int i=0; i<intersectionsSize; i++)
				intersections[i]->write2BinaryFile(fptr);
		}
	}
	static const int getSkipedSizeInFile()
	{
		return sizeof(float)*10;//skip rotx, roty, rotz, avpenlen, avdipangle, maxpenlen, badAreaPercentage, position//20+4*3
	}
protected:
	unsigned int m_headerSize;//< used when writing to binary file
};

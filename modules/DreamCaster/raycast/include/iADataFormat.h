// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QTextStream>

#include <vector>

//! Class representing an intersection between triangle and ray.
class iAIntersection
{
public:
	unsigned int tri_index; //!< index of intersected triangle in object
	float dip_angle;        //!< cos of dip angle btw intersected triangle and ray

	iAIntersection():tri_index(0), dip_angle(0.0f)
	{
		m_write2FileSize = sizeof(tri_index)+sizeof(dip_angle);
	}
	iAIntersection(int a_tri_index, float a_dip_angle):tri_index(a_tri_index), dip_angle(a_dip_angle)
	{
		m_write2FileSize = sizeof(tri_index)+sizeof(dip_angle);
	}
	inline void setData(int a_tri_index, float a_dip_angle)
	{
		tri_index = a_tri_index;
		dip_angle = a_dip_angle;
	}

	//! Writes intersection data into binary file by file descriptor.
	//! @param fptr output file descriptor.
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

//! Class representing total penetration of single ray into object.
class iARayPenetration
{
public:
	int m_X, m_Y;           //!< ray X, Y coordinates on plate
	float totalPenetrLen;   //!< total length of all ray penetrations
	float avDipAng;         //!< average dip angle cos of ray
	unsigned int penetrationsSize; //!< number of all ray penetrations

	iARayPenetration(){
		penetrationsSize=0;
		m_write2FileSize = sizeof(m_X)+sizeof(m_Y)+sizeof(totalPenetrLen)+sizeof(avDipAng)+sizeof(penetrationsSize);
	}
	iARayPenetration(int a_X, int a_Y, float a_totalPenetrLen, float a_avDip):
		m_X(a_X),
		m_Y(a_Y),
		totalPenetrLen(a_totalPenetrLen),
		avDipAng(a_avDip)
	{}
	//! Writes ray penetrations data into binary file by file descriptor.
	//! @param fptr output file descriptor.
	inline void write2BinaryFile(FILE *fptr)
	{
		fwrite(this, m_write2FileSize, 1, fptr);
	}
	unsigned int getWrite2FileSize(){return m_write2FileSize;}
protected:
	unsigned int m_write2FileSize;
};

//! Class representing single object render.
//!
//! Contains all data of single object render with some set of parameters.
//! Parameters are: rotations about X and Y axes, object's position, average penetration length of render,
//! average dip angle cos of render.
//! Also contains all statistical data as: all intersections data, all rays' penetrations data.
class iARenderFromPosition
{
public:
	iARenderFromPosition()
	{
		m_headerSize = sizeof(rotX)+sizeof(rotY)+sizeof(rotZ)+sizeof(pos)+sizeof(avPenetrLen)+sizeof(avDipAngle)+sizeof(maxPenetrLen)+sizeof(badAreaPercentage)+sizeof(raysSize);
		rawPtrRaysVec.clear();
	};
	~iARenderFromPosition()
	{
		clear();
	}
	//order of declarations is important when writing to file
	float rotX, rotY, rotZ;  //!< rotation about X, Y and Z axis
	float pos[3];            //!< object's position
	float avPenetrLen;       //!< average penetration length
	float avDipAngle;        //!< average dip angle cos
	float maxPenetrLen;      //!< maximum penetration length in rendering
	float badAreaPercentage; //!< percentage of bad surface area corresponding to radon space analysis
	unsigned int raysSize;   //!< number of rays
	std::vector<iARayPenetration*> rays; //!<rays' penetrations data
	std::vector<iARayPenetration*> rawPtrRaysVec;
	std::vector<iAIntersection*> intersections; //!< intersections data
	unsigned int intersectionsSize; //!< number of intersections

	//! Clears all statistical data (penetrations and intersectoins data).
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
	//! Writes all rendering data into binary file by file descriptor.
	//! @param fptr output file descriptor.
	//! @param saveAdditionalData if true, write also rays and intersections to file
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
	static int getSkipedSizeInFile()
	{
		return sizeof(float)*10;//skip rotx, roty, rotz, avpenlen, avdipangle, maxpenlen, badAreaPercentage, position//20+4*3
	}
protected:
	unsigned int m_headerSize; //!< used when writing to binary file
};

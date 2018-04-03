/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
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

#include "common.h"

#include <vector>

//using namespace Raytracer;
/**	\class CutAAB.
\brief Axis aligned box.

Used for selecting some area on specimen in order to calculate only that area.	
*/
class CutAAB
{
public:
	CutAAB(QString name) 
	{
		m_name = QString(name);
		box.setData(0,0,0,0,0,0);
		//m_sizeInFile = sizeof(aabb) + sizeof(slidersValues);
	};
	CutAAB()
	{
		m_name = "";
		box.setData(0,0,0,0,0,0);
		//m_sizeInFile = sizeof(aabb) + sizeof(slidersValues);
	};
	QString name() {return m_name;}
	inline QString GetDimString() 
	{
		return "("+QString::number(box.x1)+", "+QString::number(box.x2)+", "
		+QString::number(box.y1)+", "+QString::number(box.y2)+", "
		+QString::number(box.z1)+", "+QString::number(box.z2)+")";
	}
	void SetSlidersValues(int v1, int v2, int v3, int v4, int v5, int v6)
	{
		slidersValues[0] = v1;
		slidersValues[1] = v2;
		slidersValues[2] = v3;
		slidersValues[3] = v4;
		slidersValues[4] = v5;
		slidersValues[5] = v6;
	}
	/**
	* Write aabb data in file
	*/
	inline void Write2File(FILE *fptr)
	{	
		fwrite(&box, sizeof(aabb), 1, fptr);
		fwrite(slidersValues, sizeof(slidersValues), 1, fptr);
	}
	aabb box; //< bounds of 3D box
	int slidersValues[6];//<corresponding values of sliders
	static const int getSkipedSizeInFile()
	{
		return (sizeof(aabb)+sizeof(int[6]));//box+slidersValues
	}
private:
	QString m_name;//<name of box
};

class CutFigList
{
public:
	CutFigList() 
	{
		m_curIndex = 0;
	}
	~CutFigList() 
	{
		size_t listSize = list.size();
		for (size_t i=0; i<listSize; i++)
		{
			delete list[i];
		}
		list.clear();
		aabbs.clear();
	}
	inline void clear()
	{
		size_t listSize = list.size();
		for (size_t i=0; i<listSize; i++)
		{
			delete list[i];
		}
		list.clear();
		aabbs.clear();
		m_curIndex = 0;
	}
	inline int add(CutAAB* item)
	{
		list.push_back(item); 
		aabbs.push_back(&item->box);
		return (int)(list.size()-1);
	}
	inline int remove(int index)
	{
		if(list[index])
		{
			delete list[index];
			list.erase(list.begin()+index);
			aabbs.erase(aabbs.begin()+index);
			if(m_curIndex >= (int)list.size())
				m_curIndex = (int)list.size() - 1;
			return 1;
		}
		else
			return 0;
	}
	inline int count(){return (int)list.size();}
	inline CutAAB * item(int index) {return list[index];}
	std::vector<CutAAB*>  list;
	std::vector<aabb*>  aabbs;
	inline int curIndex() {return m_curIndex;}
	inline void SetCurIndex(int curInd) {m_curIndex = curInd;}
private:
	int m_curIndex;
};

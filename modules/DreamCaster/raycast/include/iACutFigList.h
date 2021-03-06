/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iADreamCasterCommon.h"

#include <vector>

//! Axis aligned box. Used for selecting some area on specimen in order to calculate only that area.
class iACutAAB
{
public:
	iACutAAB(QString name)
	{
		m_name = QString(name);
		box.setData(0,0,0,0,0,0);
		//m_sizeInFile = sizeof(aabb) + sizeof(slidersValues);
	};
	iACutAAB()
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
	//! Write aabb data in file
	inline void Write2File(FILE *fptr)
	{
		fwrite(&box, sizeof(iAaabb), 1, fptr);
		fwrite(slidersValues, sizeof(slidersValues), 1, fptr);
	}
	iAaabb box; //!< bounds of 3D box
	int slidersValues[6];//!< corresponding values of sliders
	static int getSkipedSizeInFile()
	{
		return (sizeof(iAaabb)+sizeof(int[6])); // box+slidersValues
	}
private:
	QString m_name; //! <name of box
};

class iACutFigList
{
public:
	iACutFigList()
	{
		m_curIndex = 0;
	}
	~iACutFigList()
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
	inline int add(iACutAAB* item)
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
	inline iACutAAB * item(int index) {return list[index];}
	std::vector<iACutAAB*>  list;
	std::vector<iAaabb*>  aabbs;
	inline int curIndex() {return m_curIndex;}
	inline void SetCurIndex(int curInd) {m_curIndex = curInd;}
private:
	int m_curIndex;
};

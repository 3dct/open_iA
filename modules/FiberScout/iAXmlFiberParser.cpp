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
 
#include "pch.h"
#include "iAXmlFiberParser.h"


iAXmlFiberParser::iAXmlFiberParser(QFile* file)
{
	this->file = file;
	textStream = new QTextStream(file);
}

iAXmlFiberParser::~iAXmlFiberParser()
{
	delete textStream;
}

FiberInfo iAXmlFiberParser::getFiberInfo(bool* ok)
{
	FiberInfo fi;
	*ok = true;	// this flag take false if we get damaged fiber info

	currentRow = textStream->readLine();

	fi.x1 = currentRow.section(",", 1, 1).toDouble(*ok ? ok : (bool *)0);
	fi.y1 = currentRow.section(",", 2, 2).toDouble(*ok ? ok : (bool *)0);
	fi.z1 = currentRow.section(",", 3, 3).toDouble(*ok ? ok : (bool *)0);

	fi.x2 = currentRow.section(",", 4, 4).toDouble(*ok ? ok : (bool *)0);
	fi.y2 = currentRow.section(",", 5, 5).toDouble(*ok ? ok : (bool *)0);
	fi.z2 = currentRow.section(",", 6, 6).toDouble(*ok ? ok : (bool *)0);

	fi.straightLength	= currentRow.section(",",  7,  7).toDouble(*ok ? ok : (bool *)0);
	fi.curvedLength		= currentRow.section(",",  8,  8).toDouble(*ok ? ok : (bool *)0);
	fi.diameter			= currentRow.section(",",  9,  9).toDouble(*ok ? ok : (bool *)0);
	fi.surfaceArea		= currentRow.section(",", 10, 10).toDouble(*ok ? ok : (bool *)0);
	fi.volume			= currentRow.section(",", 11, 11).toDouble(*ok ? ok : (bool *)0);

	fi.isSeperated	= currentRow.section(",", 12, 12).toInt(*ok ? ok : (bool *)0) == 0? false : true;
	fi.isCurved		= currentRow.section(",", 13, 13).toInt(*ok ? ok : (bool *)0) == 0? false : true;

	return fi;
}
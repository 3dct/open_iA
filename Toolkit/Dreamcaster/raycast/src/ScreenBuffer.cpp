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
#include "../include/ScreenBuffer.h"

//namespace Raytracer {

iAScreenBuffer::iAScreenBuffer( int a_Width, int a_Height ) :
	m_Width( a_Width ),
	m_Height( a_Height )
{
	m_Buffer = new unsigned int[a_Width * a_Height];
	m_buffSize = m_Width * m_Height * sizeof(m_Buffer[0]);
	clear();
	//memset(m_Buffer, 0, a_Width * a_Height);
}

iAScreenBuffer::~iAScreenBuffer()
{
	if(m_Buffer)
		delete [] m_Buffer;
}

void iAScreenBuffer::clear()
{
	memset(m_Buffer, 0, m_buffSize);
}

//}; // namespace Raytracer

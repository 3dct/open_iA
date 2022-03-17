/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAMhdFileInfo.h"

#include <QStringList>

iAMhdFileInfo::iAMhdFileInfo( QString fileName )
{
	parseFile( fileName );
}


void iAMhdFileInfo::getFileDimSize( double* dimSize )
{
	dimSize[0] = m_dimSize[0];
	dimSize[1] = m_dimSize[1];
	dimSize[2] = m_dimSize[2];
}


void iAMhdFileInfo::getElementSpacing( double* spacing )
{
	spacing[0] = m_elemSpacing[0];
	spacing[1] = m_elemSpacing[1];
	spacing[2] = m_elemSpacing[2];
}


void iAMhdFileInfo::parseFile( QString fileName )
{
	m_file.setFileName( fileName );
	if( !m_file.open( QIODevice::ReadOnly ) ) {
		return;
	}

	while( !m_file.atEnd( ) ) {
		QString line = QString( m_file.readLine( ) );
		QStringList strList = line.split( ' ' );
		if( strList[0] == "DimSize" ) {
			m_dimSize[0] = strList[2].toDouble( );
			m_dimSize[1] = strList[3].toDouble( );
			m_dimSize[2] = strList[4].toDouble( );
		}
		else if( strList[0] == "ElementSpacing" ) {
			m_elemSpacing[0] = strList[2].toDouble( );
			m_elemSpacing[1] = strList[3].toDouble( );
			m_elemSpacing[2] = strList[4].toDouble( );
		}
		else {
			continue;
		}
	}
}

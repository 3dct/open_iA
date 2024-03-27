// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMhdFileInfo.h"

#include <QFile>
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
	QFile file(fileName);
	if( !file.open( QIODevice::ReadOnly ) )
	{
		return;
	}
	while( !file.atEnd( ) ) 
	{
		QString line = QString( file.readLine( ) );
		QStringList strList = line.split( ' ' );
		if( strList[0] == "DimSize" )
		{
			m_dimSize[0] = strList[2].toDouble( );
			m_dimSize[1] = strList[3].toDouble( );
			m_dimSize[2] = strList[4].toDouble( );
		}
		else if( strList[0] == "ElementSpacing" )
		{
			m_elemSpacing[0] = strList[2].toDouble( );
			m_elemSpacing[1] = strList[3].toDouble( );
			m_elemSpacing[2] = strList[4].toDouble( );
		}
	}
}

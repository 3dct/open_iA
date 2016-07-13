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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#pragma once

//#define iALookupTable_USE_VTK //!! uncomment if you want to allow VTK lookup table input

#include <QList>
#include <vtkLookupTable.h>

//! Class representing lookup table for color coding used in scatter plot matrix (SPLOM).
/*!
	Has methods for importing existing VTK lookup table (!if VTK is enabled via a preprocessor flag!) 
	and mapping scalar values to a corresponding QColor.
*/
class iALookupTable
{
public:
	iALookupTable() 
		: m_isInitialized( false ),
		m_rangeLen( 1.0 ),
		m_numColors( 0 )
	{
		m_range[0] = m_range[1] = 0.0;
	}

	explicit iALookupTable( vtkLookupTable * vtk_lut ) : m_isInitialized( false )
	{
		copyFromVTK( vtk_lut );
	}

	//!  Copies data from the existing VTK lookup table (vtkLookupTable)
	void copyFromVTK( vtkLookupTable * vtk_lut )
	{
		m_numColors = vtk_lut->GetNumberOfTableValues();
		allocate( m_numColors );
		setRange( vtk_lut->GetRange() );
		for( unsigned long i = 0; i < m_numColors; ++i )
		{
			double rgba[4];
			vtk_lut->GetTableValue( i, rgba );
			setColor( i, rgba );
		}
		m_isInitialized = true;
	}

	//!  Map a scalar value into an RGBA color.
	void getColor( double val, double * rgba_out )
	{
		if( val  < m_range[0] )
		{
			for( unsigned long i = 0; i < 4; ++i )
				rgba_out[i] = m_data[i];
		}
		else if( val > m_range[1] )
		{
			unsigned long offset = m_data.size() - 4;
			for( unsigned long i = 0; i < 4; ++i )
				rgba_out[i] = m_data[offset + i];
		}
		else
		{
			double t = ( val - m_range[0] ) / m_rangeLen;
			unsigned long index = t * m_numColors;
			index *= 4;
			for( unsigned long i = 0; i < 4; ++i )
				rgba_out[i] = m_data[index++];
		}
	}

	//!  Allocate place for a given number of colors and fill with zeros.
	void allocate( unsigned long numberOfColors )
	{
		m_numColors = numberOfColors;
		m_data.clear();
		unsigned long size = m_numColors * 4;
		for( unsigned long i = 0; i < size; ++i )
			m_data.push_back( 0.0 );
		m_isInitialized = true;
	}

	//!  Assign an RGBA color to a given index in the table.
	void setColor( unsigned long colInd, double * rgba )
	{
		int maxInd = ( colInd + 1 ) * 4;
		if( m_data.size() < maxInd )
			return;
		unsigned long offset = colInd * 4;
		for( unsigned long i = 0; i < 4; ++i )
			m_data[offset++] = rgba[i];
	}

	//!  Assign an QColor color to a given index in the table.
	void setColor( unsigned long colInd, QColor & col )
	{
		double rgba[4] = { col.redF(), col.greenF(), col.blueF(), col.alphaF(), };
		setColor(colInd, rgba);
	}
	
	//! Fill the lookup table using provided raw RBGA data for a given number of colors.
	void setData( unsigned long numberOfColors, double * rgba_data )
	{
		m_numColors = numberOfColors;
		allocate( m_numColors );
		unsigned long offset = 0;
		for( unsigned long i = 0; i < m_numColors; ++i )
		{
			setColor( i, &rgba_data[offset] );
			offset += 4;
		}
	}

	//! Set a given alpha value for every color in the table
	void setOpacity( double alpha )
	{
		unsigned long offset = 0;
		for( unsigned long i = 0; i < m_numColors; ++i )
		{
			m_data[offset + 3] = alpha;
			offset += 4;
		}
	}

	/* Setters/Getters */
	const double * getRange() const { return m_range; }						//!< Get the mapped scalar range.
	void setRange( double from_val, double to_val )							//!< Set the mapped scalar range.
	{ 
		m_range[0] = from_val; m_range[1] = to_val; 
		m_rangeLen = m_range[1] - m_range[0]; 
	}
	void setRange( double * range ) { setRange( range[0], range[1] ); }		//!< Set the mapped scalar range.
	bool initialized() const { return m_isInitialized; }					//!< Check if the table has data (initialized).

protected:
	bool m_isInitialized;					///< flag which is on if lookup table data is set
	QList<double> m_data;					///< lookup table raw color data, each color is 4 doubles (RGBA)
	double m_range[2];						///< scalar range mapped by the lookup table 
	double m_rangeLen;						///< length of the total scalar range that is mapped by the lookup table
	unsigned long m_numColors;				///< number of colors stored in the lookup table
};

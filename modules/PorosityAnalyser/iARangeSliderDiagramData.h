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
 
#ifndef IARANGESLIDERDIAGRAMDATA_H
#define IARANGESLIDERDIAGRAMDATA_H

#include "iAAbstractDiagramData.h"

#include <QSharedPointer>

class iARangeSliderDiagramData : public iAAbstractDiagramRangedData
{
public:
	iARangeSliderDiagramData( QList<double> m_rangeSliderData, double min, double max );
	~iARangeSliderDiagramData();
	void updateRangeSliderFunction();
	
	virtual DataType const * GetData() const;
	virtual size_t GetNumBin() const;

	virtual double GetSpacing() const
	{
		if ( GetNumBin() <= 1 )
			return 0.0;
		
		return ( m_range[1] - m_range[0] ) / (GetNumBin() - 1.0);
	}

	virtual double * GetDataRange()
	{
		return m_range;
	}

	virtual double GetDataRange( int idx ) const
	{
		return m_range[idx];
	}

	virtual DataType GetMaxValue() const 
	{
		double max = 0;
		for ( int i = 0; i < m_rangeSliderData.size(); ++i )
		{
			if ( m_rangeSliderData.at( i ) > max )
				max = m_rangeSliderData.at( i );
		}
		return max;
	}

	virtual DataType GetMinValue() const
	{
		double min = 0;
		for ( int i = 0; i < m_rangeSliderData.size(); ++i )
		{
			if ( m_rangeSliderData.at( i ) < min )
				min = m_rangeSliderData.at( i );
		}
		return min;
	}

private:
	DataType* m_rangeSliderFunction;
	QList<double> m_rangeSliderData;

	double m_range[2];
};

#endif /* IARANGESLIDERDIAGRAMDATA_H */

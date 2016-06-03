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
 
#ifndef iASPLOMData_h__
#define iASPLOMData_h__

#include <QTableWidget>

//! Class for storing data shown in scatter plot matrix (SPLOM).
/*! 
	Stores data points, parameter names as well as number of data points and parameters.
*/
class iASPLOMData
{
public:
	iASPLOMData() : m_numParams( 0 ), m_numPoints( 0 ) {};
	explicit iASPLOMData( const QTableWidget * tw ) { import( tw ); }

	//! Free all the data.
	void clear()
	{
		m_paramNames.clear();
		m_dataPoints.clear();
	}

	//! Methods that imports data from a QTableWidget and stores them in a list of double-lists.
	void import( const QTableWidget * tw )
	{
		clear();

		m_numParams = tw->columnCount();
		m_numPoints = tw->rowCount() - 1;
		if( m_numPoints < 0 )
			m_numPoints = 0;
		for( unsigned long c = 0; c < m_numParams; ++c )
		{
			m_paramNames.push_back( tw->item( 0, c )->text() );
			m_dataPoints.push_back( QList<double>() );
			QList<double> * paramData = &m_dataPoints[c];
			for( unsigned long r = 1; r < m_numPoints + 1; ++r )
				paramData->push_back( tw->item( r, c )->text().toDouble() );
		}
	}

	//! Get constant reference to the lists containing raw data points.
	const QList<QList<double>> & data() const { return m_dataPoints; }

	//! Get constant reference to the list containing raw data points of a given parameter (index).
	const QList<double> & paramData( int paramIndex ) const { return m_dataPoints[paramIndex]; }

	//! Get parameter name by its index.
	QString parameterName( int paramIndex ) const { return m_paramNames[paramIndex]; }

	//! Get number of data point parameters.
	unsigned long numParams() const { return m_numParams; }

	//! Get number of data points.
	unsigned long numPoints() const { return m_numPoints; }

protected:
	QList<QString> m_paramNames;		///< list of parameter names
	QList<QList<double>> m_dataPoints;	///< lists containing data points
	unsigned long m_numParams;			///< total number of data point parameters
	unsigned long m_numPoints;			///< total number of data points
};

#endif // iASPLOMData_h__
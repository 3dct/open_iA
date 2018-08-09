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
#pragma once

#include "open_iA_Core_export.h"

#include <cstddef>    // for size_t
#include <vector>

class QString;
class QTableWidget;

//! Class for storing data shown in a scatter plot matrix (SPLOM)
//! (a table with data values for one object per row, along with the names of the columns/parameters).
class open_iA_Core_API iASPLOMData
{
public:
	iASPLOMData();
	explicit iASPLOMData(const QTableWidget * tw);    //!< Create data from a QTableWidget
	void clear();                                     //!< Free all the data.
	void import(const QTableWidget * tw);             //!< Imports data from a QTableWidget and stores it in a list of double-lists.
	std::vector<std::vector<double>> & data();        //!< Get the table values
	std::vector<QString> & paramNames();              //!< Get the names of the columns/parameters
	const std::vector<std::vector<double>> & data() const; //!< Get constant ref. to the lists containing raw data points.
	const std::vector<double> & paramData(size_t paramIndex) const; //!< Get constant ref. to the list containing raw data points of a given parameter (index).
	QString parameterName(size_t paramIndex) const;   //!< Get parameter name by its index.
	size_t paramIndex(QString const & paramName) const; //!< Get the index of a specified parameter name.
	size_t numParams() const;                         //!< Get number of data point parameters.
	size_t numPoints() const;                         //!< Get number of data points.
	bool isInverted(size_t paramIndex);               //!< Get whether the axis of a parameter should be inverted in the scatter plots.
	void setInverted(size_t paramIndex, bool isInverted);//!< Set whether the axis of a parameter should be inverted in the scatter plots.
	void setParameterNames(std::vector<QString> const & names); //! Set the parameter names (clears all columns)
protected:
	std::vector<QString> m_paramNames;                //!< list of parameter names
	std::vector<std::vector<double>> m_dataPoints;    //!< lists containing data points
	std::vector<bool> m_inverted;                     //!< whether to invert a feature
};

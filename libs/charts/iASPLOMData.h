/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAcharts_export.h"

#include <QObject>

#include <cstddef>    // for size_t
#include <vector>

class QString;

//! Class for storing data shown in a scatter plot matrix (SPLOM)
//! (a table with data values for one object per row, along with the names of the columns/parameters).
class iAcharts_API iASPLOMData : public QObject
{
	Q_OBJECT
public:
	static const size_t NoDataIdx;
	iASPLOMData();
	void setParameterNames(std::vector<QString> const& paramNmes, size_t rowReserve = 0);  //! Set the parameter names (clears all columns) with an optional row "size" (i.e. how many rows are planned to be there, i.e. used in vector::reserve)
	std::vector<std::vector<double>> & data();        //!< Get the table values
	std::vector<QString> & paramNames();              //!< Get the names of the columns/parameters
	const std::vector<std::vector<double>> & data() const; //!< Get constant ref. to the lists containing raw data points.
	const std::vector<double> & paramData(size_t paramIndex) const; //!< Get constant ref. to the list containing raw data points of a given parameter (index).
	QString parameterName(size_t paramIndex) const;   //!< Get parameter name by its index.
	size_t paramIndex(QString const & paramName) const; //!< Get the index of a specified parameter name.
	size_t numParams() const;                         //!< Get number of data point parameters.
	size_t numPoints() const;                         //!< Get number of data points.
	void addParameter(QString& name);                 //!< Add an additional column
	double const* paramRange(size_t paramIndex) const;//!< Get the range of the parameter with given index
	void updateRanges();                              //!< update range of all parameters
	void updateRanges(std::vector<size_t> paramIndices); //!< update range for multiple parameters. Call if data of multiple parameters has changed
	void updateRange(size_t paramIndex);              //!< update range of a single parameter. Call if data of a parameter has changed
signals:
	void dataChanged(size_t paramIndex);              //!< emitted when the range of a parameter has changed
protected:
	std::vector<QString> m_paramNames;                //!< list of parameter names
	std::vector<std::vector<double>> m_dataPoints;    //!< lists containing data points
	std::vector<std::vector<double> > m_ranges;       //!< ranges of all parameters
private:
	void updateRangeInternal(size_t paramIndex);      //!< Update internal range data for parameter paramIndex
};

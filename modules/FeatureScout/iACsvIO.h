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
#pragma once

#include "iAFeatureScoutObjectType.h"
#include "iACsvConfig.h"

#include "FeatureScout_export.h"

#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QVector>

class QTextStream;

//! interface used by iACsvIO for creating the actual table
//! subclass for each kind of table that is specifically required somewhere
//! (e.g. vtkTable, QTableWidget)
//! @see iACsvVtkTableCreator, iACsvQTableCreator
class iACsvTableCreator
{
public:
	virtual void initialize(QStringList const & headers, size_t const rowCount) = 0;
	virtual void addRow(size_t row, QStringList const & values) = 0;
};

//! class for reading a csv into a table, using given options
class FeatureScout_API iACsvIO
{
public:
	static const char * ColNameAutoID;  //!< name of the auto ID column (inserted optionally)
	static const char * ColNameClassID; //!< name of the class ID column (always inserted as last column)
	iACsvIO();
	//! reads table entries from csv file
	bool loadCSV(iACsvTableCreator & dstTbl, iACsvConfig const & params,
		size_t const rowCount = std::numeric_limits<size_t>::max());
	//! get the list of columns/headers as it is in the file
	const QStringList & getFileHeaders() const;
	//! get list of all headers in result table (including computed columns)
	const QStringList & getOutputHeaders() const;
	//! get mapping in which fields the important values are stored
	QSharedPointer<QMap<uint, uint>> getOutputMapping() const;
private:
	QStringList m_fileHeaders;          //!< list of column header names in file
	QStringList m_outputHeaders;        //!< list of column header names in result table
	iACsvConfig m_csvConfig;            //!< settings used for reading the csv
	QSharedPointer<QMap<uint, uint> > m_outputMapping;   //!< maps a value identifier (given as a value out of the iACsvConfig::MappedColumn enum) to the index of the column in the output which contains this value

	//! determine the header columns used in the output
	void determineOutputHeaders(QVector<int> const & selectedCols);
	//! determine how man actual data rows the result table will have
	size_t calcRowCount(QTextStream& in, size_t const skipLinesStart,
		size_t const skipLinesEnd);
	//! determine the indices of the selected columns
	QVector<int> computeSelectedColIdx();
};

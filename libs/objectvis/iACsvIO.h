// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACsvConfig.h"

#include "iaobjectvis_export.h"

#include <iAVec3.h>

#include <QStringList>

#include <map>
#include <vector>

class QTextStream;

//! Interface used by iACsvIO for creating a actual table from .csv data.
//! Subclass for each kind of table that is specifically required somewhere
//! (e.g. vtkTable, QTableWidget)
//! @see iACsvVtkTableCreator, iACsvQTableCreator
class iACsvTableCreator
{
public:
	virtual void initialize(QStringList const & headers, size_t const rowCount) = 0;
	virtual void addRow(size_t row, std::vector<double> const & values) = 0;
};

//! Class for reading a csv file into a table, using given options.
class iAobjectvis_API iACsvIO
{
public:
	static const char * ColNameAutoID;  //!< name of the auto ID column (inserted optionally)
	static const char * ColNameClassID; //!< name of the class ID column (always inserted as last column)
	iACsvIO();
	//! reads table entries from csv file
	bool loadCSV(iACsvTableCreator & dstTbl, iACsvConfig const & params,
		size_t const rowCount = std::numeric_limits<size_t>::max());
	//! get the list of columns/headers as it is in the file
	const QStringList & fileHeaders() const;
	//! get list of all headers in result table (including computed columns)
	const QStringList & outputHeaders() const;
	//! get mapping in which fields the important values are stored.
	//! This is basically the column mapping of the iACsvConfig used to load the dataset,
	//! but adapted / extended to match the created output: When an auto-id is inserted as first column,
	//! all indices shift by one to the back; also for computed columns, mappings are inserted.
	iAColMapP outputMapping() const;
private:
	QStringList m_fileHeaders;          //!< list of column header names in file
	QStringList m_outputHeaders;        //!< list of column header names in result table
	iACsvConfig m_csvConfig;            //!< settings used for reading the csv
	iAColMapP m_outputMapping;          //!< maps a value identifier (given as a value out of the iACsvConfig::MappedColumn enum) to the index of the column in the output which contains this value

	//! determine the header columns used in the output
	void determineOutputHeaders(QVector<uint> const & selectedCols);
	//! determine how man actual data rows the result table will have
	size_t calcRowCount(QTextStream& in, size_t const skipLinesStart,
		size_t const skipLinesEnd);
	//! determine the indices of the selected columns
	QVector<uint> computeSelectedColIdx();
};

//! read the curved fiber info file
iAobjectvis_API bool readCurvedFiberInfo(QString const& fileName, std::map<size_t, std::vector<iAVec3f>>& outMap);

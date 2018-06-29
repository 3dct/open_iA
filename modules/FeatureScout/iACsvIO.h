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

#include "iAFeatureScoutObjectType.h"
#include "iACsvConfig.h"

#include <vtkSmartPointer.h>

#include <QString>
#include <QVector>

class vtkTable;
class QTableWidget;
class QTextStream;

class iACsvIO
{
public:
	static const char * ColNameAutoID;
	static const char * ColNameClassID;
	iACsvIO();
	bool loadCsvFile(iAFeatureScoutObjectType fid, QString const & fileName);
	vtkTable * getCSVTable();
	void setColumnHeaders(QStringList & colHeaders);
	bool loadCSVCustom(iACsvConfig const & cnfg_params);

	void setParams(QStringList& headers, QVector<uint> const & colIDs);
	static QStringList getFibreElementsName(bool withUnit);
	
	void debugTable(const bool useTabSeparator);

	//! @{ migrated from DataTable
	//! reads table entries from csv file
	bool readTableEntries(QTableWidget* dstTbl, iACsvConfig const & params, const size_t rowCount);
	const QStringList & getHeaders() const;
private:
	QStringList m_headerEntries; //!< list of column header names <-> TODO: duplicate of m_TableHeaders ??
	QString m_LastEncoding;      //!< encoding used when last reading the csv file
	void readTableValues(QTableWidget* dstTbl, size_t const rowCount, QTextStream &file, bool addAutoID, const QString & colSeparator);
	void addLineToTable(QTableWidget* dstTbl, QStringList const &tableEntries, size_t row, bool addAutoID);
	void prepareHeader(QTableWidget* dstTbl, size_t skipLinesStart, QTextStream &file, bool readHeaders, bool addAutoID, const QString & colSeparator);
	//! @}

	bool readCustomFileEntries();
	void loadFeatureData(QTextStream &in, size_t const colCount, size_t const rowCount);
	size_t calcRowCount(QTextStream& in, size_t const skipLinesStart, size_t const skipLinesEnd);
	void fibreCalculation(QTextStream & in, size_t const colCount, size_t const rowCount, bool const useOldFeatureScoutFormat);
	int assingFiberValuesPart_2(int i, int col_idx, double phi, double theta, double xm, double ym, double zm);
	int assignFiberValuesPart1(int i, int col_idx, double a11, double a22, double a33, double a12, double a13, double a23);
	bool loadFibreCSV(const QString &fileName);
	bool loadPoreCSV(const QString &fileName);

	bool useCVSOnly;	//!< indicates whether we read a custom csv format or a "legacy" FiberScout/FeatureScout fiber/pore file
	bool enableFiberTransformation; //!< indicates whether to compute fiber characteristics

	iACsvConfig m_csvConfig;
	vtkSmartPointer<vtkTable> table;
	QVector<uint> m_colIds;
	QStringList  m_TableHeaders;
};

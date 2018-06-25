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

#include "iAObjectAnalysisType.h"
#include "csv_config.h"

#include <vtkSmartPointer.h>

#include <QString>
#include <QVector>

class vtkTable;
class QTextStream;

class iACsvIO
{
public:
	iACsvIO();
	bool LoadCsvFile(iAObjectAnalysisType fid, QString const & fileName);
	vtkTable * GetCSVTable();

	void setColumnHeaders(QStringList & colHeaders);
	bool loadCSVCustom(csvConfig::configPararams &cnfg_params);
	void setTableParams(csvConfig::configPararams & csv_Params);

	void setParams(QStringList& headers, const QVector<uint> &colIDs, uint TableWidth);

	void debugTable(const bool useTabSeparator);
	static QStringList GetFibreElementsName(bool withUnit);

private:

	inline void setTableHeaders(QStringList& headers) {
		this->m_TableHeaders = headers;
	}
	inline void setColIDs(const QVector<uint> &colIDs) {
		this->m_colIds = colIDs;
	}
	inline void setTableWidth(uint TableWidth) {
		this->m_tableWidth = TableWidth;
	}

	bool readCustomFileEntries(const QString & fileName, const uint skipLinesStart);
	bool loadCsv_WithConfig();
	void loadPoreData(long tableLength, QString &line, QTextStream &in, int tableWidth, QString &tmp_section, int col_count);
	long CalcTableLength(const QString &fileName, const int skipLinesStart);
	bool LoadFibreCSV(const QString &fileName);
	void FibreCalculation(QTextStream & in, int eleWidth, int tableLength, const int colCount, const bool useOldFeatureScoutFormat);
	int assingFiberValuesPart_2(int i, int col_idx, double phi, double theta, double xm, double ym, double zm);
	int assignFiberValuesPart1(int i, int col_idx, double a11, double a22, double a33, double a12, double a13, double a23);
	bool LoadPoreCSV(const QString &fileName);
	bool loadConfig(const QString configName, bool & applyEN_Formating);

	bool m_EN_Values;
	//!Mode Read Custom csv
	bool useCVSOnly;
	bool enableFiberTransformation;

	//! element id for each row entry
	ulong m_EL_ID;
	uint m_tableWidth;
	uint m_skipLinesEnd, m_skipLinesStart;

	QString configPath;
	QString m_colSeparator;
	QString m_decimalSeparator;
	QString m_FileName;

	vtkSmartPointer<vtkTable> table;
	csvConfig::CTInputObjectType inputElementType;
	QVector<uint> m_colIds;
	QStringList  m_TableHeaders;
};

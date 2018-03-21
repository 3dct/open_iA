/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "open_iA_Core_export.h"
#include <vtkSmartPointer.h>
#include "io/csv_config.h"
#include <QString>

class vtkTable;

class open_iA_Core_API iACsvIO
{
public:
	iACsvIO();
	bool LoadCsvFile(iAObjectAnalysisType fid, QString const & fileName);
	vtkTable * GetCSVTable();

	bool loadCsv_WithConfig(const QString & fileName, csvConfig::configPararams &csv_Params);
	void readFileEntries(const QString & fileName, const int rows_toSkip, const QString & colSeparator, const QString decimalSeparator, bool En_Values, bool & retFlag);

	//input parameters from configuration file 
	//headerlines to skip	nrOfHeaderLines: headerLinesToSkip
	//startRowInd -> where to start from row
	//column separator "," "\t", ";"

	/*bool loadCSVData(const QString &fileName, const int nrOfHeaderLines, const int NrOFParameters, const int startRowInd, const char separator);*/
	
	//similar to load pore csv
	
	//bool loadGeneralCSVFile(const QString &FName, const QString *confName );
	
	bool loadCSVCustom(csvConfig::configPararams &cnfg_params);
	bool loadConfigurationFile(csvConfig::configPararams &cnf_Params) const;
	
	//todo check if file path exists
	inline void setConfigPath(const QString _configPath){
		configPath = _configPath; 
	}

	const csvConfig::CTInputObjectType getInputElementType() const {
		return this->inputElementType; 
	}

private:

	void setDefaultConfigPath(); 
	int CalcTableLength(const QString &fileName, const int *nrHeadersToSkip);  //
	
	QString configPath; 
	QStringList GetFibreElementsName(bool withUnit);
	bool LoadFibreCSV(const QString &fileName);
	bool LoadPoreCSV(const QString &fileName);
	
	bool loadConfig(const QString configName, bool & applyEN_Formating);
	vtkSmartPointer<vtkTable> table;
	csvConfig::CTInputObjectType inputElementType; 
};

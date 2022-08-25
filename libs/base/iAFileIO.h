/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAAttributes.h"
#include "iADataSet.h"
#include "iADataSetType.h"

#include <QMap>
#include <QString>

#include <memory>
#include <vector>

class iAProgress;

//! Base class for dataset readers within open_iA
//! Derived classes can be registered via iAFileTypeRegistry
class iAbase_API iAFileIO
{
public:
	static const QString CompressionStr;
	//! create a file I/O for the given dataset type
	iAFileIO(iADataSetTypes readTypes, iADataSetTypes writeTypes);
	//! virtual destructor for proper cleanup
	virtual ~iAFileIO();
	//! The name of the file type that this IO supports
	virtual QString name() const = 0;
	//! The file extensions that this file IO should be used for
	virtual QStringList extensions() const = 0;

	//! Load the (list of) dataset(s). The default implementation assumes that reading is not implemented and does nothing.
	virtual std::vector<std::shared_ptr<iADataSet>> load(QString const& fileName, iAProgress* progress, QVariantMap const& paramValues);
	//! Required parameters for loading the file
	//! Copied from iAFilter - maybe reuse? move to new common base class iAParameterizedSomething ...?
	iAAttributes const& parameters() const;
	//! Types of dataset contained in this file format, which this IO can read 
	iADataSetTypes supportedLoadDataSetTypes() const;

	//! Whether this IO can be used for storing the given data set.
	//! It could for example check whether the format supports the data types in the dataset
	//! The default implementation here always returns true
	virtual bool isDataSetSupported(std::shared_ptr<iADataSet> dataSet, QString const& fileName) const;
	//! Save the (list of) dataset(s). The default implementation assumes that writing is not implemented and does nothing.
	virtual void save(QString const& fileName, iAProgress* progress, std::vector<std::shared_ptr<iADataSet>> const& dataSets, QVariantMap const& paramValues);
	//! Types of dataset contained in this file format, which this IO can write 
	iADataSetTypes supportedSaveDataSetTypes() const;

protected:
	//! Adds the description of a parameter to the filter.
	//! @param name the parameter's name
	//! @param valueType the type of value this parameter can have
	//! @param defaultValue the default value of the parameter; for Categorical
	//!     valueTypes, this should be the list of possible values
	//! @param min the minimum value this parameter can have (inclusive).
	//! @param max the maximum value this parameter can have (inclusive)
	void addParameter(QString const& name, iAValueType valueType, QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max());

private:
	iAAttributes m_parameters;
	iADataSetTypes m_loadDataSetTypes;
	iADataSetTypes m_saveDataSetTypes;
};
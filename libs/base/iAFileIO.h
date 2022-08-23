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
	//! create a file I/O for the given dataset type
	iAFileIO(iADataSetTypes type);
	//! set up file I/O for the given file name
	//! TODO: make possible to also use e.g. folder name or list of files
	virtual void setup(QString const& fileName);
	virtual ~iAFileIO();
	//! The name of the file type that this IO supports
	virtual QString name() const = 0;
	//! The file extensions that this file IO should be used for
	virtual QStringList extensions() const = 0;
	//! Load the (list of) dataset(s)
	virtual std::vector<std::shared_ptr<iADataSet>> load(iAProgress* progress, QVariantMap const& paramValues) = 0;
	//! Required parameters for loading the file
	//! Copied from iAFilter - maybe reuse? move to new common base class iAParameterizedSomething ...?
	iAAttributes const& parameters() const;
	//! Types of dataset that the the file format which this IO is for delivers
	iADataSetTypes supportedDataSetTypes() const;

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

	QString m_fileName;

private:
	iAAttributes m_parameters;
	iADataSetTypes m_dataSetTypes;
};

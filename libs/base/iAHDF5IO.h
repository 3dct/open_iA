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

#ifdef USE_HDF5

#include "iAbase_export.h"

#include "iAFileIO.h"

// for now, let's use HDF5 1.10 API:
#define H5_USE_110_API
#include <hdf5.h>

class iAbase_API iAHDF5IO : public iAFileIO
{
public:
	static const QString Name;
	static const QString DataSetPathStr;
	static const QString SpacingStr;

	iAHDF5IO();
	std::vector<std::shared_ptr<iADataSet>> load(QString const& fileName, iAProgress* progress, QVariantMap const& parameters) override;
	QString name() const override;
	QStringList extensions() const override;
};

iAbase_API QString MapHDF5TypeToString(H5T_class_t hdf5Type);
iAbase_API int GetNumericVTKTypeFromHDF5Type(H5T_class_t hdf5Type, size_t numBytes, H5T_sign_t sign);
iAbase_API void printHDF5ErrorsToConsole();

#endif
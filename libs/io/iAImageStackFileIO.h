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

#include "iAio_export.h"

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

class iAio_API iAImageStackFileIO : public iAFileIO, private iAAutoRegistration<iAFileIO, iAImageStackFileIO, iAFileTypeRegistry>
{
public:
	static QString const Name;
	static QString const FileNameBase;
	static QString const Extension;
	static QString const NumDigits;
	static QString const MinimumIndex;
	static QString const MaximumIndex;
	static QString const LoadTypeStr;
	static QString const SingleImageOption;
	static QString const ImageStackOption;
	iAImageStackFileIO();
	std::vector<std::shared_ptr<iADataSet>> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
	bool isDataSetSupported(std::shared_ptr<iADataSet> dataSet, QString const& fileName) const override;
	void saveData(QString const& fileName, std::vector<std::shared_ptr<iADataSet>> & dataSets, QVariantMap const& paramValues, iAProgress const& progress) override;
};

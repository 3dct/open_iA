// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

//! Support for loading/saving files via ITK (https://itk.org/Wiki/ITK/File_Formats) for the file I/O framework of open_iA.
class iAITKFileIO : public iAFileIO, private iAAutoRegistration<iAFileIO, iAITKFileIO, iAFileTypeRegistry>
{
public:
	static const QString Name;
	iAITKFileIO();
	std::shared_ptr<iADataSet> loadData(
		QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	void saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet,
		QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
};

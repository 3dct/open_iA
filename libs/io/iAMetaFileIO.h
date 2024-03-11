// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaio_export.h"

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

//! Support for loading/saving MetaImage files (https://itk.org/Wiki/ITK/MetaIO/Documentation) for the file I/O framework of open_iA.
class iAio_API iAMetaFileIO : public iAFileIO, iAAutoRegistration<iAFileIO, iAMetaFileIO, iAFileTypeRegistry>
{
public:
	iAMetaFileIO();
	std::shared_ptr<iADataSet> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	void saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
};

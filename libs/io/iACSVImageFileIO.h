// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

//! Support for storing volumes in .csv for the file I/O framework of open_iA.
class iACSVImageFileIO : public iAFileIO, private iAAutoRegistration<iAFileIO, iACSVImageFileIO, iAFileTypeRegistry>
{
public:
	iACSVImageFileIO();
	void saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
};

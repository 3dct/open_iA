// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaio_export.h"

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

//! Support for loading and storing volume stacks for the file I/O framework of open_iA.
//! Stores/Loads a .volstack file, a descriptive text file containing the parameters for finding the actual volume data files.
class iAio_API iAVolStackFileIO : public iAFileIO, private iAAutoRegistration<iAFileIO, iAVolStackFileIO, iAFileTypeRegistry>
{
public:
	static const QString Name;
	iAVolStackFileIO();
	std::shared_ptr<iADataSet> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	void saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
};

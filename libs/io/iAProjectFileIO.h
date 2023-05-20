// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaio_export.h"

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

//! Support for loading/saving a "project file" (a collection of datatasets) for the file I/O framework of open_iA.
//! ONLY stores the references to the actual datasets; storing viewing settings doesn't happen within this class.
//! It does store parameters required to load the datasets (e.g. size, spacing etc. for raw datasets).
class iAio_API iAProjectFileIO : public iAFileIO, private iAAutoRegistration<iAFileIO, iAProjectFileIO, iAFileTypeRegistry>
{
public:
	static const QString Name;
	iAProjectFileIO();
	std::shared_ptr<iADataSet> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	void saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
};

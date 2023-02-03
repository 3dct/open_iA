// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAio_export.h"

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

//! class I/O of a "project file" for a collection of datatasets
//! ONLY stores the references to the actual datasets; no viewing settings, etc.; it does store parameters required to load the data though
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

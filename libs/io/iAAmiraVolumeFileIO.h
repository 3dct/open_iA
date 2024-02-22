// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

//! Amira mesh support for the file I/O framework of open_iA.
class iAAmiraVolumeFileIO : public iAFileIO, private iAAutoRegistration<iAFileIO, iAAmiraVolumeFileIO, iAFileTypeRegistry>
{
public:
	iAAmiraVolumeFileIO();
	std::shared_ptr<iADataSet> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	void saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
};

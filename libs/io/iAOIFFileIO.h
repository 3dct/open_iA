// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

//! Support for loading Olympus image files (.oif) for the file I/O framework of open_iA.
class iAOIFFileIO : public iAFileIO, private iAAutoRegistration<iAFileIO, iAOIFFileIO, iAFileTypeRegistry>
{
public:
	static const QString Name;
	iAOIFFileIO();
	std::shared_ptr<iADataSet> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
};

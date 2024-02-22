// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

//! Support for loading graph datasets from a simple text-based description for the file I/O framework of open_iA.
class iAGraphFileIO : public iAFileIO, private iAAutoRegistration<iAFileIO, iAGraphFileIO, iAFileTypeRegistry>
{
public:
	iAGraphFileIO();
	std::shared_ptr<iADataSet> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
};

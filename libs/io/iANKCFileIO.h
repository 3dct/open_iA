// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

//! Support for loading .nkc files for the file I/O framework of open_iA.
//! Description of ultrasound measurements, including a reference to a file containig binary data.
class iANKCFileIO : public iAFileIO, private iAAutoRegistration<iAFileIO, iANKCFileIO, iAFileTypeRegistry>
{
public:
	static const QString Name;
	iANKCFileIO();
	std::shared_ptr<iADataSet> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
};

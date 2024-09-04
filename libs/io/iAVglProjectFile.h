// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

#include <vector>

//! Basic support for loading files from VG Studio for the file I/O framework of open_iA.
class iAVglProjectFile : public iAFileIO, private iAAutoRegistration<iAFileIO, iAVglProjectFile, iAFileTypeRegistry>
{
public:
	static const QString Name;
	iAVglProjectFile();
	std::shared_ptr<iADataSet> loadData(
		QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;

	std::vector<char> unzip(QString filename);
};

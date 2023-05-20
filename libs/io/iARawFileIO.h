// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaio_export.h"

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

//! Support for loading/saving raw files (that is, binary data) for the file I/O framework of open_iA.
class iAio_API iARawFileIO : public iAFileIO, private iAAutoRegistration<iAFileIO, iARawFileIO, iAFileTypeRegistry>
{
public:
	static const QString Name;
	static const QString SizeStr;
	static const QString SpacingStr;
	static const QString OriginStr;
	static const QString HeadersizeStr;
	static const QString DataTypeStr;
	static const QString ByteOrderStr;
	iARawFileIO();
	std::shared_ptr<iADataSet> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	void saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
};

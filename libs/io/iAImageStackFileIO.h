// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAio_export.h"

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

// TODO NEWIO: merge with modules/MetaFilters/iAStackReaderFilter ?
class iAio_API iAImageStackFileIO : public iAFileIO, private iAAutoRegistration<iAFileIO, iAImageStackFileIO, iAFileTypeRegistry>
{
public:
	static QString const Name;
	static QString const LoadTypeStr;
	static QString const SingleImageOption;
	static QString const ImageStackOption;
	iAImageStackFileIO();
	std::shared_ptr<iADataSet> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
	bool isDataSetSupported(std::shared_ptr<iADataSet> dataSet, QString const& fileName) const override;
	void saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress) override;
};

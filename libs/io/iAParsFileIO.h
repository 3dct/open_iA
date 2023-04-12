// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAio_export.h"

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"

//! Support for loading .pars files for the file I/O framework of open_iA.
//! Pars file contain reconstruction settings as well as a reference to a raw file with the sinogram.
class iAio_API iAParsFileIO : public iAFileIO, private iAAutoRegistration<iAFileIO, iAParsFileIO, iAFileTypeRegistry>
{
public:
	iAParsFileIO();
	std::shared_ptr<iADataSet> loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;
};

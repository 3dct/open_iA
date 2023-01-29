// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iASampleOperation.h"

#include <QMap>
#include <QString>
#include <QVector>

class iADataSet;
class iALogger;

class iASampleBuiltInFilterOperation : public iASampleOperation
{
	Q_OBJECT
public:
	iASampleBuiltInFilterOperation(
		QString const& filterName,
		bool compressOutput,
		bool overwriteOutput,
		QVariantMap parameters,
		std::vector<std::shared_ptr<iADataSet>> input,
		QString const& outputFileName,
		iALogger * logger);
	QString output() const override;
private:
	void performWork() override;

	QString m_filterName;
	bool m_compressOutput, m_overwriteOutput;
	QVariantMap m_parameters;
	std::vector<std::shared_ptr<iADataSet>> m_input;
	QString m_outputFileName;
	iALogger * m_logger;
};

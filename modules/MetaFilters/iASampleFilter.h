// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAAttributes.h>
#include <iAAutoRegistration.h>
#include <iAFilter.h>
#include <iAFilterRegistry.h>
#include <iAFilterRunnerGUI.h>

class iADataSet;
class iAImageSampler;

class iASampleFilter : public iAFilter, private iAAutoRegistration<iAFilter, iASampleFilter, iAFilterRegistry>
{
public:
	iASampleFilter();
	void setParameters(std::map<size_t, std::shared_ptr<iADataSet>> input, QSharedPointer<iAAttributes> parameterRanges,
		QSharedPointer<iAAttributes> parameterSpecs, QString const& parameterRangeFile, QString const& parameterSetFile,
		QString const& derivedOutFile, int samplingID, std::vector<int> numOfSamplesPerParameter);
	void abort() override;
private:
	void performWork(QVariantMap const& parameters) override;
	bool checkParameters(QVariantMap const& parameters) override;
	std::map<size_t, std::shared_ptr<iADataSet>> m_input;
	QSharedPointer<iAAttributes> m_parameterRanges, m_parameterSpecs;
	QString m_parameterRangeFile,
		m_parameterSetFile,
		m_derivedOutFile;
	int m_samplingID;
	std::vector<int> m_numOfSamplesPerParameter;
	iAImageSampler * m_sampler;
};

class iASampleFilterRunnerGUI : public iAFilterRunnerGUI
{
public:
	static std::shared_ptr<iAFilterRunnerGUI> create();
	bool askForParameters(std::shared_ptr<iAFilter> filter, QVariantMap& paramValues,
		iAMdiChild* sourceMdi, iAMainWindow* mainWnd, bool askForAdditionalInput) override;
};

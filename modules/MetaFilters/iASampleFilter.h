/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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

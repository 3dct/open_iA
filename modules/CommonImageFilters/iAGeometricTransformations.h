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

#include <iAFilterDefault.h>

IAFILTER_DEFAULT_CLASS(iACopy)

class iAExtractComponent : public iAFilter, private iAAutoRegistration<iAFilter, iAExtractComponent, iAFilterRegistry>
{
public:
	iAExtractComponent();
	void adaptParametersToInput(QVariantMap& params, std::vector<std::shared_ptr<iADataSet>> const& dataSets) override;
private:
	void performWork(QVariantMap const& parameters) override;
};

class iASimpleResampleFilter : public iAFilter, private iAAutoRegistration<iAFilter, iASimpleResampleFilter, iAFilterRegistry>
{
public:
	iASimpleResampleFilter();
	void adaptParametersToInput(QVariantMap& params, std::vector<std::shared_ptr<iADataSet>> const& dataSets) override;
private:
	void performWork(QVariantMap const& parameters) override;
};

class iAResampleFilter : public iAFilter, private iAAutoRegistration<iAFilter, iAResampleFilter, iAFilterRegistry>
{
public:
	iAResampleFilter();
	void adaptParametersToInput(QVariantMap& params, std::vector<std::shared_ptr<iADataSet>> const& dataSets) override;
private:
	void performWork(QVariantMap const& parameters) override;
};

class iAExtractImageFilter : public iAFilter, private iAAutoRegistration<iAFilter, iAExtractImageFilter, iAFilterRegistry>
{
public:
	iAExtractImageFilter();
	void adaptParametersToInput(QVariantMap& params, std::vector<std::shared_ptr<iADataSet>> const& dataSets) override;
private:
	void performWork(QVariantMap const& parameters) override;
};

IAFILTER_DEFAULT_CLASS(iAPadImageFilter);

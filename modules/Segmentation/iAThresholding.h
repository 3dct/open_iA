/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iAAlgorithm.h"

#include "iAFilter.h"

class iABinaryThreshold : public iAFilter
{
public:
	static QSharedPointer<iABinaryThreshold> Create();
	void Run(QMap<QString, QVariant> parameters) override;
private:
	iABinaryThreshold();
};

class iARatsThreshold : public iAFilter
{
public:
	static QSharedPointer<iARatsThreshold> Create();
	void Run(QMap<QString, QVariant> parameters) override;
private:
	iARatsThreshold();
};

class iAOtsuThreshold : public iAFilter
{
public:
	static QSharedPointer<iAOtsuThreshold> Create();
	void Run(QMap<QString, QVariant> parameters) override;
private:
	iAOtsuThreshold();
};

class iAAdaptiveOtsuThreshold : public iAFilter
{
public:
	static QSharedPointer<iAAdaptiveOtsuThreshold> Create();
	void Run(QMap<QString, QVariant> parameters) override;
private:
	iAAdaptiveOtsuThreshold();
};

class iAOtsuMultipleThreshold : public iAFilter
{
public:
	static QSharedPointer<iAOtsuMultipleThreshold> Create();
	void Run(QMap<QString, QVariant> parameters) override;
private:
	iAOtsuMultipleThreshold();
};

/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <iAFilter.h>

#include <itkConfigure.h>    // for ITK_VERSION_MAJOR

typedef iAAttributeDescriptor ParamDesc;

class iAFCMFilter : public iAFilter
{
public:
	static QSharedPointer<iAFCMFilter> create();
	bool checkParameters(QMap<QString, QVariant> & parameters) override;
	void performWork(QMap<QString, QVariant> const & parameters) override;
private:
	iAFCMFilter();
};

class iAKFCMFilter : public iAFilter
{
public:
	static QSharedPointer<iAKFCMFilter> create();
	bool checkParameters(QMap<QString, QVariant> & parameters) override;
	void performWork(QMap<QString, QVariant> const & parameters) override;
private:
	iAKFCMFilter();
};

#if ITK_VERSION_MAJOR < 5
class iAMSKFCMFilter : public iAFilter
{
public:
	static QSharedPointer<iAMSKFCMFilter> create();
	bool checkParameters(QMap<QString, QVariant> & parameters) override;
	void performWork(QMap<QString, QVariant> const & parameters) override;
private:
	iAMSKFCMFilter();
};
#endif

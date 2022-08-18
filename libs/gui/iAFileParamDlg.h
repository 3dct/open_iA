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

#include <QMap>
#include <QString>
#include <QVariant>

#include <memory>

class QWidget;

//! provides a dialog for reading parameters, currently for a given file I/O type
//! ToDo: check whether it makes sense to use this or something similar for similar purposes,
//! e.g. for asking for filter parameters?
class iAFileParamDlg
{
public:
	virtual ~iAFileParamDlg();
	//! default implementation for asking file parameters
	virtual bool askForParameters(QWidget* parent, iAAttributes const& parameters, QMap<QString, QVariant>& values, QString const & fileName) const;

	//! factory method, creating a parameter dialog for the given file I/O name:
	static iAFileParamDlg* get(QString const & ioName);

	//! register a dialog for a given I/O name:
	static void add(QString const& ioName, std::shared_ptr<iAFileParamDlg> dlg);

	//! set up a few custom file parameter dialogs
	static void setupDefaultFileParamDlgs();

private:
	static QMap<QString, std::shared_ptr<iAFileParamDlg>> m_dialogs;
};

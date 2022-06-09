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

#include "iAguibase_export.h"

#include "iAParameterDlg.h"

#include <QObject>

struct iARawFileParameters;

class QLabel;

class iAguibase_API iARawFileParamDlg : public QObject
{
	Q_OBJECT

public:
	//! Constructor.
	//! @param fileName         File name of the RAW file.
	//! @param parent           The parent widget.
	//! @param title            The window title.
	//! @param additionalParams List of additional input parameters (@see iAParamDlg).
	//! @param [out] rawFileParams The parameters of the raw file that were set by the user.
	//! @param brightTheme      whether the program currently uses a dark theme (affects green/red background in file size check)
	iARawFileParamDlg(QString const& fileName, QWidget* parent, QString const& title,
		iAParameterDlg::ParamListT const& additionalParams, iARawFileParameters& rawFileParams, bool brightTheme);
	~iARawFileParamDlg();
	//! Checks whether or not the user has accepted the input dialog.
	//! @return true if the user accepted (i.e. clicked "OK"), false if he cancelled.
	bool accepted() const;
	//! access to the values entered by user.
	QMap<QString, QVariant> parameterValues() const;  // make const &, cache

private:
	qint64 m_fileSize;
	QLabel * m_proposedSizeLabel;
	iAParameterDlg* m_inputDlg;
	bool m_accepted;
	bool m_brightTheme;
private slots:
	//! update labels indicating whether current parameters fit the actual file size
	void checkFileSize();
	//! guess file parameters from file name
	void guessParameters(QString fileName);
};

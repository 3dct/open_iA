/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "dlg_commoninput.h"

class dlg_openfile_sizecheck : public dlg_commoninput
{
	Q_OBJECT

public:
	//! constructor
	//! @param isVolumeStack   whether we are opening a volume stack (true) or a single file (false).
	//! @param fileName        File name of the RAW file.
	//! @param [in,out]	parent The parent widget.
	//! @param title           The window title.
	//! @param labels          List of input parameter labels (@see dlg_commoninput).
	//! @param values          List of input parameter values (@see dlg_commoninput).
	//! @param text            The description text shown in the top of the dialog (@see dlg_commoninput).
	dlg_openfile_sizecheck (bool isVolumeStack, QString const & fileName, QWidget *parent, QString const & title,
		QStringList const & labels,	QList<QVariant> const & values, QTextDocument *text = nullptr);

private:
	qint64 m_fileSize;
	QLabel * m_actualSizeLabel;
	QLabel * m_proposedSizeLabel;
	int m_extentXIdx, m_extentYIdx, m_extentZIdx, m_voxelSizeIdx;

private slots:
	//! update labels indicating whether current parameters fit the actual file size
	void checkFileSize();
};

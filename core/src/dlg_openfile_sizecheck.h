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

#include "dlg_commoninput.h"

class dlg_openfile_sizecheck : public dlg_commoninput
{
	Q_OBJECT

public:
	/**
	* constructor function for the class.
	*
	* \param [in,out]	parent	If non-null, the parent.
	* \param	winTitle		The window title.
	* \param	inList			List of ins.
	* \param	inPara			The in para.
	* \param	fileName		File name of the RAW file.
	* \param	extentIndex1	Index of the line edit with  X extent.
	* \param	extentIndex2	Index of the line edit with  Y extent.
	* \param	extentIndex3	Index of the line edit with  Z extent.
	* \param	datatypeIndex	Index of the check box with  datatype stored in file.
	* \param	modal			true to modal.
	*/
	dlg_openfile_sizecheck (bool isVolumeStack, QWidget *parent, QString winTitel, QStringList inList, QList<QVariant> inPara, QTextDocument *text, QString fileName,
		int extentIndex1 = 0, int extentIndex2 = 1, int extentIndex3 = 2, int datatypeIndex = 10,
		bool modal = true);
private:
	qint64 fileSize;
	QLabel *actualSizeLabel;
	QLabel *proposedSizeLabel;
	QString tStr;
	bool isVolumeStack;

private slots:

/**
* \brief	Check if parameters fit actual file size.
*/
	void CheckFileSize();
};

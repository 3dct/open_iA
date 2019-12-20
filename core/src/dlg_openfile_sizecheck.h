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

#include <QObject>

class dlg_commoninput;
struct iARawFileParameters;

class QLabel;

class dlg_openfile_sizecheck: public QObject
{
	Q_OBJECT

public:
	//! Constructor.
	//! @param fileName         File name of the RAW file.
	//! @param parent           The parent widget.
	//! @param title            The window title.
	//! @param additionalLabels List of additional input parameter labels (@see dlg_commoninput).
	//! @param additionalValues List of additional input parameter values (@see dlg_commoninput).
	//! @param [out] rawFileParams The parameters of the raw file that were set by the user.
	dlg_openfile_sizecheck (QString const & fileName, QWidget *parent, QString const & title,
		QStringList const & additionalLabels, QList<QVariant> const & additionalValues, iARawFileParameters & rawFileParams);
	~dlg_openfile_sizecheck();
	//! Checks whether or not the user has accepted the input dialog.
	//! @return true if the user accepted (i.e. clicked "OK"), false if he cancelled.
	bool accepted() const;
	//! The number of fixed parameters which the dialog creates itself.
	//! Use this to determine where the first user-specified parameters
	//! (additionalLabels/additionalValues parameter in constructor) started.
	//! @return the number of fixed parameters used by the dialog itself;
	//!     this is also the index of the first user-specified parameter.
	int fixedParams() const;
	//! access to the underlying common input dialog, mainly for accessing values entered by user.
	//! @return the common input dialog.
	dlg_commoninput const * inputDlg() const;
private:
	qint64 m_fileSize;
	QLabel * m_actualSizeLabel;
	QLabel * m_proposedSizeLabel;
	int m_sizeXIdx, m_sizeYIdx, m_sizeZIdx, m_voxelSizeIdx, m_headerSizeIdx;
	double * dlg;
	dlg_commoninput* m_inputDlg;
	bool m_accepted;
	int m_fixedParams;
private slots:
	//! update labels indicating whether current parameters fit the actual file size
	void checkFileSize();
};

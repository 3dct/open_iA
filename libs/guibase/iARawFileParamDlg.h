// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include "iAParameterDlg.h"

#include <QObject>

class QLabel;

//! Shows a dialog specialized for reading parameters of a raw file.
//! Adds a red/green bar depending on whether the currently specified dimensions
//! match the size of the raw file.
class iAguibase_API iARawFileParamDlg : public QObject
{
	Q_OBJECT

public:
	//! Constructor.
	//! @param fileName         File name of the RAW file.
	//! @param parent           The parent widget.
	//! @param title            The window title.
	//! @param additionalParams List of additional input parameters (@see iAParamDlg).
	//! @param [out] paramValues The parameters of the raw file that were set by the user.
	//! @param brightTheme      whether the program currently uses a dark theme (affects green/red background in file size check)
	iARawFileParamDlg(QString const& fileName, QWidget* parent, QString const& title,
		iAAttributes const& additionalParams, QVariantMap & paramValues, bool brightTheme);
	~iARawFileParamDlg();
	//! Checks whether or not the user has accepted the input dialog.
	//! @return true if the user accepted (i.e. clicked "OK"), false if he cancelled.
	bool accepted() const;
	//! access to the values entered by user.
	QVariantMap parameterValues() const;  // make const &, cache

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

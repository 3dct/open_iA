// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>

#include <iARawFileParameters.h>

class iAChartWithFunctionsWidget;
class iAParameterDlg;
class iARawFilePreviewSlicer;
struct iASliceMergedValues;
class iATransferFunctionOwner;

class QLabel;

//! Shows a dialog specialized for reading parameters of a raw file.
//! Adds a red/green bar depending on whether the currently specified dimensions
//! match the size of the raw file.
class iARawFileParamDlg : public QObject
{
	Q_OBJECT

public:
	//! Constructor.
	//! @param fileName         File name of the RAW file.
	//! @param parent           The parent widget.
	//! @param title            The window title.
	//! @param [out] paramValues The parameters of the raw file that were set by the user.
	//! @param brightTheme      whether the program currently uses a dark theme (affects green/red background in file size check)
	iARawFileParamDlg(QString const& fileName, QWidget* parent, QString const& title,
		QVariantMap & paramValues, bool brightTheme);
	~iARawFileParamDlg();
	//! Checks whether or not the user has accepted the input dialog.
	//! @return true if the user accepted (i.e. clicked "OK"), false if he cancelled.
	bool accepted() const;
	//! access to the values entered by user.
	QVariantMap parameterValues() const;  // make const &, cache

private:
	qint64 m_fileSize;
	QLabel * m_proposedSizeLabel = nullptr;
	QWidget* m_previewContainer = nullptr;
	iAChartWithFunctionsWidget* m_chart;
	iAParameterDlg* m_inputDlg = nullptr;
	bool m_accepted = false;
	bool m_brightTheme;
	bool m_previewShown = false;
	int m_previewWidth = 0;
	QString m_fileName;
	std::vector<std::shared_ptr<iARawFilePreviewSlicer>> m_slicer;
	std::optional<iARawFileParameters> m_params;
	std::unique_ptr<iASliceMergedValues> m_dataValues;
	std::unique_ptr<iATransferFunctionOwner> m_tf;

private slots:
	//! update labels indicating whether current parameters fit the actual file size
	void checkFileSize();
	//! guess file parameters from file name
	void guessParameters(QString fileName);
	//! toggle preview visibility
	void togglePreview();
	//! update the preview if visible
	void updatePreview();
};

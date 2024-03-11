// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QSpinBox;
class QLabel;

class dlg_blobVisualization : public QDialog
{
	Q_OBJECT
public:
	dlg_blobVisualization (QWidget* parent = 0);
	~dlg_blobVisualization (void);

	bool	GetOverlapping (void) const;
	double	GetRange (void) const ;
	double	GetOverlapThreshold (void) const;
	double	GetBlurVariance (void) const;
	int		GetResolution(void) const;

private:
	QDialogButtonBox*	m_buttons;
	QLabel*	m_labelOverlapping;
	QLabel*	m_labelRange;
	QLabel*	m_labelOverlapThreshold;
	QLabel* m_labelVariance;
	QLabel* m_labelResolution;
	QDoubleSpinBox*	m_spinBoxRange;
	QDoubleSpinBox*	m_gaussianBlurVariance;
	QDoubleSpinBox*	m_spinBoxOverlapThreshold;
	QSpinBox*	m_spinBoxResolution;
	QCheckBox*	m_chekBoxOverlapping;
};

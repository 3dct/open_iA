// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_blobVisualization.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QLabel>

#define DEFAULT_OVERLAP_THRESHOLD 0.01
#define DEFAULT_RANGE 0.025
#define DEFAULT_VARIANCE 150.0
#define DEFAULT_RESOLUTION 50
#define DEFAULT_OVERLAPPING true

dlg_blobVisualization::dlg_blobVisualization (QWidget* parent) : QDialog (parent)
{
	// initialize layout
	QVBoxLayout* mainLayout = new QVBoxLayout (this);
	m_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	m_labelOverlapping = new QLabel ("Smart overlapping:");
	m_labelRange = new QLabel ("Range:");
	m_labelOverlapThreshold = new QLabel ("Overlap threshold:");
	m_labelVariance = new QLabel ("Gaussian blur variance:");
	m_labelResolution = new QLabel ("Blob resolution:");

	m_spinBoxRange = new QDoubleSpinBox (this);
	m_spinBoxRange->setDecimals (6);
	m_spinBoxRange->setSingleStep (0.001);
	m_spinBoxRange->setMinimum(0.000001);
	m_spinBoxRange->setMaximum(99999999.0);
	m_spinBoxRange->setValue (DEFAULT_RANGE);

	m_gaussianBlurVariance = new QDoubleSpinBox (this);
	m_gaussianBlurVariance->setDecimals (6);
	m_gaussianBlurVariance->setSingleStep (0.001);
	m_gaussianBlurVariance->setMinimum(0.000001);
	m_gaussianBlurVariance->setMaximum(99999999.0);
	m_gaussianBlurVariance->setValue (DEFAULT_VARIANCE);

	m_spinBoxResolution = new QSpinBox (this);
	m_spinBoxResolution->setMinimum(1);
	m_spinBoxResolution->setMaximum(2048);
	m_spinBoxResolution->setValue (DEFAULT_RESOLUTION);

	m_spinBoxOverlapThreshold = new QDoubleSpinBox (this);
	m_spinBoxOverlapThreshold->setDecimals (3);
	m_spinBoxOverlapThreshold->setSingleStep (0.001);
	m_spinBoxOverlapThreshold->setValue (DEFAULT_OVERLAP_THRESHOLD);

	m_chekBoxOverlapping = new QCheckBox (this);
	m_chekBoxOverlapping->setChecked (DEFAULT_OVERLAPPING);

	QWidget* page = new QWidget (this);
	QGridLayout* layout = new QGridLayout (page);
	int indx = 0;
	layout->addWidget (m_labelResolution, indx, 0);
	layout->addWidget (m_spinBoxResolution, indx, 1);

	layout->addWidget (m_labelRange, ++indx, 0);
	layout->addWidget (m_spinBoxRange, indx, 1);

	layout->addWidget (m_labelVariance, ++indx, 0);
	layout->addWidget (m_gaussianBlurVariance, indx, 1);

	layout->addWidget (m_labelOverlapping, ++indx, 0);
	layout->addWidget (m_chekBoxOverlapping, indx, 1);

	layout->addWidget (m_labelOverlapThreshold, ++indx, 0);
	layout->addWidget (m_spinBoxOverlapThreshold, indx, 1);

	mainLayout->addWidget (page);
	mainLayout->addWidget (m_buttons);

	// setup connections
	connect (m_buttons, &QDialogButtonBox::accepted, this, &dlg_blobVisualization::accept);
	connect (m_buttons, &QDialogButtonBox::rejected, this, &dlg_blobVisualization::reject);
}

dlg_blobVisualization::~dlg_blobVisualization (void)
{

}

bool dlg_blobVisualization::GetOverlapping( void ) const
{
	return m_chekBoxOverlapping->isChecked();
}

double dlg_blobVisualization::GetRange( void ) const
{
	return m_spinBoxRange->value();
}

double dlg_blobVisualization::GetOverlapThreshold( void ) const
{
	return m_spinBoxOverlapThreshold->value();
}

double dlg_blobVisualization::GetBlurVariance( void ) const
{
	return m_gaussianBlurVariance->value();
}

int dlg_blobVisualization::GetResolution( void ) const
{
	return m_spinBoxResolution->value();
}

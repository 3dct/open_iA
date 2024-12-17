// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASlicerContainer.h"

#include "iASlicerImpl.h"

const int iASlicerContainer::BorderWidth = 3;

iASlicerContainer::iASlicerContainer(iASlicerImpl* slicer):
	m_slicer(slicer),
	m_titleVisible(true)
{
	setupUi(this);
	QString slicePlaneName = slicerModeString(slicer->mode());
	QString sliceAxisString = axisName(slicer->globalAxis(iAAxisIndex::Z));
	QColor color(axisColor(slicer->mode()));
	lbTitle->setText(slicePlaneName);
	lbSlice->setText(QString("Slice # %1").arg(sliceAxisString));
	lbRotation->setText(QString("Rot %1").arg(sliceAxisString));
	sliceContainerLayout->addWidget(slicer);
	sliceContainer->setStyleSheet(QString("#sliceContainer { border: %1px solid rgb(%2, %3, %4) } ")
		.arg(BorderWidth).arg(color.red()).arg(color.green()).arg(color.blue()));
	sbSlice->setRange(-8192, 8192);
	sbSlabThickness->hide();
	lbSlabThickness->hide();
	cbSlabCompositeMode->hide();

	connect(pbSaveScreen, &QToolButton::clicked, slicer, &iASlicer::saveAsImage);
	connect(pbSaveStack, &QToolButton::clicked, slicer, &iASlicerImpl::saveImageStack);
	connect(pbSaveMovie, &QToolButton::clicked, slicer, &iASlicer::saveMovie);
	connect(pbToggleInteraction, &QToolButton::clicked, slicer, [slicer]() { slicer->enableInteractor(!slicer->isInteractorEnabled()); });
	connect(dsbRotation, QOverload<double>::of(&QDoubleSpinBox::valueChanged), slicer, &iASlicer::rotateSlice);
	connect(sbSlice, QOverload<int>::of(&QSpinBox::valueChanged), this, &iASlicerContainer::setSliceSpinBox);
	connect(verticalScrollBar, &QSlider::valueChanged, this, &iASlicerContainer::setSliceScrollBar);
	connect(cbSlabMode, &QCheckBox::toggled, this, &iASlicerContainer::setSlabMode);
	connect(sbSlabThickness, QOverload<int>::of(&QSpinBox::valueChanged), this, &iASlicerContainer::updateSlabThickness);
	connect(cbSlabCompositeMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iASlicerContainer::updateSlabCompositeMode);
	connect(m_slicer, &iASlicerImpl::sliceRangeChanged, this, &iASlicerContainer::updateSliceControls);
	connect(m_slicer, &iASlicerImpl::sliceNumberChanged, this, [this](int mode, int s)
	{
		Q_UNUSED(mode);
		if (sbSlice->value() != s)
		{
			QSignalBlocker block(sbSlice);
			sbSlice->setValue(s);
		}
		if (verticalScrollBar->value() != s)
		{
			QSignalBlocker block(verticalScrollBar);
			verticalScrollBar->setValue(s);
		}
	});
}

void iASlicerContainer::showBorder(bool show)
{
	int borderWidth = show ? BorderWidth : 0;
	sliceContainerLayout->setContentsMargins(borderWidth, borderWidth, borderWidth, borderWidth);
}

void iASlicerContainer::showTitle(bool show)
{
	m_titleVisible = show;
	slicerControls->setVisible(show);
}

bool iASlicerContainer::isTitleShown() const
{
	return m_titleVisible; // workaround for slicerControls apparently not initialized yet when this is called first // slicerControls->isVisible();
}

void iASlicerContainer::setSliceSpinBox(int s)
{
	m_slicer->setSliceNumber(s);
}

void iASlicerContainer::setSliceScrollBar(int s)
{
	m_slicer->setSliceNumber(s);
}

void iASlicerContainer::setSlabMode(bool slabMode)
{
	lbSlabThickness->setVisible(slabMode);
	sbSlabThickness->setVisible(slabMode);
	cbSlabCompositeMode->setVisible(slabMode);
	updateSlabThickness(slabMode == true ? sbSlabThickness->value() : 0);
}

void iASlicerContainer::updateSlabThickness(int thickness)
{
	m_slicer->setSlabThickness(thickness);
}

void iASlicerContainer::updateSlabCompositeMode(int mode)
{
	m_slicer->setSlabCompositeMode(mode);
}

void iASlicerContainer::updateSliceControls(int minIdx, int maxIdx, int val)
{
	QSignalBlocker spinboxBlock(sbSlice);
	QSignalBlocker scrollBlock(verticalScrollBar);
	sbSlice->setRange(minIdx, maxIdx);
	sbSlice->setValue(val);
	verticalScrollBar->setRange(minIdx, maxIdx);
	verticalScrollBar->setValue(val);
}

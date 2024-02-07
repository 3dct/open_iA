// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAMultimodalWidget.h"

class iAInterpolationSliderWidget;
class iAMdiChild;

class QLabel;

class iABimodalWidget : public iAMultimodalWidget
{
	Q_OBJECT

public:
	iABimodalWidget(iAMdiChild *mdiChild);
	void initialize();

private:
	iAInterpolationSliderWidget *m_slider;
	QVector<QLabel*> m_labels;
	void dataSetChanged(size_t dataSetIdx) override;

private slots:
	void dataSetsLoaded_beforeUpdateSlot();
	void tChanged(double t);

};

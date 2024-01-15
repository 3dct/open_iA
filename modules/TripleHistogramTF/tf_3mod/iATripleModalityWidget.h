// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAMultimodalWidget.h"

#include "iAHistogramAbstract.h"
#include "iABCoord.h"

class iABarycentricTriangleWidget;
class iABarycentricContextRenderer;

class iAMdiChild;

class QComboBox;
class QLabel;
class QSlider;
class QSpinBox;

class iATripleModalityWidget : public iAMultimodalWidget
{
	Q_OBJECT

public:
	iATripleModalityWidget(iAMdiChild *mdiChild);
	~iATripleModalityWidget();

	iAHistogramAbstractType getLayoutTypeAt(int comboBoxIndex);
	void setHistogramAbstractType(iAHistogramAbstractType type);

	iABarycentricTriangleWidget* w_triangle()
	{
		return m_triangleWidget;
	}

	QComboBox* w_layoutComboBox()
	{
		return m_layoutComboBox;
	}

private slots:
	void layoutComboBoxIndexChanged(int newIndex);
	void triangleWeightChanged(iABCoord newWeights);
	void weightsChangedSlot(iABCoord newWeights);
	void dataSetsLoaded_beforeUpdateSlot();

private:
	void dataSetChanged(size_t dataSetIdx) override;
	QComboBox *m_layoutComboBox;
	void setLayoutTypePrivate(iAHistogramAbstractType type);

	iABarycentricTriangleWidget *m_triangleWidget;
	iABarycentricContextRenderer *m_triangleRenderer;

	iAHistogramAbstract *m_histogramAbstract = nullptr;
	iAHistogramAbstractType m_histogramAbstractType;

	void updateDataSets();
};

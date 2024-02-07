// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAChartWithFunctionsWidget.h>

#include <QMap>

class iAAccumulatedXRFData;
struct iACharacteristicEnergy;
class iATransferFunctionPtrs;
class iASpectrumFilterListener;

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

class QRubberBand;

class iAEnergySpectrumWidget: public iAChartWithFunctionsWidget
{
public:
	iAEnergySpectrumWidget(QWidget *parent,
		std::shared_ptr<iAAccumulatedXRFData> data,
		vtkPiecewiseFunction* oTF,
		vtkColorTransferFunction* cTF,
		iASpectrumFilterListener* filterListener,
		QString const & xLabel);
	void AddElementLines(iACharacteristicEnergy* element, QColor const & color);
	void RemoveElementLines(iACharacteristicEnergy* element);
protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void drawAfterPlots(QPainter& painter) override;
private:
	void NotifySelectionUpdateListener();

	std::shared_ptr<iAAccumulatedXRFData> m_data;

	QPoint selectionOrigin;
	QRubberBand* selectionRubberBand;
	QVector<QRect> selectionRects;
	iASpectrumFilterListener* filterListener;
	QMap<iACharacteristicEnergy*, QColor> m_elementEnergies;
	std::shared_ptr<iATransferFunctionPtrs> m_tf;
};

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

#include <charts/iAChartWithFunctionsWidget.h>

#include <QMap>

class iAAccumulatedXRFData;
struct iACharacteristicEnergy;
class iAPlotData;
class iASpectrumFilterListener;

class QRubberBand;

class iAEnergySpectrumWidget: public iAChartWithFunctionsWidget
{
public:
	iAEnergySpectrumWidget(QWidget *parent, MdiChild *mdiChild,
		QSharedPointer<iAAccumulatedXRFData> data,
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

	QSharedPointer<iAAccumulatedXRFData>	m_data;

	QPoint selectionOrigin;
	QRubberBand* selectionRubberBand;
	QVector<QRect> selectionRects;
	iASpectrumFilterListener* filterListener;
	QMap<iACharacteristicEnergy*, QColor> m_elementEnergies;
};

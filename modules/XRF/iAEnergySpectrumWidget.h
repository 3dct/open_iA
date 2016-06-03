/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IAENERGYSPECTRUMWIDGET_H
#define IAENERGYSPECTRUMWIDGET_H

#include "iADiagramFctWidget.h"

#include <QRubberBand>
#include <QMap>

class iAAbstractDiagramData;
class iAAccumulatedXRFData;
class iASpectrumFilterListener;
struct iACharacteristicEnergy;

class iAEnergySpectrumWidget: public iADiagramFctWidget
{
public:
	iAEnergySpectrumWidget(QWidget *parent, MdiChild *mdiChild,
		QSharedPointer<iAAccumulatedXRFData> data,
		vtkPiecewiseFunction* oTF,
		vtkColorTransferFunction* cTF,
		iASpectrumFilterListener* filterListener,
		QString const & xLabel);
	virtual QSharedPointer<iAAbstractDiagramRangedData> GetData();
	virtual QSharedPointer<iAAbstractDiagramRangedData> const GetData() const;
	void AddElementLines(iACharacteristicEnergy* element, QColor const & color);
	void RemoveElementLines(iACharacteristicEnergy* element);
protected:
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void drawDatasets(QPainter& painter);
private:
	void NotifySelectionUpdateListener();

	QSharedPointer<iAAccumulatedXRFData>	m_data;

	QSharedPointer<iAAbstractDrawableFunction> CreatePrimaryDrawer();

	QPoint selectionOrigin;
	QRubberBand* selectionRubberBand;
	QVector<QRect> selectionRects;
	iASpectrumFilterListener* filterListener;
	QMap<iACharacteristicEnergy*, QColor> m_elementEnergies;
};

#endif /* IAENERGYSPECTRUMWIDGET_H */

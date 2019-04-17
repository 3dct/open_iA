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
#include "iAEnergySpectrumWidget.h"

#include "iAAccumulatedXRFData.h"
#include "iACharacteristicEnergy.h"
#include "iASpectrumFilter.h"

#include <charts/iAPlotTypes.h>
#include <iAMapper.h>

#include <QFontMetrics>
#include <QMap>
#include <QMouseEvent>
#include <QPainter>
#include <QRubberBand>

const char * EnergyLineNames[9] =
{
	"Kα1",
	"Kα2",
	"Kß1",
	"Lα1",
	"Lα2",
	"Lß1",
	"Lß2",
	"Lγ1",
	"Mα1"
};

iAEnergySpectrumWidget::iAEnergySpectrumWidget(QWidget *parent, MdiChild *mdiChild,
		QSharedPointer<iAAccumulatedXRFData> data,
		vtkPiecewiseFunction* oTF,
		vtkColorTransferFunction* cTF,
		iASpectrumFilterListener* filterListener,
		QString const & xLabel)
	: iADiagramFctWidget(parent, mdiChild, xLabel, "Count"),
	m_data(data),
	selectionRubberBand(new QRubberBand(QRubberBand::Rectangle, this)),
	filterListener(filterListener)
{
	setTransferFunctions(cTF, oTF);
	addPlot(QSharedPointer<iAPlot>(new iAStepFunctionPlot(m_data, QColor(70, 70, 70, 255))));
	selectionRubberBand->hide();
	setAllowTrfReset(false);
	setEnableAdditionalFunctions(false);
	setMinimumHeight(150);
}

void iAEnergySpectrumWidget::mousePressEvent(QMouseEvent *event)
{
	if ( (event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier )
	{
		selectionOrigin = event->pos();
		selectionRubberBand->setGeometry(QRect(selectionOrigin, QSize()));
		selectionRubberBand->show();
	}
	else
	{	
		if(!selectionRects.isEmpty())
		{
			selectionRects.clear();
			update();
		}
		else if( event->modifiers() || event->y() > geometry().height() - bottomMargin() )
		{
			QMouseEvent eventCopy(event->type(),
				QPoint(event->x(), geometry().height() - bottomMargin()),
				event->globalPos(),
				event->button(),
				event->buttons(),
				event->modifiers()
			);
			iADiagramFctWidget::mousePressEvent(&eventCopy); //if any modifiers, or click is on the bottom panel: process in base class
		}
	}
}

void iAEnergySpectrumWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QMouseEvent eventCopy(event->type(),
		QPoint(event->x(), geometry().height() - bottomMargin()),
		event->globalPos(),
		event->button(),
		event->buttons(),
		event->modifiers()
	);
	iADiagramFctWidget::mouseReleaseEvent(&eventCopy);
	if (selectionRubberBand->isVisible())
	{
		selectionRubberBand->hide();
		QRect diagramRect;
		QRect selectionRect(selectionRubberBand->geometry());     // height-y because we are drawing reversed from actual y direction
		diagramRect.setTop(    yMapper().dstToSrc(activeHeight() - selectionRect.bottom()) );
		diagramRect.setBottom( yMapper().dstToSrc(activeHeight() - selectionRect.top()   ) );
		diagramRect.setLeft(   screenX2DataBin(selectionRect.left()  ) );
		diagramRect.setRight(  screenX2DataBin(selectionRect.right() ) );
		diagramRect = diagramRect.normalized();

		if (diagramRect.top() < 0)
		{
			diagramRect.setTop(0);
		}

		if (diagramRect.bottom() > yBounds()[1])
		{
			diagramRect.setBottom(yBounds()[1]);
		}

		// .width() and .height() counter-intuitively report 1 if x1=x2/y1=y2
		if (diagramRect.width() > 1 && diagramRect.height() > 1)
		{
			selectionRects.push_back(diagramRect);
		}
	}
	// notify any listener of changed selection
	NotifySelectionUpdateListener();
}

void iAEnergySpectrumWidget::mouseMoveEvent(QMouseEvent *event)
{
	QMouseEvent eventCopy(event->type(),
		QPoint(event->x(), geometry().height() - bottomMargin()),
		event->globalPos(),
		event->button(),
		event->buttons(),
		event->modifiers()
	);
	iADiagramFctWidget::mouseMoveEvent(&eventCopy);
	if (!selectionRubberBand->isVisible())
	{
		return;
	}
	selectionRubberBand->setGeometry(QRect(selectionOrigin, event->pos()).normalized());
}

void iAEnergySpectrumWidget::drawAfterPlots(QPainter& painter)
{
	QPen pen(Qt::red);
	pen.setWidth(2);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	for (int i=0; i<selectionRects.size(); ++i)
	{
		QRect drawRect;
		drawRect.setTop(   yMapper().srcToDst(selectionRects[i].top()));
		drawRect.setBottom(yMapper().srcToDst(selectionRects[i].bottom())-1);
		drawRect.setLeft(  dataBin2ScreenX(selectionRects[i].left()));
		drawRect.setRight( dataBin2ScreenX(selectionRects[i].right())-2);
		drawRect = drawRect.normalized();
		painter.drawRect(drawRect);
	}
	QFontMetrics fm(painter.font());
	QList<iACharacteristicEnergy*> keys = m_elementEnergies.keys();
	for (QList<iACharacteristicEnergy*>::const_iterator it = keys.begin();
		it != keys.end();
		++it)
	{
		iACharacteristicEnergy * element = (*it);
		QColor color = m_elementEnergies[element];
		painter.setPen(color);
		int drawnLines = 0;
		for (int j=0; j<element->energies.size(); ++j)
		{
			double elementkEV = element->energies[j]/1000.0;
			if (elementkEV >= xBounds()[0] &&
				elementkEV <= xBounds()[1])
			{
				QLine line;
				QRect diagram = geometry();
				double pos = m_xMapper->srcToDst(elementkEV);
				line.setP1(QPoint(pos, 0));
				line.setP2(QPoint(pos, diagram.height()-bottomMargin()));
				painter.drawLine(line);
				painter.save();
				painter.scale(1, -1);
				QRect captionBoundingBox(
					static_cast<int>(pos+5),
					- (10 + (fm.height()+2) * (drawnLines+1)*2),
					std::max(fm.width(element->symbol), fm.width(QString::fromUtf8(EnergyLineNames[j]))),
					fm.height()*2
				);
				drawnLines++;
				painter.drawText(
					captionBoundingBox,
					Qt::TextWordWrap,
					element->symbol + "\n" + QString::fromUtf8(EnergyLineNames[j]));
				painter.restore();
			}
		}
	}
}

void iAEnergySpectrumWidget::NotifySelectionUpdateListener()
{
	if (!filterListener)
	{
		return;
	}
	QVector<iASpectrumFilter> filter;
	for (int i=0; i<selectionRects.size(); ++i)
	{
		for (int x=selectionRects[i].left(); x<selectionRects[i].right(); ++x)
		{
			// min = top (because rect is normalized, there top left corner holds the smallest coords)
			// max = bottom
			filter.push_back(iASpectrumFilter(x, selectionRects[i].top(), selectionRects[i].bottom()));
		}
	}
	filterListener->OnSelectionUpdate(filter);
}

void iAEnergySpectrumWidget::AddElementLines(iACharacteristicEnergy* element, QColor const & color)
{
	m_elementEnergies.insert(element, color);
}

void iAEnergySpectrumWidget::RemoveElementLines(iACharacteristicEnergy* element)
{
	m_elementEnergies.remove(element);
}

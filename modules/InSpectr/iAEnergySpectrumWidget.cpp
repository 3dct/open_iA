// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAEnergySpectrumWidget.h"

#include "iAAccumulatedXRFData.h"
#include "iACharacteristicEnergy.h"
#include "iASpectrumFilter.h"

#include <iAPlotTypes.h>
#include <iAMapper.h>
#include <iATransferFunctionPtrs.h>

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

iAEnergySpectrumWidget::iAEnergySpectrumWidget(QWidget *parent,
		std::shared_ptr<iAAccumulatedXRFData> data,
		vtkPiecewiseFunction* oTF,
		vtkColorTransferFunction* cTF,
		iASpectrumFilterListener* filterListener,
		QString const & xLabel)
	: iAChartWithFunctionsWidget(parent, xLabel, "Count"),
	m_data(data),
	selectionRubberBand(new QRubberBand(QRubberBand::Rectangle, this)),
	filterListener(filterListener),
	m_tf(new iATransferFunctionPtrs(cTF, oTF))
{
	setTransferFunction(m_tf.get());
	addPlot(std::make_shared<iAStepFunctionPlot>(m_data, QColor(70, 70, 70, 255)));
	selectionRubberBand->hide();
	setAllowTFReset(false);
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
		else if (event->modifiers() || event->position().y() > geometry().height() - bottomMargin())
		{
			QMouseEvent eventCopy(event->type(),
				QPoint(event->position().x(), geometry().height() - bottomMargin()),
				event->globalPosition().toPoint(),
				event->button(),
				event->buttons(),
				event->modifiers()
			);
			iAChartWithFunctionsWidget::mousePressEvent(&eventCopy); //if any modifiers, or click is on the bottom panel: process in base class
		}
	}
}

void iAEnergySpectrumWidget::mouseReleaseEvent(QMouseEvent *event)
{
	QMouseEvent eventCopy(event->type(),
		QPoint(event->position().x(), geometry().height() - bottomMargin()),
		event->globalPosition().toPoint(),
		event->button(),
		event->buttons(),
		event->modifiers()
	);
	iAChartWithFunctionsWidget::mouseReleaseEvent(&eventCopy);
	if (selectionRubberBand->isVisible())
	{
		// TODO: avoid duplication with iAChartWidget!
		selectionRubberBand->hide();
		QRect diagramRect;
		QRect selectionRect(selectionRubberBand->geometry());     // height-y because we are drawing reversed from actual y direction
		diagramRect.setTop(    yMapper().dstToSrc(chartHeight() - selectionRect.bottom()) );
		diagramRect.setBottom( yMapper().dstToSrc(chartHeight() - selectionRect.top()   ) );
		diagramRect.setLeft(   static_cast<int>(screenX2DataBin(selectionRect.left()  )) );
		diagramRect.setRight(  static_cast<int>(screenX2DataBin(selectionRect.right() )) );
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
		QPoint(event->position().x(), geometry().height() - bottomMargin()),
		event->globalPosition().toPoint(),
		event->button(),
		event->buttons(),
		event->modifiers()
	);
	iAChartWithFunctionsWidget::mouseMoveEvent(&eventCopy);
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
		for (size_t j=0; j<element->energies.size(); ++j)
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
					std::max(fm.horizontalAdvance(element->symbol), fm.horizontalAdvance(QString::fromUtf8(EnergyLineNames[j]))),
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
	iAChartWithFunctionsWidget::drawAfterPlots(painter);
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

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

#include "iARangeSliderDiagramData.h"

#include <charts/iADiagramFctWidget.h>
#include <charts/iAPlotTypes.h>

#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

#include <QMouseEvent>
#include <QRubberBand>
#include <QToolTip>
#include <QTableWidget>


class iAFilteringDiagramData : public iAPlotData
{
public:
	iAFilteringDiagramData( QSharedPointer<iAPlotData> other, int min, int max )
		: m_data( new double[other->numBin()] ),
		m_size( other->numBin() ),
		m_other(other)
	{
		for ( int i = 0; i < other->numBin(); ++i )
		{
			m_data[i] = ( i >= min && i <= max ) ? other->rawData()[i] : 0;
		}
	}

	DataType const * rawData() const override
	{
		return m_data;
	}

	size_t numBin() const override
	{
		return m_size;
	}
	double spacing() const override
	{
		return m_other->spacing();
	}
	double const * xBounds() const override
	{
		return m_other->xBounds();
	}
	DataType const * yBounds() const override
	{
		return m_other->yBounds();
	}
private:
	DataType* m_data;
	size_t m_size;
	QSharedPointer<iAPlotData> m_other;
};


class iARangeSliderDiagramWidget : public iADiagramFctWidget
{
	Q_OBJECT

public:
	iARangeSliderDiagramWidget( QWidget *parent, MdiChild *mdiChild,
								vtkPiecewiseFunction* oTF,
								vtkColorTransferFunction* cTF,
								QSharedPointer<iARangeSliderDiagramData> data,
								QMap<double, QList<double> > *,
								const QTableWidget * rawTable, 
								QString const & xlabel = "Greyvalue",
								QString const & yLabel = "Frequency" );

	void drawFunctions( QPainter &painter ) override;
	void mouseDoubleClickEvent( QMouseEvent *event ) override;
	void mousePressEvent( QMouseEvent *event ) override;
	void mouseReleaseEvent( QMouseEvent *event ) override;
	void mouseMoveEvent( QMouseEvent *event ) override;
	void contextMenuEvent( QContextMenuEvent *event ) override;
	void updateSelectedDiagrams();
	QList<int> getSelectedRawTableRows();

signals:
	void selected();
	void deselected();
	void selectionRelesedSignal();
	
public slots:
	void selectSlot();
	void deleteSlot();

private:
	QSharedPointer<iARangeSliderDiagramData>			m_data;
	QSharedPointer<iAPlotData>							m_selectedData;
	QSharedPointer<iAStepFunctionPlot>				m_selectionDrawer;
	QList<QSharedPointer<iAStepFunctionPlot> >		m_histogramDrawerList;
	QPoint												m_selectionOrigin;
	QRubberBand*										m_selectionRubberBand;
	QString												m_xLabel;
	QString												m_yLabel;
	QColor												m_selectionColor;
	QList<int>											m_addedHandles;

	int m_firstSelectedBin;
	int m_lastSelectedBin;

	QMap<double, QList<double> >				*m_histogramMap;
	const QTableWidget							*m_rawTable;

	int getBin( QMouseEvent *event );
	void setupSelectionDrawer();
};

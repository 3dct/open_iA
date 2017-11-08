/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iADiagramFctWidget.h"
#include "iAFunctionDrawers.h"
#include "iARangeSliderDiagramData.h"

#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

#include <QMouseEvent>
#include <QRubberBand>
#include <QToolTip>
#include <QTableWidget>

#include "dlg_function.h"
#include "dlg_transfer.h"


class iAFilteringDiagramData : public iAPlotData
{
public:
	iAFilteringDiagramData( QSharedPointer<iAPlotData> other, int min, int max )
		: m_data( new double[other->GetNumBin()] ),
		m_size( other->GetNumBin() ),
		m_other(other)
	{
		for ( int i = 0; i < other->GetNumBin(); ++i )
		{
			m_data[i] = ( i >= min && i <= max ) ? other->GetRawData()[i] : 0;
		}
	}

	DataType const * GetRawData() const override
	{
		return m_data;
	}

	size_t GetNumBin() const override
	{
		return m_size;
	}
	double GetSpacing() const override
	{
		return m_other->GetSpacing();
	}
	double const * XBounds() const override
	{
		return m_other->XBounds();
	}
	DataType const * YBounds() const override
	{
		return m_other->YBounds();
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

	virtual void drawFunctions( QPainter &painter );
	virtual void mouseDoubleClickEvent( QMouseEvent *event );
	virtual void mousePressEvent( QMouseEvent *event );
	virtual void mouseReleaseEvent( QMouseEvent *event );
	virtual void mouseMoveEvent( QMouseEvent *event );
	virtual void contextMenuEvent( QContextMenuEvent *event );
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
	QSharedPointer<iAStepFunctionDrawer>				m_selectionDrawer;
	QList<QSharedPointer<iAStepFunctionDrawer> >		m_histogramDrawerList;
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

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAChartWithFunctionsWidget.h>
#include <iAPlotTypes.h>

#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

#include <QMouseEvent>
#include <QRubberBand>
#include <QTableWidget>

class iAHistogramData;
class iATransferFunction;

// TODO: Merge with GEMSE: iAFilterChart
class iARangeSliderDiagramWidget : public iAChartWithFunctionsWidget
{
	Q_OBJECT

public:
	iARangeSliderDiagramWidget( QWidget *parent,
								vtkPiecewiseFunction* oTF,
								vtkColorTransferFunction* cTF,
								QSharedPointer<iAHistogramData> data,
								QMultiMap<double, QList<double> > *,
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
	QSharedPointer<iAHistogramData> m_data;
	QSharedPointer<iAPlotData> m_selectedData;
	QSharedPointer<iAStepFunctionPlot> m_selectionDrawer;
	QList<QSharedPointer<iAStepFunctionPlot> > m_histogramDrawerList;
	QPoint m_selectionOrigin;
	QRubberBand * m_selectionRubberBand;
	QString m_xLabel;
	QString m_yLabel;
	QColor m_selectionColor;
	QList<int> m_addedHandles;

	int m_firstSelectedBin;
	int m_lastSelectedBin;

	QMultiMap<double, QList<double> > * m_histogramMap;
	const QTableWidget * m_rawTable;
	QSharedPointer<iATransferFunction> m_tf;

	int getBin( QMouseEvent *event );
	void setupSelectionDrawer();
};

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

#include "iAValueType.h"

class iAParamHistogramData;
class iANameMapper;

class iAFilterChart: public iADiagramFctWidget
{
	Q_OBJECT
public:
	iAFilterChart(QWidget* parent,
		QString const & caption,
		QSharedPointer<iAParamHistogramData> data,
		QSharedPointer<iANameMapper> nameMapper,
		bool showCaption = false);
	virtual QSharedPointer<iAAbstractDiagramRangedData> GetData();
	virtual QSharedPointer<iAAbstractDiagramRangedData> const GetData() const;
	double mapBinToValue(double bin) const;
	double mapValueToBin(double value) const;
	QSharedPointer<iAAbstractDrawableFunction> GetDrawer(QSharedPointer<iAParamHistogramData> data, QColor color);
	void RemoveMarker();
	void SetMarker(double value);
	virtual iAValueType GetRangeType() const;
	double GetMinVisibleBin() const;
	double GetMaxVisibleBin() const;

	void SetBinColor(int bin, QColor const & color);
	
	double GetMinSliderPos();
	double GetMaxSliderPos();
	void SetMinMaxSlider(double min, double max);
signals:
	void SelectionChanged();
protected:
	virtual void drawAxes(QPainter& painter);
	virtual void contextMenuEvent(QContextMenuEvent *event);
	virtual void mousePressEvent( QMouseEvent *event );
	virtual void mouseReleaseEvent( QMouseEvent *event );
	virtual void mouseMoveEvent( QMouseEvent *event );
private:
	virtual QSharedPointer<iAAbstractDrawableFunction> CreatePrimaryDrawer();
	virtual QString GetXAxisCaption(double value, int placesBeforeComma, int requiredPlacesAfterComma);

	QSharedPointer<iAParamHistogramData> m_data;
	QSharedPointer<iANameMapper> m_nameMapper;
	double m_markedLocation;
	QVector<QColor> m_binColors;

	double m_minSliderPos, m_maxSliderPos;
	int m_selectedHandle;
	int m_selectionOffset;

	int value2X(double value) const;
	double x2value(int x) const;
	void drawMarker(QPainter & painter, double markerLocation, QPen const & pen, QBrush const & brush);
};

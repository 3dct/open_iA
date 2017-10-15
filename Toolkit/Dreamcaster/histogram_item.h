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
#ifndef HISTOGRAM_ITEM_H
#define HISTOGRAM_ITEM_H

#include <qglobal.h>
#include <qcolor.h>

#include "qwt_plot_item.h" 

class QwtIntervalData;
class QString;
/**	\class HistogramItem
	\brief A histogram plot implementation based on QwtPlotItem class.

	Implementing base functionality of bar charts plot.	
*/
class HistogramItem: public QwtPlotItem
{
public:
    explicit HistogramItem(const QString &title = QString::null);
    explicit HistogramItem(const QwtText &title);
    virtual ~HistogramItem();

    void setData(const QwtIntervalData &data);
    const QwtIntervalData &data() const;

    void setColor(const QColor &);
    QColor color() const;

    virtual QwtDoubleRect boundingRect() const;

    virtual int rtti() const;

    virtual void draw(QPainter *, const QwtScaleMap &xMap, 
        const QwtScaleMap &yMap, const QRect &) const;

    void setBaseline(double reference);
    double baseline() const;

    enum HistogramAttribute
    {
        Auto = 0,
        Xfy = 1
    };

    void setHistogramAttribute(HistogramAttribute, bool on = true);
    bool testHistogramAttribute(HistogramAttribute) const;

protected:
    virtual void drawBar(QPainter *,
        Qt::Orientation o, const QRect &) const;

private:
    void init();

    class PrivateData;
    PrivateData *d_data;
};

#endif

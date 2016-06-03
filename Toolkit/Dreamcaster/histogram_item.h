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

/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "qthelper/iASignallingWidget.h"

#include <charts/iAChartWidget.h>

class iABarData;
class iABarsWidget;
class iAColorTheme;

class QGridLayout;
class QMenu;

class iAStackedBarChart: public iASignallingWidget
{
	Q_OBJECT
public:
	static const int MaxBarHeight = 100;
	static const int TextPadding = 5;
	iAStackedBarChart(iAColorTheme const* theme, QGridLayout* gL, int row, int col,
		bool header = false, bool last = false, bool chart = false,
		QString const & yLabelName = QString());
	void addBar(QString const & name, double value, double maxValue);
	void updateBar(QString const& name, double value, double maxValue);
	void removeBar(QString const & name);
	int barIndex(QString const& name) const;
	void setColorTheme(iAColorTheme const * theme);
	QMenu* contextMenu();
	void setDoStack(bool doStack);
	size_t numberOfBars() const;
	void setBackgroundColor(QColor const & color);
	double weightedSum() const;
	void setSelectedBar(int barIdx);
	QString barName(size_t barIdx) const;
	iAChartWidget* chart(size_t barIdx);
	void setLeftMargin(int leftMargin);
	double maxYValue() const;
	void setChartYRange(double yMin, double yMax);
signals:
	void switchedStackMode(bool mode);
	void weightsChanged(std::vector<double> const& weights);
	void clicked();
	void doubleClicked();
	void barClicked(size_t barID);
	void barDblClicked(size_t barID);
	void normalizeModeChanged(bool normalizePerBar);
public slots:
	void setWeights(std::vector<double> const & weights);
	void setNormalizeMode(bool normalizePerBar);
private slots:
	void switchStackMode();
	void resetWeights();
	void toggleNormalizeMode();
private:
	//! @{ Event Handlers:
	void contextMenuEvent(QContextMenuEvent *ev) override;
	void resizeEvent(QResizeEvent* e) override;
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseReleaseEvent(QMouseEvent* ev) override;
	void mouseMoveEvent(QMouseEvent* ev) override;
	//! @}
	
	void drawBar(QPainter& painter, size_t barID, int left, int top, int barHeight);
	int barHeight() const;
	void updateBars();
	size_t getBarAt(int x) const;

	size_t dividerWithinRange(int x) const;
	double weightAndNormalize(iABarData const& bar) const;
	int barWidth(iABarData const& bar) const;
	void normalizeWeights();
	void updateOverallMax();
	void updateChartBars();
	void updateDividers();

	void emitBarClick(size_t barID);
	void emitBarDblClick(size_t barID);

	std::vector<QSharedPointer<iABarData>> m_bars;
	std::vector<int> m_dividers;
	iAColorTheme const * m_theme;
	QMenu* m_contextMenu;
	bool m_header, m_stack, m_last;
	size_t m_resizeBar;
	int m_resizeStartX;
	double m_resizeWidth;
	std::vector<QSharedPointer<iABarData>> m_resizeBars;
	QColor m_bgColor;
	bool m_normalizePerBar;
	double m_overallMaxValue;
	int m_selectedBar;
	bool m_showChart;
	QString m_yLabelName;
	int m_leftMargin;
	QGridLayout* m_gL;
	int m_row, m_col;
	int m_chartAreaPixelWidth;

	iABarsWidget* m_barsWidget;

	friend class iABarsWidget;
	friend class iABarWidget;
};

class QGridLayout;

void addHeaderLabel(QGridLayout* layout, int column, QString const& text);

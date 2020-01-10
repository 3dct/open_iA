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
#include "qthelper/iASignallingWidget.h"

class iABarData;

class iAColorTheme;

class QMenu;

class iABarData
{
public:
	iABarData(): name(""), value(0), maxValue(1), weight(1.0)
	{}
	iABarData(QString const & name, double value, double maxValue, double weight):
		name(name), value(value), maxValue(maxValue), weight(weight)
	{}
	QString name;
	double value, maxValue, weight;
};

class iAStackedBarChart: public iASignallingWidget
{
	Q_OBJECT
public:
	const int MaxBarHeight = 50;
	const int TextPadding = 5;
	iAStackedBarChart(iAColorTheme const * theme, bool header = false);
	void addBar(QString const & name, double value, double maxValue);
	void removeBar(QString const & name);
	void setColorTheme(iAColorTheme const * theme);
	QMenu* contextMenu();
	void setDoStack(bool doStack);
	size_t numberOfBars() const;
	void setBackgroundColor(QColor const & color);
	double weightedSum() const;
signals:
	void switchedStackMode(bool mode);
	void weightsChanged(std::vector<double> const & weights);
	void doubleClicked();
public slots:
	void setWeights(std::vector<double> const & weights);
private slots:
	void switchStackMode();
private:
	//! @{ Event Handlers:
	void paintEvent(QPaintEvent* ev) override;
	void contextMenuEvent(QContextMenuEvent *ev) override;
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseReleaseEvent(QMouseEvent* ev) override;
	void mouseMoveEvent(QMouseEvent* ev) override;
	void mouseDoubleClickEvent(QMouseEvent* e) override;
	//! @}

	size_t dividerWithinRange(int x) const;
	int barWidth(iABarData const & bar) const;
	void normalizeWeights();

	std::vector<iABarData> m_bars;
	std::vector<int> m_dividers;
	iAColorTheme const * m_theme;
	QMenu* m_contextMenu;
	bool m_header, m_stack;
	size_t m_resizeBar;
	int m_resizeStartX;
	double m_resizeWidth;
	std::vector<iABarData> m_resizeBars;
	QColor m_bgColor;
};

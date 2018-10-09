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
#include <QWidget>

class iABarData;

class iAColorTheme;

class QMenu;

class iAStackedBarChart: public QWidget
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
signals:
	void switchedStackMode(bool mode);
	void weightsChanged(std::vector<double> const & weights);
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
	//! @}
	const int DividerRange = 2;

	int dividerWithinRange(int x) const;
	double weightSum() const;
	int barWidth(iABarData const & bar, double weightSum) const;

	std::vector<iABarData> m_bars;
	std::vector<int> m_dividers;
	iAColorTheme const * m_theme;
	QMenu* m_contextMenu;
	bool m_header, m_stack;
	int m_resizeBar;
	int m_resizeStartX;
	double m_resizeWidth, m_resizeFullWidth;
	std::vector<iABarData> m_resizeBars;
};

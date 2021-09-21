/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include <QColor>
#include <QWidget>

class QMouseEvent;
class QPainter;

class iAAlgorithmInfo : public QWidget
{
	Q_OBJECT
public:
	enum DisplayMode
	{
		Box,
		Matrix,
		DefaultDisplayMode = Matrix
	};

	iAAlgorithmInfo(QString const& name, QStringList const& inNames, QStringList const& outNames, QColor const& inColor,
		QColor const& outColor, QVector<QVector<QVector<QVector<double>>>> const& agrSens);
	void updateSensitivityMax();
	int boxHeight() const;
	int oneEntryHeight() const;
	void setSelectedInput(int inIdx);
	void addShownOut(int outIdx);
	void removeShownOut(int outIdx);
	void setInSortOrder(QVector<int> const& inSortOrder);
	void setMeasure(int newMeasure);
	void setAggregation(int newAggregation);
	void setMode(int mode);
	void setNormalizePerOutput(bool maxPerOut);

private:
	void drawInOut(QPainter& p, QRect textRect, QString const& text, QVector<QRect>& rects, QColor const& color,
		bool selected, bool useColor, bool verticalText);
	int connectorWidth(QFontMetrics fm, QStringList const& strings);
	void drawConnectors(QPainter& p, int left, int width, QStringList const& strings, QVector<QRect>& rects,
		QColor const& color, int selected, QVector<int> const& shown, QVector<int> const& sort, QVector<QPoint>& posOut,
		bool isLeft, int connHeight);
	void drawSensitivities(QPainter& p, QVector<QPoint> paramPt, QVector<QPoint> characPt);
	void drawLegend(QPainter& p, int leftWidth, bool top);
	void paintEvent(QPaintEvent* ev) override;
	void mousePressEvent(QMouseEvent* ev) override;
	QSize sizeHint() const override;
signals:
	void inputClicked(int inIdx);
	void outputClicked(int outIdx);
private:
	double mapVal(int inIdx, double val);

	QString m_name;
	QStringList m_inNames, m_outNames;
	QVector<QRect> m_inRects, m_outRects;
	QColor m_inColor, m_outColor;
	int m_selectedIn;
	QVector<int> m_shownOut;
	QVector<int> m_inSort;

	QVector<QVector<QVector<QVector<double>>>> const & m_agrSens;
	std::vector<double> m_maxS;
	double m_maxO;
	int m_measureIdx, m_aggrType;
	// some widths as determined during painting:
	int m_inWidth, m_outWidth, m_boxMinWidth, m_legendWidth;
	DisplayMode m_displayMode;
	bool m_normalizePerOut;
};

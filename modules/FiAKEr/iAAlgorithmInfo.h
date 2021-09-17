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

#include <QApplication>    // for qApp
#include <QWidget>

// could go into .cpp if separated:
#include <iALog.h>
#include <iAMathUtility.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

class iAAlgorithmInfo : public QWidget
{
	Q_OBJECT
public:
	//static const int ArrowHeadSize = 5;
	static const int ArrowTextDistance = 1;
	static const int ArrowTextLeft = 5;
	static const int RoundedCornerRadius = 3;
	static const int TopMargin = 1;
	static const int BottomMargin = 5; // ArrowHeadSize;
	static const int HMargin = 1;
	static const int TextHPadding = 3;
	static const int TextVPadding = 1;
	static const int ArrowMinBottomDist = 1;
	static const int MaxLineWidth = 4;

	static const QString LegendCaption;		// see iASensitivityInfo.cpp
	static const int LegendLineWidth = 15;
	static const int LegendNumEntries = 3;
	static const int LegendMargin = 6;
	static const int LegendSpacing = 3;
	static const int LegendHeightMin = 50;

	iAAlgorithmInfo(QString const& name, QStringList const& inNames, QStringList const& outNames,
		QColor const & inColor, QColor const & outColor, QVector<QVector<QVector<QVector<double>>>> const & agrSens):
		m_name(name), m_inNames(inNames), m_outNames(outNames), m_inColor(inColor), m_outColor(outColor),
		m_selectedIn(-1),
		m_agrSens(agrSens),
		m_measureIdx(0),
		m_aggrType(0),
		m_inWidth(1),
		m_outWidth(1),
		m_boxMinWidth(1)
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
	}
	int boxHeight() const
	{
		return geometry().height() - (TopMargin + BottomMargin);
	}
	int oneEntryHeight() const
	{
		return fontMetrics().height() + 2 * TextVPadding + ArrowTextDistance;
	}
	void setSelectedInput(int inIdx)
	{
		m_selectedIn = inIdx;
		update();
	}
	void addShownOut(int outIdx)
	{
		m_shownOut.push_back(outIdx);
	}
	void removeShownOut(int outIdx)
	{
		int idx = m_shownOut.indexOf(outIdx);
		if (idx == -1)
		{
			LOG(lvlWarn, QString("removeShownOut called for outIdx not currently shown (%1)!").arg(outIdx));
			return;
		}
		m_shownOut.remove(idx);
	}
	void setInSortOrder(QVector<int> const& inSortOrder)
	{
		m_inSort = inSortOrder;
		update();
	}
	void setMeasure(int newMeasure)
	{
		m_measureIdx = newMeasure;
		update();
	}

	void setAggregation(int newAggregation)
	{
		m_aggrType = newAggregation;
		update();
	}

private:
	void drawInOut(QPainter& p, int left, int top, int width, int height, QString const& text, QVector<QRect>& rects,
		QColor const& color, bool selected, bool useColor)
	{
		int right = left + width;
		p.drawLine(left, top, right, top);
		//p.drawLine(right - ArrowHeadSize, top - ArrowHeadSize, right, top);
		//p.drawLine(right - ArrowHeadSize, top + ArrowHeadSize, right, top);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		int textWidth = p.fontMetrics().horizontalAdvance(text);
#else
		int textWidth = p.fontMetrics().width(text);
#endif
		int boxHeight = p.fontMetrics().height() + 2 * TextVPadding;
		QRect textRect(left + ArrowTextLeft, top - ArrowTextDistance - boxHeight,
			std::min(textWidth + 2 * TextHPadding, width - ArrowTextLeft /* - ArrowHeadSize */),
			std::min(height, boxHeight));
		rects.push_back(textRect);
		QPainterPath path;
		path.addRoundedRect(textRect, RoundedCornerRadius, RoundedCornerRadius);
		if (useColor)
		{
			p.fillPath(path, color);
		}
		if (selected)
		{
			p.drawPath(path);
		}
		p.drawText(textRect, Qt::AlignCenter, text);
	}
	int connectorWidth(QFontMetrics fm, QStringList const & strings)
	{
		// determine max width:
		int width = 0;
		for (auto str : strings)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
			width = std::max(width, fm.horizontalAdvance(str));
#else
			width = std::max(fm.width(str));
#endif
		}
		return width;
	}
	void drawConnectors(QPainter& p, int left, int width, QStringList const& strings, QVector<QRect>& rects, QColor const& color,
		int selected, QVector<int> const& shown, QVector<int> const& sort, QVector<QPoint>& posOut, bool isLeft, int connHeight)
	{
		rects.clear();
		int bottomDistance = connHeight / (strings.size() + 1);
		double fontToBoxRatio = static_cast<double>(bottomDistance) / oneEntryHeight();
		int arrowBottomDistance = clamp(ArrowMinBottomDist, bottomDistance,
			static_cast<int>(mapValue(1.0, 4.0, ArrowMinBottomDist, bottomDistance, fontToBoxRatio)));
		//int arrowBottomDistance = ArrowBottomDist;
		int oneHeight = (connHeight - arrowBottomDistance) / strings.size();
		int baseTop = TopMargin + oneHeight;
		for (int idx = 0; idx < strings.size(); ++idx)
		{
			QString name = strings[sort.size() > idx ? sort[idx] : idx];
			posOut.push_back(QPoint(left + (isLeft ? width : 0), baseTop + idx * oneHeight));
			drawInOut(p, left, baseTop + idx * oneHeight, width, oneHeight, name, rects, color, selected == idx,
				shown.size() == 0 || shown.contains(idx));
		}
	}
	void drawSensitivities(QPainter& p, QVector<QPoint> paramPt, QVector<QPoint> characPt)
	{
		// determine max for proper scaling (determining min not needed - at no variation, the minimum is zero,
		// so norming the minimum encountered to 0 would lead to omitting showing the smallest variation influence:
		std::vector<double> maxS(m_agrSens.size());
		for (int c = 0; c < m_agrSens.size(); ++c)
		{
			auto const& d = m_agrSens[c][m_measureIdx][m_aggrType];
			maxS[c] = *std::max_element(d.begin(), d.end());
		}
		//LOG(lvlDebug, QString("min=%1, max=%2").arg(minS).arg(maxS));
		const int C = 255;
		for (int charIdx = 0; charIdx < m_agrSens.size(); ++charIdx)
		{
			for (int paramIdx = 0; paramIdx < m_agrSens[charIdx][m_measureIdx][m_aggrType].size(); ++paramIdx)
			{
				int pIdx = m_inSort.size() > 0 ? m_inSort[paramIdx] : paramIdx;
				double normVal = mapToNorm(0.0, maxS[charIdx], m_agrSens[charIdx][m_measureIdx][m_aggrType][pIdx]);
				if (!dblApproxEqual(normVal, 0.0))
				{
					// maybe draw in order of increasing sensitivities?
					int colorVal = C - (C * normVal);
					auto pen = p.pen();
					pen.setColor(QColor(colorVal, colorVal, colorVal));
					pen.setWidth(std::max(1.0, MaxLineWidth * normVal));
					p.setPen(pen);
					p.drawLine(paramPt[paramIdx], characPt[charIdx]);
				}
			}
		}
	}
	void drawLegend(QPainter& p, int leftWidth)
	{
		auto fm = p.fontMetrics();
		const double LegendHeight = std::max(LegendHeightMin, LegendNumEntries * fm.height());
		const double LegendEntryHeight = LegendHeight / LegendNumEntries;
		const int LegendTextWidth = leftWidth - LegendMargin - LegendLineWidth;
		const int C = 255;
		p.setPen(qApp->palette().color(QWidget::foregroundRole()));
		p.drawText(LegendMargin, height() - LegendMargin - LegendHeight - LegendSpacing, LegendCaption);
		double const LegendBottom = height() - LegendMargin;
		for (int i=0; i<LegendNumEntries; ++i)
		{
			double normVal = static_cast<double>(i) / (LegendNumEntries-1);
			QRect textRect(LegendMargin + LegendLineWidth + LegendSpacing,
				LegendBottom - (LegendNumEntries - i) * LegendEntryHeight,
				LegendTextWidth, LegendEntryHeight);
			p.drawText(textRect, Qt::AlignLeft |
				((i == 0) ?
					Qt::AlignTop :
					((i == LegendNumEntries-1) ?
						Qt::AlignBottom :
						Qt::AlignVCenter)),
				QString::number(normVal, 'f', 2));
		}
		QPolygon poly;
		int legendCenterX = LegendMargin + LegendSpacing + LegendLineWidth / 2;
		poly.push_back(QPoint(legendCenterX, LegendBottom - (LegendNumEntries * LegendEntryHeight)));
		poly.push_back(QPoint(legendCenterX - MaxLineWidth / 2, LegendBottom));
		poly.push_back(QPoint(legendCenterX + MaxLineWidth / 2, LegendBottom));
		poly.push_back(poly[0]); // close loop back to point 0
		QPainterPath path;
		path.addPolygon(poly);
		QLinearGradient gradient;
		gradient.setColorAt(0, QColor(255, 255, 255));
		gradient.setColorAt(1, QColor(0, 0, 0));
		gradient.setStart(poly[0]);
		gradient.setFinalStop(QPoint(legendCenterX, LegendBottom));
		p.fillPath(path, gradient);
	}
	void paintEvent(QPaintEvent* ev) override
	{
		Q_UNUSED(ev);
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing);
		p.setPen(qApp->palette().color(QWidget::foregroundRole()));

		m_inWidth = connectorWidth(p.fontMetrics(), m_inNames) + 2 * ArrowTextLeft + 2 * RoundedCornerRadius;
		// to make sure there's space for the legend:
		m_inWidth = std::max(m_inWidth,
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
			p.fontMetrics().horizontalAdvance(LegendCaption) + 2*LegendMargin
#else
			p.fontMetrics().width(text);
#endif
		);

		m_outWidth = connectorWidth(p.fontMetrics(), m_outNames) + 2 * ArrowTextLeft + 2 * RoundedCornerRadius;
		//if (leftConnectorW + rightConnectorW > width())
		//{
			// reduce?
		//}
		m_boxMinWidth =
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
			p.fontMetrics().horizontalAdvance(m_name);
#else
			p.fontMetrics().width(m_name);
#endif
		QRect algoBox(HMargin + m_inWidth, TopMargin, width() - (2 * HMargin + m_inWidth + m_outWidth), boxHeight());
		p.drawRect(algoBox);
		p.drawText(algoBox, Qt::AlignHCenter | Qt::AlignBottom, m_name);
		QVector<QPoint> paramPt, characPt;
		drawConnectors(p, HMargin, m_inWidth, m_inNames, m_inRects, m_inColor, m_selectedIn, QVector<int>(), m_inSort, paramPt, true,
			boxHeight() - LegendHeightMin - LegendMargin - LegendSpacing - p.fontMetrics().height());
		drawConnectors(p, HMargin + m_inWidth + algoBox.width(), m_outWidth, m_outNames, m_outRects, m_outColor, -1, m_shownOut, QVector<int>(),
			characPt, false, boxHeight());
		drawSensitivities(p, paramPt, characPt);
		drawLegend(p, m_inWidth);
	}
	void mousePressEvent(QMouseEvent* ev) override
	{
		for (int rIdx = 0; rIdx < m_inRects.size(); ++rIdx)
		{
			if (m_inRects[rIdx].contains(ev->pos()))
			{
				int clickedIn = (rIdx < m_inSort.size()) ? m_inSort[rIdx] : rIdx;
				emit inputClicked(clickedIn);
				return;
			}
		}
		for (int rIdx = 0; rIdx < m_outRects.size(); ++rIdx)
		{
			if (m_outRects[rIdx].contains(ev->pos()))
			{
				emit outputClicked(rIdx);
				return;
			}
		}
	}
	QSize sizeHint() const override
	{
		return QSize(m_inWidth + m_outWidth + m_boxMinWidth, oneEntryHeight() * std::max(m_inNames.size(), m_outNames.size()));
	}
signals:
	void inputClicked(int inIdx);
	void outputClicked(int outIdx);
private:
	QString m_name;
	QStringList m_inNames, m_outNames;
	QVector<QRect> m_inRects, m_outRects;
	QColor m_inColor, m_outColor;
	int m_selectedIn;
	QVector<int> m_shownOut;
	QVector<int> m_inSort;

	QVector<QVector<QVector<QVector<double>>>> const & m_agrSens;
	int m_measureIdx, m_aggrType;
	int m_inWidth, m_outWidth, m_boxMinWidth;
};

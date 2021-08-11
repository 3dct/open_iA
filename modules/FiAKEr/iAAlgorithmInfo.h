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
	static const int ArrowHeadSize = 5;
	static const int ArrowTextDistance = 1;
	static const int ArrowTextLeft = 5;
	static const int RoundedCornerRadius = 3;
	static const int TopMargin = 1;
	static const int BottomMargin = ArrowHeadSize;
	static const int HMargin = 1;
	static const int TextHPadding = 3;
	static const int TextVPadding = 1;
	static const int ArrowMinBottomDist = 1;
	iAAlgorithmInfo(QString const& name, QStringList const& inNames, QStringList const& outNames,
		QColor const & inColor, QColor const & outColor, QVector<QVector<QVector<QVector<double>>>> const & agrSens):
		m_name(name), m_inNames(inNames), m_outNames(outNames), m_inColor(inColor), m_outColor(outColor),
		m_selectedIn(-1),
		m_agrSens(agrSens),
		m_measureIdx(0),
		m_aggrType(0)
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
	}
	int boxWidth() const
	{
		return (geometry().width() - 2 * HMargin) / 3;
	}
	int boxHeight() const
	{
		return geometry().height() - (TopMargin + BottomMargin);
	}
	int oneEntryHeight() const
	{
		return fontMetrics().height() + 2 * TextVPadding + ArrowTextDistance;
	}
	void drawArrow(QPainter& p, int left, int top, int width, int height, QString const& text,
		QVector<QRect>& rects, QColor const& color, bool selected, bool useColor)
	{
		int right = left + width;
		p.drawLine(left, top, right, top);
		p.drawLine(right - ArrowHeadSize, top - ArrowHeadSize, right, top);
		p.drawLine(right - ArrowHeadSize, top + ArrowHeadSize, right, top);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		int textWidth = p.fontMetrics().horizontalAdvance(text);
#else
		int textWidth = p.fontMetrics().width(text);
#endif
		int boxHeight = p.fontMetrics().height() + 2 * TextVPadding;
		QRect textRect(left + ArrowTextLeft,
			top - ArrowTextDistance - boxHeight,
			std::min(textWidth + 2 * TextHPadding, width - ArrowTextLeft - ArrowHeadSize),
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
	void drawConnectors(QPainter& p, int left, QStringList const& strings, QVector<QRect>& rects,
		QColor const& color, int selected, QVector<int> const & shown, QVector<int> const &sort, QVector<QPoint> & posOut, bool isLeft)
	{
		rects.clear();
		int bottomDistance = boxHeight() / (strings.size() + 1);
		double fontToBoxRatio = static_cast<double>(bottomDistance) / oneEntryHeight();
		int arrowBottomDistance = clamp(ArrowMinBottomDist, bottomDistance,
			static_cast<int>(mapValue(1.0, 4.0,	ArrowMinBottomDist, bottomDistance, fontToBoxRatio)));
		//int arrowBottomDistance = ArrowBottomDist;
		int oneHeight = (boxHeight() - arrowBottomDistance) / strings.size();
		int baseTop = TopMargin + oneHeight;
		for (int idx = 0; idx < strings.size(); ++idx)
		{
			QString name = strings[ sort.size() > idx ? sort[idx] : idx ];
			posOut.push_back(QPoint(left + (isLeft ? boxWidth():0), baseTop + idx * oneHeight));
			drawArrow(p, left, baseTop + idx * oneHeight, boxWidth(), oneHeight,
				name, rects, color, selected == idx, shown.size() == 0 || shown.contains(idx));
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
		for (int charIdx = 0; charIdx < m_agrSens.size(); ++charIdx)
		{
			for (int paramIdx = 0; paramIdx < m_agrSens[charIdx][m_measureIdx][m_aggrType].size(); ++paramIdx)
			{
				auto pen = p.pen();
				double normVal = mapToNorm(0.0, maxS[charIdx], m_agrSens[charIdx][m_measureIdx][m_aggrType][paramIdx]);
				pen.setWidth(std::max(1.0, 3 * normVal));
				if (!dblApproxEqual(normVal, 0.0))
				{
					// maybe draw in order of increasing sensitivities?
					const int C = 255;
					int colorVal = C - (C * normVal);
					pen.setColor(QColor(colorVal, colorVal, colorVal));
					p.setPen(pen);
					p.drawLine(paramPt[paramIdx], characPt[charIdx]);
				}
			}
		}
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
	void paintEvent(QPaintEvent* ev) override
	{
		Q_UNUSED(ev);
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing);
		p.setPen(qApp->palette().color(QWidget::foregroundRole()));

		QRect algoBox(HMargin + boxWidth(), TopMargin, boxWidth(), boxHeight());
		p.drawRect(algoBox);
		p.drawText(algoBox, Qt::AlignHCenter | Qt::AlignBottom, m_name);
		QVector<QPoint> paramPt, characPt;
		drawConnectors(p, HMargin, m_inNames, m_inRects, m_inColor, m_selectedIn, QVector<int>(), m_inSort, paramPt, true);
		drawConnectors(p, HMargin + 2 * boxWidth(), m_outNames, m_outRects, m_outColor, -1, m_shownOut, QVector<int>(), characPt, false);

		drawSensitivities(p, paramPt, characPt);
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
		return QSize(1, oneEntryHeight() * std::max(m_inNames.size(), m_outNames.size()));
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
};

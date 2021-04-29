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
		QColor const & inColor, QColor const & outColor):
		m_name(name), m_inNames(inNames), m_outNames(outNames), m_inColor(inColor), m_outColor(outColor),
		m_selectedIn(-1)
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
	void drawConnections(QPainter& p, int left, QStringList const& strings, QVector<QRect>& rects,
		QColor const& color, int selected, QVector<int> const & shown)
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
			drawArrow(p, left, baseTop + idx * oneHeight, boxWidth(), oneHeight,
				strings[idx], rects, color, selected == idx, shown.size() == 0 || shown.contains(idx));
		}
	}
	void paintEvent(QPaintEvent* ev) override
	{
		Q_UNUSED(ev);
		QPainter p(this);
		p.setPen(qApp->palette().color(QWidget::foregroundRole()));

		QRect algoBox(HMargin + boxWidth(), TopMargin, boxWidth(), boxHeight());
		p.drawRect(algoBox);
		p.drawText(algoBox, Qt::AlignCenter, m_name);

		drawConnections(p, HMargin, m_inNames, m_inRects, m_inColor, m_selectedIn, QVector<int>());
		drawConnections(p, HMargin + 2 * boxWidth(), m_outNames, m_outRects, m_outColor, -1, m_shownOut);
	}

	void mousePressEvent(QMouseEvent* ev) override
	{
		for (int rIdx = 0; rIdx < m_inRects.size(); ++rIdx)
		{
			if (m_inRects[rIdx].contains(ev->pos()))
			{
				emit inputClicked(rIdx);
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
};

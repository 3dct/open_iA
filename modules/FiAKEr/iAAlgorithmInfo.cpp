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
#include "iAAlgorithmInfo.h"

#include <QApplication>    // for qApp

#include <iALog.h>
#include <iAMathUtility.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

namespace
{
	const int ArrowHeadSize = 5;
	const int ArrowTextDistance = 1;
	const int ArrowTextLeft = 5;
	const int RoundedCornerRadius = 3;
	const int TopMargin = 1;
	const int BottomMargin = 5;  // ArrowHeadSize;
	const int HMargin = 2;
	const int TextHPadding = 3;
	const int TextVPadding = 1;
	const int ArrowMinBottomDist = 1;
	
	const int DefaultMaxLineWidth = 5;

	const int LegendLineWidth = 15;
	const int LegendNumEntries = 2;
	const int LegendMargin = 6;
	const int LegendSpacing = 3;
	const int LegendHeightMin = 40;

	const int MatrixMinBoxSize = 3;

	const QString LegendCaption("Sensitivity:");
	const QColor SelectionColor(0, 0, 0);

	void drawVerticalText(QPainter& p, QRect const& textRect, int flags, QString const& text)
	{
		p.save();
		p.rotate(-90);
		QRect rotRect(-(textRect.height() + HMargin), textRect.left(), textRect.height(), textRect.width());
		p.drawText(rotRect, flags, text);
		p.restore();
	}
}

iAAlgorithmInfo::iAAlgorithmInfo(QString const& name, QStringList const& inNames, QStringList const& outNames,
	QColor const& inColor, QColor const& outColor) :
	m_name(name),
	m_inNames(inNames),
	m_outNames(outNames),
	m_inColor(inColor),
	m_outColor(outColor),
	m_selectedIn(-1),
	m_maxPerColumn(m_outNames.size()),
	m_inWidth(1),
	m_outWidth(1),
	m_boxMinWidth(1),
	m_legendWidth(1),
	m_legendLineWidth(DefaultMaxLineWidth),
	m_displayMode(DefaultDisplayMode),
	m_normalizePerOutput(false),
	m_showArrows(false),
	m_showHighlight(false),
	m_mergeHighlight(true)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
	setMouseTracking(true);
}

void iAAlgorithmInfo::updateMatrixMax()
{
	// determine max sensitivity (per output characteristic)
	// for proper scaling (determining min not needed - at no variation, the minimum is zero,
	// so norming the minimum encountered to 0 would lead to omitting showing the smallest variation influence:
	assert(m_matrix.size() == m_outNames.size());
	for (int col = 0; col < m_matrix.size(); ++col)
	{
		assert(m_matrix[col].size() == m_inNames.size());
		m_maxPerColumn[col] = *std::max_element(m_matrix[col].begin(), m_matrix[col].end());
	}
	m_maxTotal = *std::max_element(m_maxPerColumn.begin(), m_maxPerColumn.end());
}

int iAAlgorithmInfo::boxHeight() const
{
	return geometry().height() - (TopMargin + BottomMargin);
}

int iAAlgorithmInfo::oneEntryHeight() const
{
	return fontMetrics().height() + 2 * TextVPadding + ArrowTextDistance;
}

void iAAlgorithmInfo::setSelectedInput(int inIdx)
{
	m_selectedIn = inIdx;
	update();
}

void iAAlgorithmInfo::addShownOut(int outIdx)
{
	m_shownOut.push_back(outIdx);
	std::sort(m_shownOut.begin(), m_shownOut.end());
}

void iAAlgorithmInfo::removeShownOut(int outIdx)
{
	int idx = m_shownOut.indexOf(outIdx);
	if (idx == -1)
	{
		LOG(lvlWarn, QString("removeShownOut called for outIdx not currently shown (%1)!").arg(outIdx));
		return;
	}
	m_shownOut.remove(idx);
}

void iAAlgorithmInfo::setInSortOrder(QVector<int> const& inSortOrder)
{
	m_inSort = inSortOrder;
	update();
}

void iAAlgorithmInfo::setMode(int mode)
{
	m_displayMode = static_cast<DisplayMode>(mode);
	update();
}

void iAAlgorithmInfo::setNormalizePerOutput(bool maxPerOut)
{
	m_normalizePerOutput = maxPerOut;
	update();
}

void iAAlgorithmInfo::setShowArrows(bool showArrows)
{
	m_showArrows = showArrows;
	update();
}

void iAAlgorithmInfo::setShowHighlight(bool showHighlight)
{
	m_showHighlight = showHighlight;
	update();
}

void iAAlgorithmInfo::setMergeHighlight(bool mergeHighlight)
{
	m_mergeHighlight = mergeHighlight;
	update();
}

void iAAlgorithmInfo::drawInOut(QPainter& p, QRect textRect, QString const& text, QVector<QRect>& rects,
	QColor const& color, bool selected, bool useColor, bool verticalText)
{
	rects.push_back(textRect);
	QPainterPath path;
	path.addRoundedRect(textRect, RoundedCornerRadius, RoundedCornerRadius);
	if (useColor)
	{
		p.fillPath(path, color);
	}
	if (selected)
	{
		p.setPen(SelectionColor);
		p.drawPath(path);
	}
	// add some inner padding:
	textRect.adjust(RoundedCornerRadius, TextVPadding, -RoundedCornerRadius, -TextVPadding);
	if (verticalText)
	{
		drawVerticalText(p, textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
	}
	else
	{
		p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
	}
}

int iAAlgorithmInfo::connectorWidth(QFontMetrics fm, QStringList const& strings)
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

void iAAlgorithmInfo::drawConnectors(QPainter& p, int left, int width, QStringList const& strings,
	QVector<QRect>& rects,
	QColor const& color, int selected, QVector<int> const& shown, QVector<int> const& sort, QVector<QPoint>& posOut,
	bool isLeft, int connHeight)
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
		auto sortIdx = sort.size() > idx ? sort[idx] : idx;
		QString name = strings[sortIdx];
		posOut.push_back(QPoint(left + (isLeft ? width : 0), baseTop + idx * oneHeight));
		int top = baseTop + idx * oneHeight;
		int right = left + width;
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		int textWidth = p.fontMetrics().horizontalAdvance(name);
#else
		int textWidth = p.fontMetrics().width(text);
#endif
		int boxHeight = p.fontMetrics().height() +  2 * (TextVPadding+RoundedCornerRadius);
		QRect textRect(left + ArrowTextLeft, top - ArrowTextDistance - boxHeight,
			std::min(textWidth + 2 * TextHPadding, width - ArrowTextLeft - (m_showArrows ? ArrowHeadSize : 0) ),
			std::min(oneHeight, boxHeight));
		drawInOut(
			p, textRect, name, rects, color, selected == sortIdx, shown.size() == 0 || shown.contains(idx), false);

		// draw line (/arrow) underneath text:
		p.drawLine(left, top, right, top);
		if (m_showArrows)
		{
			p.drawLine(right - ArrowHeadSize, top - ArrowHeadSize, right, top);
			p.drawLine(right - ArrowHeadSize, top + ArrowHeadSize, right, top);
		}
	}
}

void iAAlgorithmInfo::drawBoxLinks(QPainter& p, QVector<QPoint> inPt, QVector<QPoint> outPt, QRect const & algoBox)
{
	p.save();
	p.setRenderHint(QPainter::Antialiasing);
	p.setClipRect(algoBox);
	const int C = 255;
	for (int outIdx = 0; outIdx < m_matrix.size(); ++outIdx)
	{
		for (int inIdx = 0; inIdx < m_matrix[outIdx].size(); ++inIdx)
		{
			int pIdx = m_inSort.size() > 0 ? m_inSort[inIdx] : inIdx;
			double normVal = mapVal(outIdx, m_matrix[outIdx][pIdx]);
			if (!dblApproxEqual(normVal, 0.0))
			{
				// maybe draw in order of increasing sensitivities?
				int colorVal = C - (C * normVal);
				auto pen = p.pen();
				pen.setColor(QColor(colorVal, colorVal, colorVal));
				pen.setWidth(std::max(1.0, m_legendLineWidth * normVal));
				pen.setCapStyle(Qt::FlatCap);
				p.setPen(pen);
				p.drawLine(inPt[inIdx], outPt[outIdx]);
			}
		}
	}
	p.restore();
}

void iAAlgorithmInfo::drawLegend(QPainter& p, int leftWidth, bool top)
{
	auto fm = p.fontMetrics();
	const double LegendHeight = std::max(LegendHeightMin, LegendNumEntries * fm.height());
	const double LegendEntryHeight = LegendHeight / LegendNumEntries;
	const int LegendTextWidth = leftWidth - LegendMargin - LegendLineWidth;
	const int C = 255;
	double const LegendBottom = top ? LegendHeight + fm.height() + LegendSpacing : height() - LegendMargin;
	p.setPen(QApplication::palette().color(QWidget::foregroundRole()));
	p.drawText(LegendMargin, LegendBottom - LegendHeight - LegendSpacing, LegendCaption);
	m_legendWidth = LegendMargin +
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		fm.horizontalAdvance(LegendCaption);
#else
		fm.width(LegendCaption);
#endif
	for (int i = 0; i < LegendNumEntries; ++i)
	{
		double normVal = static_cast<double>(i) / (LegendNumEntries - 1);
		QRect textRect(LegendMargin + LegendLineWidth + LegendSpacing,
			LegendBottom - (LegendNumEntries - i) * LegendEntryHeight, LegendTextWidth, LegendEntryHeight);
		p.drawText(textRect,
			Qt::AlignLeft |
				((i == 0) ? Qt::AlignTop : ((i == (LegendNumEntries - 1)) ? Qt::AlignBottom : Qt::AlignVCenter)),
			QString::number(normVal, 'f', 2));
	}
	QPolygon poly;
	int legendCenterX = LegendMargin + LegendSpacing + LegendLineWidth / 2;
	int LegendTop = LegendBottom - (LegendNumEntries * LegendEntryHeight);
	p.save();
	if (top)
	{
		poly.push_back(QPoint(legendCenterX + m_legendLineWidth / 2, LegendTop));
		poly.push_back(QPoint(legendCenterX - m_legendLineWidth / 2, LegendTop));
	}
	else
	{
		p.setRenderHint(QPainter::Antialiasing);
		poly.push_back(QPoint(legendCenterX, LegendTop));
	}
	poly.push_back(QPoint(legendCenterX - m_legendLineWidth / 2, LegendBottom));
	poly.push_back(QPoint(legendCenterX + m_legendLineWidth / 2, LegendBottom));
	poly.push_back(poly[0]);  // close loop back to point 0
	QPainterPath path;
	path.addPolygon(poly);
	QLinearGradient gradient;
	gradient.setColorAt(0, QColor(255, 255, 255));
	gradient.setColorAt(1, QColor(0, 0, 0));
	gradient.setStart(poly[0]);
	gradient.setFinalStop(QPoint(legendCenterX, LegendBottom));
	p.fillPath(path, gradient);
	p.restore();
}

void iAAlgorithmInfo::paintEvent(QPaintEvent* ev)
{
	Q_UNUSED(ev);
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, false);
	p.setPen(QApplication::palette().color(QWidget::foregroundRole()));

	m_inWidth = connectorWidth(p.fontMetrics(), m_inNames) + 2 * ArrowTextLeft + 2 * RoundedCornerRadius;
	// to make sure there's space for the legend:
	m_inWidth = std::max(m_inWidth,
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		p.fontMetrics().horizontalAdvance(LegendCaption) + 2 * LegendMargin
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

	if (m_displayMode == Box)
	{
		QRect algoBox(
			HMargin + m_inWidth, TopMargin, width() - (2 * HMargin + m_inWidth + m_outWidth), boxHeight());
		p.drawRect(algoBox);
		p.drawText(algoBox, Qt::AlignHCenter | Qt::AlignBottom, m_name);
		QVector<QPoint> inPt, outPt;
		drawConnectors(p, HMargin, m_inWidth, m_inNames, m_inRects, m_inColor, m_selectedIn, QVector<int>(), m_inSort,
			inPt, true,
			boxHeight() - LegendHeightMin - LegendMargin - LegendSpacing - p.fontMetrics().height());
		drawConnectors(p, HMargin + m_inWidth + algoBox.width(), m_outWidth, m_outNames, m_outRects, m_outColor, -1,
			m_shownOut, QVector<int>(), outPt, false, boxHeight());
		drawBoxLinks(p, inPt, outPt, algoBox);
		drawLegend(p, m_inWidth, false);
	}
	else
	{
		drawLegend(p, m_inWidth, true);
		m_matrixRect = QRect(HMargin + m_inWidth, HMargin + m_outWidth + TextHPadding,
			width() - (2 * HMargin + m_inWidth), height() - (2 * HMargin + m_outWidth + TextHPadding));
		int cellWidth = m_matrixRect.width() / m_outNames.size();
		int cellHeight = m_matrixRect.height() / m_inNames.size();

		// draw labels:
		m_inRects.clear();
		m_outRects.clear();
		p.drawText(QRect(0, 0, m_matrixRect.left(), m_matrixRect.top()), Qt::AlignHCenter | Qt::AlignBottom, "In");
		for (int inIdx = 0; inIdx < m_inNames.size(); ++inIdx)
		{
			int pIdx = m_inSort.size() > 0 ? m_inSort[inIdx] : inIdx;
			QRect textRect(HMargin, m_matrixRect.top() + inIdx * cellHeight + TextVPadding, m_inWidth - 2 * HMargin,
				cellHeight - 2 * TextVPadding);
			drawInOut(p, textRect, m_inNames[pIdx], m_inRects, m_inColor, m_selectedIn == pIdx, true, false);
		}
		const int VertCaptHeight = p.fontMetrics().height();
		drawVerticalText(p,
			QRect(m_matrixRect.left() - VertCaptHeight - HMargin, m_matrixRect.top() - TextHPadding, VertCaptHeight,
				m_outWidth), Qt::AlignHCenter | Qt::AlignTop, "Out");
		for (int outIdx = 0; outIdx < m_outNames.size(); ++outIdx)
		{
			QRect textRect(m_matrixRect.left() + outIdx * cellWidth + TextVPadding,
				TextHPadding, cellWidth - 2 * TextVPadding, m_outWidth - TextHPadding);
			drawInOut(p, textRect, m_outNames[outIdx], m_outRects, m_outColor, false,
				m_shownOut.contains(outIdx), true);
		}

		// draw sensitivities :
		const int C = 255;
		for (int outIdx = 0; outIdx < m_matrix.size(); ++outIdx)
		{
			for (int inIdx = 0; inIdx < m_matrix[outIdx].size(); ++inIdx)
			{
				int pIdx = m_inSort.size() > 0 ? m_inSort[inIdx] : inIdx;
				double normVal = mapVal(outIdx, m_matrix[outIdx][pIdx]);
				int colorVal = C - (C * normVal);
				QRect boxRect(m_matrixRect.left() + outIdx * cellWidth, m_matrixRect.top() + inIdx * cellHeight,
					cellWidth, cellHeight);
				p.fillRect(boxRect, QColor(colorVal, colorVal, colorVal));
			}
		}

		// draw matrix grid lines:
		p.setPen(QApplication::palette().color(QPalette::AlternateBase));
		p.drawRect(m_matrixRect);
		for (int inIdx = 1; inIdx < m_inNames.size(); ++inIdx)
		{
			int y = m_matrixRect.top() + inIdx * cellHeight;
			p.drawLine(m_matrixRect.left(), y, m_matrixRect.right(), y);
		}
		for (int outIdx = 1; outIdx < m_outNames.size(); ++outIdx)
		{
			int x = m_matrixRect.left() + outIdx * cellWidth;
			p.drawLine(x, m_matrixRect.top(), x, m_matrixRect.bottom());
		}

		// highlight "selected" columns:
		if (m_showHighlight)
		{
			// TODO: merge neighbouring columns!
			p.setPen(QApplication::palette().color(QWidget::foregroundRole()));
			if (m_mergeHighlight)
			{
				QVector<int>::size_type startIdx = 0;
				while (startIdx < m_shownOut.size())
				{
					int startColIdx = m_shownOut[startIdx];
					int endIdx = startIdx + 1;
					while (endIdx < m_shownOut.size() && m_shownOut[endIdx] == (startColIdx + (endIdx - startIdx)) )
					{
						++endIdx;
					}
					int x = m_matrixRect.left() + startColIdx * cellWidth;
					p.drawRect(x, m_matrixRect.top(), (endIdx - startIdx) * cellWidth, m_matrixRect.height());
					startIdx = endIdx;
				}
			}
			else
			{
				for (auto i : m_shownOut)
				{
					int x = m_matrixRect.left() + i * cellWidth;
					p.drawRect(x, m_matrixRect.top(), cellWidth, m_matrixRect.height());
				}
			}
		}
	}
}

void iAAlgorithmInfo::mousePressEvent(QMouseEvent* ev)
{
	for (int rIdx = 0; rIdx < m_inRects.size(); ++rIdx)
	{
		if (m_inRects[rIdx].contains(ev->pos()))
		{
			int clickedIn = (rIdx < m_inSort.size()) ? m_inSort[rIdx] : rIdx;
			setSelectedInput(clickedIn);
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

void iAAlgorithmInfo::mouseMoveEvent(QMouseEvent* event)
{
	if (m_displayMode == Box)
	{
		return;
	}
	int cellWidth  = m_matrixRect.width() / m_outNames.size();
	int cellHeight = m_matrixRect.height() / m_inNames.size();

#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
	QPointF mousePoint(event->x(), event->y());
#else
	QPointF mousePoint(event->position());
#endif
	if (!m_matrixRect.contains(mousePoint.x(), mousePoint.y()))
	{
		QToolTip::hideText();
		return;
	}
	QPointF matrixPoint = mousePoint - m_matrixRect.topLeft();
	int col = clamp(0, static_cast<int>(m_matrix.size()-1), static_cast<int>(matrixPoint.x() / cellWidth));  // out
	int row = clamp(0, static_cast<int>(m_matrix[col].size()-1), static_cast<int>(matrixPoint.y() / cellHeight)); // in
	auto sortIdx = m_inSort.size() > row ? m_inSort[row] : row;
	QToolTip::showText(event->globalPos(),
		QString("%1/%2: %3").arg(m_inNames[sortIdx]).arg(m_outNames[col]).arg(m_matrix[col][sortIdx]));
}

QSize iAAlgorithmInfo::sizeHint() const
{
	return (m_displayMode == Box)
		? QSize(m_inWidth + m_outWidth + m_boxMinWidth,
				oneEntryHeight() * std::max(m_inNames.size(), m_outNames.size()))
		: QSize(m_inWidth + m_outNames.size() * MatrixMinBoxSize, m_outWidth + m_inNames.size() * MatrixMinBoxSize);
}

void iAAlgorithmInfo::setMatrix(iAMatrixType const& matrix)
{
	m_matrix = matrix;
	updateMatrixMax();
	update();
}

double iAAlgorithmInfo::mapVal(int outIdx, double val)
{
	return mapToNorm(0.0, m_normalizePerOutput ? m_maxPerColumn[outIdx] : m_maxTotal, val);
}

void iAAlgorithmInfo::setInOutColor(QColor const& inColor, QColor const& outColor)
{
	m_inColor = inColor;
	m_outColor = outColor;
	update();
}

void iAAlgorithmInfo::setLegendLineWidth(int lineWidth)
{
	m_legendLineWidth = lineWidth;
	update();
}

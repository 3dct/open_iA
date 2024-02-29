// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAFiberResult.h"            // for iAFiberSimilarity -> REFACTOR!!!

#include <iALookupTable.h>
#include <iAMathUtility.h>
#include <iAStringHelper.h>

#include <QFontMetrics>
#include <QPainter>
#include <QVector>
#include <QWidget>

class iAResultPairInfo
{
public:
	iAResultPairInfo();
	iAResultPairInfo(int measureCount);
	// average dissimilarity, per dissimilarity measure
	QVector<double> avgDissim;

	// for every fiber, and every dissimilarity measure, the n best matching fibers (in descending match quality)
	QVector<QVector<QVector<iAFiberSimilarity>>> fiberDissim;
};

// result similarity matrix type: resultID1 x resultID2
using iADissimilarityMatrixType = QVector<QVector<iAResultPairInfo>>;

using iAMatrixValuesType = std::vector<std::vector<double>>;

// TODO: Refactor to use more generic data source
class iAMatrixWidget : public QWidget
{
public:
	iAMatrixWidget(iADissimilarityMatrixType const& data,
		iAMatrixValuesType const& paramValues, bool showScalarBar, bool showAxes) :
		m_data(data),
		m_paramValues(paramValues),
		m_sortParam(0),
		m_dataIdx(0),
		m_showScalarBar(showScalarBar),
		m_showAxes(showAxes)
		//m_paramName(paramName)
		//m_quadraticAspectRatio(true)
	{
		m_range[0] = m_range[1] = 0;
	}
	void setData(int idx)
	{
		m_dataIdx = idx;
		m_range[0] = std::numeric_limits<double>::max();
		m_range[1] = std::numeric_limits<double>::lowest();
		for (iADissimilarityMatrixType::size_type i = 0; i < m_data.size(); ++i)
		{
			for (iADissimilarityMatrixType::size_type j = 0; j < m_data[i].size(); ++j)
			{
				double value = (i == j)? 0 : m_data[i][j].avgDissim[m_dataIdx];
				if (value < m_range[0])
				{
					m_range[0] = value;
				}
				if (value > m_range[1])
				{
					m_range[1] = value;
				}
			}
		}
		if (m_lut.initialized())
		{
			m_lut.setRange(m_range);
		}
	}
	void setLookupTable(iALookupTable const & lut)
	{
		m_lut = lut;
	}
	void setSortParameter(int paramIdx)
	{
		m_sortParam = paramIdx;
		m_sortOrder = sort_indexes(m_paramValues[paramIdx]);
	}
	double const* range()
	{
		return m_range;
	}
private:
	void drawAxisText(QPainter& p, int textStartX, int textBoxWidth, int textHeight, int outerAxisPadding, double value)
	{
		// x label:
		p.drawText(textStartX, outerAxisPadding, textBoxWidth, textHeight, Qt::AlignCenter, QString::number(value));
		// y label:
		p.save();
		p.translate(outerAxisPadding, textStartX + textBoxWidth);
		p.rotate(-90);
		p.drawText(0, 0, textBoxWidth, textHeight, Qt::AlignCenter, QString::number(value));
		p.restore();
	}
	void drawAxisTick(QPainter& p, int axisPos, int outTickDist, int tickPos)
	{
		p.drawLine(tickPos, outTickDist, tickPos, axisPos);    // x tick
		p.drawLine(outTickDist, tickPos, axisPos, tickPos);    // y tick
	}
	void paintEvent(QPaintEvent* /*event*/) override
	{
		if (!m_lut.initialized())
		{
			return;
		}

		QPainter p(this);
		QFontMetrics fm = p.fontMetrics();

		int scalarBarWidth = 20;
		int scalarBarPadding = 4;
		int outerAxisPadding = 0;
		int innerAxisPadding = 4;
		int axisTickHeight = 5;

		QString minStr = dblToStringWithUnits(m_range[0]);
		QString maxStr = dblToStringWithUnits(m_range[1]);
		int textWidth = std::max(fm.horizontalAdvance(minStr), fm.horizontalAdvance(maxStr));
		int textHeight = fm.height();
		int fullScalarBarWidth = m_showScalarBar ? (3 * scalarBarPadding + scalarBarWidth + textWidth) : 0;
		int axisSize = m_showAxes ? (textHeight + innerAxisPadding + outerAxisPadding + axisTickHeight) : 0;
		int cellPixel = std::max(1,
			std::min((geometry().height() - axisSize) / static_cast<int>(m_data.size()),
				(geometry().width() - fullScalarBarWidth - axisSize) / static_cast<int>(m_data.size())));
		QRect matrixRect(axisSize, axisSize, static_cast<int>(m_data.size() * cellPixel), static_cast<int>(m_data.size() * cellPixel));
		for (iADissimilarityMatrixType::size_type x = 0; x < m_data.size(); ++x)
		{
			for (iADissimilarityMatrixType::size_type y = 0; y < m_data[x].size(); ++y)
			{
				QRect cellRect(matrixRect.left() + static_cast<int>(x * cellPixel),
					matrixRect.top() + static_cast<int>(y * cellPixel),
					cellPixel, cellPixel);
				double value = m_data[static_cast<int>(m_sortOrder[x])][static_cast<int>(m_sortOrder[y])].avgDissim[m_dataIdx];
				QColor color = m_lut.getQColor(value);
				p.fillRect(cellRect, color);
			}
		}

		// showAxes currently buggy - crashes!
		if (m_showAxes)
		{
			//p.drawText(axisSize, axisPadding, matrixRect.width(), textHeight, m_paramName);
			// draw x axis line:
			int outTickDist = outerAxisPadding + textHeight;
			int axisPos = outTickDist + axisTickHeight;
			p.drawLine(axisSize, axisPos, axisSize + matrixRect.width(), axisPos);
			// draw y axis line:
			p.drawLine(axisPos, axisSize, axisPos, axisSize + matrixRect.height());

			const size_t NoRangeStart = std::numeric_limits<size_t>::max();
			const double NoValue = std::numeric_limits<double>::infinity();
			double prevValue = NoValue;
			size_t rangeStart = NoRangeStart;
			auto const& pv = m_paramValues[m_sortParam];
			for (size_t i = 0; i <= m_sortOrder.size(); ++i)
			{
				double curValue = i < m_sortOrder.size() ? pv[m_sortOrder[i]] : NoValue;
				if (curValue != prevValue)
				{
					bool rangeStarts = (i < m_sortOrder.size() - 1) && pv[m_sortOrder[i]] == pv[m_sortOrder[i + 1]];
					bool rangeEnds = rangeStart != NoRangeStart;
					if (rangeEnds)
					{    // does a previously started range end here? if yes, draw label and end tick
						drawAxisText(p, axisSize + static_cast<int>(rangeStart) * cellPixel, static_cast<int>(i - rangeStart) * cellPixel, textHeight, outerAxisPadding, prevValue);
						drawAxisTick(p, axisPos, outTickDist, axisSize + (static_cast<int>(i) * cellPixel));
					}
					if (!rangeStarts && i < m_sortOrder.size())
					{   // draw tick and label for single cell not belonging to range
						drawAxisText(p, axisSize + (static_cast<int>(i) * cellPixel), cellPixel, textHeight, outerAxisPadding, curValue);
						drawAxisTick(p, axisPos, outTickDist, axisSize + static_cast<int>((i + 0.5) * cellPixel));
						rangeStart = NoRangeStart;
					}
					if (rangeStarts)
					{
						rangeStart = i;
						if (!rangeEnds)
						{
							drawAxisTick(p, axisPos, outTickDist, axisSize + (static_cast<int>(rangeStart) * cellPixel));
						}
					}
					prevValue = m_paramValues[m_sortParam][m_sortOrder[i]];
				}
			}
		}

		if (m_showScalarBar)
		{
			// Draw scalar bar (duplicated from iAQSplom!)
			QPoint topLeft(geometry().width() - (scalarBarPadding + scalarBarWidth), scalarBarPadding);

			QRect colorBarRect(topLeft.x(), topLeft.y(),
				scalarBarWidth, height() - 2 * scalarBarPadding);
			QLinearGradient grad(topLeft.x(), topLeft.y(), topLeft.x(), topLeft.y() + colorBarRect.height());
			QMap<double, QColor>::iterator it;
			for (size_t i = 0; i < m_lut.numberOfValues(); ++i)
			{
				double rgba[4];
				m_lut.getTableValue(i, rgba);
				QColor color(rgba[0] * 255, rgba[1] * 255, rgba[2] * 255, rgba[3] * 255);
				double key = 1 - (static_cast<double>(i) / (m_lut.numberOfValues() - 1));
				grad.setColorAt(key, color);
			}
			p.fillRect(colorBarRect, grad);
			p.drawRect(colorBarRect);
			// Draw color bar / name of parameter used for coloring
			int colorBarTextX = topLeft.x() - (textWidth + scalarBarPadding);
			p.drawText(colorBarTextX, topLeft.y() + fm.height(), maxStr);
			p.drawText(colorBarTextX, height() - (fm.height() + scalarBarPadding), minStr);
		}
	}
	iADissimilarityMatrixType const& m_data;
	iAMatrixValuesType const& m_paramValues;
	int m_sortParam;
	int m_dataIdx;
	iALookupTable m_lut;
	double m_range[2];
	std::vector<size_t> m_sortOrder;
	bool m_showScalarBar, m_showAxes;
};

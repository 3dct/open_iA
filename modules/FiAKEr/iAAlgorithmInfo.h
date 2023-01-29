// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QColor>
#include <QWidget>

class QMouseEvent;
class QPainter;


class iAAlgorithmInfo : public QWidget
{
	Q_OBJECT
public:
	using iAMatrixType = QVector<QVector<double>>;
	enum DisplayMode
	{
		Box,
		Matrix,
		DefaultDisplayMode = Matrix
	};

	iAAlgorithmInfo(QString const& name, QStringList const& inNames, QStringList const& outNames, QColor const& inColor,
		QColor const& outColor);
	void updateMatrixMax();
	int boxHeight() const;
	int oneEntryHeight() const;
	void setSelectedInput(int inIdx);
	void addShownOut(int outIdx);
	void removeShownOut(int outIdx);
	void setInSortOrder(QVector<int> const& inSortOrder);
	void setMode(int mode);
	void setNormalizePerOutput(bool maxPerOut);
	void setShowArrows(bool showArrows);
	void setShowHighlight(bool showHighlight);
	void setMergeHighlight(bool mergeHighlight);
	void setMatrix(iAMatrixType const& matrix);
	void setInOutColor(QColor const& inColor, QColor const& outColor);
	void setLegendLineWidth(int lineWidth);

private:
	void drawInOut(QPainter& p, QRect textRect, QString const& text, QVector<QRect>& rects, QColor const& color,
		bool selected, bool useColor, bool verticalText);
	int connectorWidth(QFontMetrics fm, QStringList const& strings) const;
	void drawConnectors(QPainter& p, int left, int width, QStringList const& strings, QVector<QRect>& rects,
		QColor const& color, int selected, QVector<int> const& shown, QVector<int> const& sort, QVector<QPoint>& posOut,
		bool isLeft, int connHeight);
	void drawBoxLinks(QPainter& p, QVector<QPoint> inPt, QVector<QPoint> outPt, QRect const& algoBox);
	void drawLegend(QPainter& p, int leftWidth, bool top);
	void paintEvent(QPaintEvent* ev) override;
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	QSize sizeHint() const override;
signals:
	void inputClicked(int inIdx);
	void outputClicked(int outIdx);
private:
	double mapVal(int outIdx, double val);

	QString m_name;
	QStringList m_inNames, m_outNames;
	QVector<QRect> m_inRects, m_outRects;
	QColor m_inColor, m_outColor;
	int m_selectedIn = -1;
	QVector<int> m_shownOut;
	QVector<int> m_inSort;

	iAMatrixType m_matrix;
	std::vector<double> m_maxPerColumn;
	double m_maxTotal = 1.0;
	// some widths as determined during painting:
	int m_inWidth = 1,
		m_outWidth = 1,
		m_boxMinWidth = 1,
		m_legendWidth = 1,
		m_legendLineWidth;
	DisplayMode m_displayMode;
	bool m_normalizePerOutput = false,
		m_showArrows = false,
		m_showHighlight = false,
		m_mergeHighlight = true;
	QRect m_matrixRect;
};

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iASignallingWidget.h>

#include <vector>

class iABarData;
class iABarWidget;
class iABarsWidget;
class iAColorTheme;

class QGridLayout;
class QMenu;

class iAStackedBarChart: public iASignallingWidget
{
	Q_OBJECT
public:
	static const int MaxBarHeight;
	static const int TextPadding;
	iAStackedBarChart(iAColorTheme const* theme, QGridLayout* gL, int row, int col,
		bool header = false, bool last = false);
	void addBar(QString const& name, double value, double maxValue, double minValDiff);
	void updateBar(QString const& name, double value, double maxValue, double minValDiff);
	int removeBar(QString const & name);
	int barIndex(QString const& name) const;
	void setColorTheme(iAColorTheme const * theme);
	QMenu* contextMenu();
	size_t numberOfBars() const;
	double weightedSum() const;
	double barValue(int barIdx) const;
	void setSelectedBar(int barIdx);
	QString barName(size_t barIdx) const;
	void setLeftMargin(int leftMargin);
	void setPos(int row, int col);
signals:
	void switchedStackMode(bool mode);
	void weightsChanged(std::vector<double> const& weights);
	void clicked();
	void barClicked(size_t barID);
	void barDblClicked(size_t barID);
	void normalizeModeChanged(bool normalizePerBar);
public slots:
	void setWeights(std::vector<double> const & weights);
	void setNormalizeMode(bool normalizePerBar);
	void setDoStack(bool doStack);
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
	void updateBars();
	size_t getBarAt(int x) const;

	size_t dividerWithinRange(int x) const;
	double weightAndNormalize(iABarData const& bar) const;
	int barWidth(iABarData const& bar) const;
	void normalizeWeights();
	void updateOverallMax();
	void updateColumnStretch();
	void updateDividers();
	void updateLayout();
	void deleteBar(int barID);

	void emitBarClick(size_t barID);
	void emitBarDblClick(size_t barID);

	std::vector<std::shared_ptr<iABarData>> m_bars;
	std::vector<int> m_dividers;
	iAColorTheme const * m_theme;
	QMenu* m_contextMenu;
	bool m_header, m_stack, m_last;
	size_t m_resizeBar;
	int m_resizeStartX;
	double m_resizeWidth;
	std::vector<std::shared_ptr<iABarData>> m_resizeBars;
	bool m_normalizePerBar;
	double m_overallMaxValue;
	int m_selectedBar;
	int m_leftMargin;
	QGridLayout* m_gL;
	int m_row, m_col;
	int m_chartAreaPixelWidth;

	iABarsWidget* m_barsWidget;
	QVector<iABarWidget*> m_barWidgets;

	friend class iABarsWidget;
	friend class iABarWidget;
};

class QGridLayout;

void addHeaderLabel(QGridLayout* layout, int column, QString const& text, QSizePolicy::Policy horPolicy);

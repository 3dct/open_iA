// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAElementSelectionListener.h"

#include <QMap>
#include <QWidget>

#include <utility> // for std::pair

class QColor;
class QPainter;
class QPaintEvent;

class iAPeriodicTableWidget: public QWidget
{
	Q_OBJECT
public:
	iAPeriodicTableWidget(QWidget *parent);
	void setConcentration(QString const & elementName, double percentage, QColor const & color);
	void setListener(std::shared_ptr<iAElementSelectionListener> listener);
	int GetCurrentElement() const;
protected:
	void paintEvent(QPaintEvent *) override;
	void mouseMoveEvent(QMouseEvent *) override;
private:
	void drawElement(QPainter& painter, QPoint const & elemPos, int elemWidth, int elemHeight, int elemIdx);
	QColor getPenColor(QPainter const & painter);
	int getElementFromMousePos(QPoint pos);

	QMap<QString, std::pair<double, QColor> > m_concentration;
	QVector<std::pair<QPoint, int> > m_elementPositions;
	int m_elementWidth, m_elementHeight;
	std::shared_ptr<iAElementSelectionListener> m_listener;
	int m_currentElemIdx;
};

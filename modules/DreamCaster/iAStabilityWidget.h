// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

class MouseEvent;

//! Class, inherited from QWidget, representing stability by 3 axes.
class iAStabilityWidget : public QWidget
{
	Q_OBJECT

public:
	iAStabilityWidget(QWidget *parent);
	~iAStabilityWidget();
protected:
	void paintEvent(QPaintEvent *event) override;
	void mouseReleaseEvent ( QMouseEvent * event ) override;
public:
	unsigned int countX() const { return m_countX; }
	unsigned int countY() const { return m_countY; }
	unsigned int countZ() const { return m_countZ; }
	void SetCount(int count);

	QColor **m_colsXY, m_colArrowX, m_colArrowY, m_colArrowZ;
private:
	unsigned int m_countX, m_countY, m_countZ;
	float m_pix_size;
	float m_stepPixSize;
	float m_spanAngleZ;
	QWidget * m_parent;
signals:
	void mouseReleaseEventSignal();
};

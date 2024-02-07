// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

//! Widget having a QPixmap pointer, drawing on widget is implemented by drawing on this pixmap.
//! paintEvent is redefined to draw data contained in pixmap on widget.
//! In this way, it is possible to draw on this widget outside paintEvent event.
class iAPaintWidget final : public QWidget
{
	Q_OBJECT

public:
	iAPaintWidget(QPixmap *a_pxmp, QWidget *parent);
	~iAPaintWidget();
	QPixmap * GetPixmap(){ return m_pxmp; }
	void SetPixmap(QPixmap *pxmp){ m_pxmp = pxmp; }
	void SetHiglightedIndices(int *inds_x, int *inds_y, unsigned int count);
	void SetHighlightStyle(const QColor &color, float penWidth);
	void RemoveHighlights();
protected:
	void paintEvent(QPaintEvent *event) override;
	void mouseReleaseEvent ( QMouseEvent * event ) override;
	void mousePressEvent ( QMouseEvent * event ) override;
	void mouseMoveEvent ( QMouseEvent * event ) override;
public:
	int lastX, lastY;
	int lastMoveX, lastMoveY;
	//int lastMoveX, lastMoveY;
	double scaleCoef;
	//int ratio;
signals:
	void mouseReleaseEventSignal();
	void mouseReleaseEventSignal(int x, int y);
	void mouseMoveEventSignal();
	void mousePressEventSignal();
	void ChangedSignal(double & scale, double & offsetX, double & offsetY);
public slots:
	void UpdateSlot(double & scale, double & offsetX, double & offsetY);

private:
	void checkOffset();
	void checkScale();
	QPixmap *m_pxmp;
	double m_scale;
	double m_offset[2];
	float m_maxScale;
	int *m_highlightX;
	int *m_highlightY;
	int highlightCount;
	float highlightPenWidth;
	QColor highlightColor;
	int m_lastMoveX, m_lastMoveY;
};

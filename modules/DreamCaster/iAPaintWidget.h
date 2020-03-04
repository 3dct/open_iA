/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

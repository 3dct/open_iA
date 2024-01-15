// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QVector>
#include <QWidget>

class QPaintEvent;

struct iAPiePiece
{
	iAPiePiece();
	iAPiePiece(QString const & name, double percentage, QColor & color);
	QString name;
	double percentage;
	QColor color;
};

class iAPieChartWidget: public QWidget
{
	Q_OBJECT
public:
	iAPieChartWidget(QWidget *parent);
	void clearPieces();
	void addPiece(QString name, double percentage, QColor color);
	bool empty() const;
protected:
	void paintEvent(QPaintEvent *);
private:
	QVector<iAPiePiece> m_pieces;
};

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QLabel>

class iAClickableLabel: public QLabel
{
	Q_OBJECT
public:
	iAClickableLabel(QString const& text, bool vertical);
signals:
	void dblClicked();
	void clicked();
private:
	void mouseDoubleClickEvent(QMouseEvent* ev) override;
	void mouseReleaseEvent(QMouseEvent* ev) override;
	void paintEvent(QPaintEvent*) override;
	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

	bool m_vertical;
};

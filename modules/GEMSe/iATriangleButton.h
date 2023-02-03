// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

class iATriangleButton: public QWidget
{
	Q_OBJECT
public:
	iATriangleButton();
	bool Toggle();
	bool IsExpanded() const;
signals:
	void clicked();
protected:
	virtual void paintEvent(QPaintEvent* ev);
	virtual void mouseReleaseEvent(QMouseEvent * ev);
private:
	bool m_expanded;
};

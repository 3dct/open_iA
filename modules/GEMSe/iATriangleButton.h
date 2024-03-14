// Copyright (c) open_iA contributors
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
	void paintEvent(QPaintEvent* ev) override;
	void mouseReleaseEvent(QMouseEvent * ev) override;
private:
	bool m_expanded;
};

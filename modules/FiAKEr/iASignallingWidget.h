// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

//! A widget that emits signals on click and double click.
class iASignallingWidget: public QWidget
{
	Q_OBJECT
signals:
	void dblClicked();
	void clicked(Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
private:
	void mouseDoubleClickEvent(QMouseEvent* ev) override;
	void mouseReleaseEvent(QMouseEvent* ev) override;
	void paintEvent(QPaintEvent* ev) override;
};

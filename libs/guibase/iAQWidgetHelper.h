// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

template <class LayoutType>
LayoutType* createLayout(int spacing = 4)
{
	auto layout = new LayoutType();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(spacing);
	return layout;
}

template <class LayoutType>
QWidget* createLayoutWidget(int spacing = 4)
{
	auto w = new QWidget();
	w->setLayout(createLayout<LayoutType>(spacing));
	return w;
}

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

template <class LayoutType>
LayoutType* createLayout(int spacing = 4, int margin = 0)
{
	auto layout = new LayoutType();
	layout->setContentsMargins(margin, margin, margin, margin);
	layout->setSpacing(spacing);
	return layout;
}

template <class LayoutType>
QWidget* createLayoutWidget(int spacing = 4, int margin = 0)
{
	auto w = new QWidget();
	w->setLayout(createLayout<LayoutType>(spacing, margin));
	return w;
}

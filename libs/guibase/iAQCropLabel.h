// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QScrollArea>

class QLabel;

//! Alternative for a QLabel which automatically cuts off text exceeding its width.
//! This widget, in contrast to a QLabel, is resizable to widths smaller than the required with to display its full text.
//! It does not impose any minimum width requirements on parent widgets, instead, any text text exceeding the width will
//! just be invisible.
class iAguibase_API iAQCropLabel : public QScrollArea
{
public:
	iAQCropLabel();
	iAQCropLabel(QString const& text, QString const& qssClass = QString());
public slots:
	void setText(QString const& text);
private:
	void setup(QString const& text = "");
	QLabel* m_label;
};

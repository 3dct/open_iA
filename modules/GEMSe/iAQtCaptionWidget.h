// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAGEMSeConstants.h"

#include <QWidget>

class iAQtCaptionWidget : public QWidget
{
public:
	iAQtCaptionWidget(QWidget* parent, QString const & name);
private:
	void paintEvent(QPaintEvent* ev) override;
	QString m_name;
	int m_height;
};

void SetCaptionedContent(QWidget* parent, QString const & caption, QWidget* w);

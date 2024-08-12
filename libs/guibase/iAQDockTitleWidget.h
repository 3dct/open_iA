// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iaguibase_export.h>

#include <QFrame>

class QDockWidget;

//! Title widget for a QDockWidget which also shows an info button.
class iAguibase_API iAQDockTitleWidget : public QFrame
{
	Q_OBJECT
public:
	iAQDockTitleWidget(QDockWidget* parent, QString infoLink);
	QSize sizeHint() const override;
};

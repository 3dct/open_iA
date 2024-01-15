// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Ui
#include "ui_iA4DCTToolsDockWidget.h"
// QT
#include <QDockWidget>

class iA4DCTToolsDockWidget : public QDockWidget, public Ui::ToolsDockWidget
{
	Q_OBJECT
public:
	iA4DCTToolsDockWidget( QWidget * parent );
	~iA4DCTToolsDockWidget( );
};

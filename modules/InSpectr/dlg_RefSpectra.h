// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_RefSpectra.h"

#include <iAQTtoUIConnector.h>

typedef iAQTtoUIConnector<QDockWidget, Ui_RefSpectra> dlg_RefSpectraContainer;

class dlg_RefSpectra: public dlg_RefSpectraContainer
{
	Q_OBJECT
public:
	dlg_RefSpectra(QWidget *parent): dlg_RefSpectraContainer(parent) {}
	QListView * getSpectraList() { return refSpectraListView; }
};

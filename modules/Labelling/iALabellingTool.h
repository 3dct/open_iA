// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Labelling_export.h"

#include <iATool.h>

#include <QObject>

class iALabelsDlg;

class iALabellingTool : public QObject, public iATool
{
	Q_OBJECT
public:
	iALabellingTool(iAMainWindow* mainWnd, iAMdiChild* child);
	static const QString Name;
	iALabelsDlg* labelsDlg();

private:
	iALabelsDlg* m_dlgLabels;
};

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "labelling_export.h"

#include <iATool.h>

#include <QObject>

class iALabelsDlg;

class Labelling_API iALabellingTool : public QObject, public iATool
{
	Q_OBJECT
public:
	iALabellingTool(iAMainWindow* mainWnd, iAMdiChild* child);
	void loadState(QSettings& projectFile, QString const& fileName) override;
	void saveState(QSettings& projectFile, QString const& fileName) override;
	static const QString Name;
	iALabelsDlg* labelsDlg();

private:
	iALabelsDlg* m_dlgLabels;
};

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <QObject>
#include <QString>

#include <memory>

class iAOpenXRTrackerServerToolImpl;

class iAOpenXRTrackerServerTool : public QObject, public iATool
{
public:
	static const QString Name;
	iAOpenXRTrackerServerTool(iAMainWindow* mainWnd, iAMdiChild* child);
	~iAOpenXRTrackerServerTool();
private:
	std::unique_ptr<iAOpenXRTrackerServerToolImpl> m_impl;
};

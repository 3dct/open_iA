// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <QObject>
#include <QString>

#include <memory>

class iAWSSToolImpl;

class iAWebSocketServerTool : public QObject, public iATool
{
public:
	static const QString Name;
	iAWebSocketServerTool(iAMainWindow* mainWnd, iAMdiChild* child);
	~iAWebSocketServerTool();
private:
	std::unique_ptr<iAWSSToolImpl> m_impl;
};

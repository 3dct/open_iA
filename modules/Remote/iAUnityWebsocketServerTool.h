// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <QObject>
#include <QString>

#include <memory>

class iAUnityWebsocketServerToolImpl;

class iAUnityWebsocketServerTool : public QObject, public iATool
{
public:
	static const QString Name;
	iAUnityWebsocketServerTool(iAMainWindow* mainWnd, iAMdiChild* child);
	~iAUnityWebsocketServerTool();
private:
	std::unique_ptr<iAUnityWebsocketServerToolImpl> m_impl;
};

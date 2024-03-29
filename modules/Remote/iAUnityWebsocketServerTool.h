// Copyright (c) open_iA contributors
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
	static std::shared_ptr<iATool> create(iAMainWindow* mainWnd, iAMdiChild* child);
	iAUnityWebsocketServerTool(iAMainWindow* mainWnd, iAMdiChild* child);
	~iAUnityWebsocketServerTool();
private:
	std::unique_ptr<iAUnityWebsocketServerToolImpl> m_impl;
};

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QMetaType>

class iARemoteAction: public QObject
{

Q_OBJECT


public:

	enum mouseAction
	{
		ButtonDown,
		ButtonUp,
		StartMouseWheel,
		MouseWheel,
		EndMouseWheel,
		Unknown
	};

	mouseAction action;
	float x;
	float y;
	float spinY;
	bool altKey;
	bool buttonLeft;
	bool buttonRight;
	bool buttonMiddle;
	bool shiftKey;
	bool ctrlKey;
	bool metaKey;
	QString viewID;
};
Q_DECLARE_METATYPE(iARemoteAction*);

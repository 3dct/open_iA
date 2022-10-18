#pragma once

#include <QObject>
#include <QMetaType>


class iARemoteAction: public QObject
{

Q_OBJECT


public:

	enum mouseAction
	{
		down,
		up,
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

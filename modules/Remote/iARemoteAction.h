#pragma once

#include <QObject>


class iARemoteAction: public QObject
{

Q_OBJECT

public:

	enum mouseAction
	{
		down,
		up
	};

	mouseAction action;
	float x;
	float y;
	bool altKey;
	bool buttonLeft;
	bool buttonRight;
	bool buttonMiddle;
	bool shiftKey;
	bool ctrlKey;
	bool metaKey;
	QString viewID;
};


// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "labelling_export.h"

#include <QObject>
#include <QColor>
#include <QSharedPointer>

struct iALabel
{
	iALabel() : id(-1) {};
	iALabel(int i, QString n, QColor c) :
		id(i), name(n), color(c)
	{}
	int id;
	QString name;
	QColor color;
};

struct iASeed
{
	iASeed() : x(-1), y(-1), z(-1), overlayImageId(-1) {};
	iASeed(int X, int Y, int Z, int oiid, QSharedPointer<iALabel> l) :
		x(X), y(Y), z(Z), overlayImageId(oiid), label(l)
	{}
	int x;
	int y;
	int z;
	int overlayImageId;
	QSharedPointer<iALabel> label;
};

inline bool operator==(const iASeed& i1, const iASeed& i2)
{
	return i1.x == i2.x && i1.y == i2.y && i1.z == i2.z && i1.overlayImageId == i2.overlayImageId;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
inline uint qHash(const iASeed& key, uint seed)
#else
inline size_t qHash(const iASeed& key, size_t seed)
#endif
{
	return qHash(key.x ^ key.y ^ key.z ^ key.overlayImageId, seed);
}

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "labelling_export.h"

#include <QObject>
#include <QColor>

#include <memory>

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
	iASeed(int X, int Y, int Z, int oiid, std::shared_ptr<iALabel> l) :
		x(X), y(Y), z(Z), overlayImageId(oiid), label(l)
	{}
	int x;
	int y;
	int z;
	int overlayImageId;
	std::shared_ptr<iALabel> label;
};

inline bool operator==(const iASeed& i1, const iASeed& i2)
{
	return i1.x == i2.x && i1.y == i2.y && i1.z == i2.z && i1.overlayImageId == i2.overlayImageId;
}

inline size_t qHash(const iASeed& key, size_t seed)
{
	return qHash(key.x ^ key.y ^ key.z ^ key.overlayImageId, seed);
}

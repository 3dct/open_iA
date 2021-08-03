/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include <QColor>
#include <QList>
#include <QString>

struct iANModalSeed
{
	friend class iANModalController;

	iANModalSeed(int X, int Y, int Z, int oiid) : x(X), y(Y), z(Z), overlayImageId(oiid), labelId(-1), scalar(-1)
	{
	}
	//iANModalSeed(int X, int Y, int Z, int oiid, int lid, double s)
	//	: x(X), y(Y), z(Z), overlayImageId(oiid), labelId(lid), scalar(s)
	//{}
	int x;
	int y;
	int z;
	int overlayImageId;

	struct Hasher
	{
		std::size_t operator()(const iANModalSeed& key) const
		{
			return qHash(key.x ^ key.y ^ key.z ^ key.overlayImageId);
		}
	};

	struct Comparator
	{
		bool operator()(const iANModalSeed& i1, const iANModalSeed& i2) const
		{
			return i1.x == i2.x && i1.y == i2.y && i1.z == i2.z && i1.overlayImageId == i2.overlayImageId;
		}
	};

private:
	int labelId;
	double scalar;
};

inline bool operator==(const iANModalSeed& i1, const iANModalSeed& i2)
{
	return i1.x == i2.x && i1.y == i2.y && i1.z == i2.z && i1.overlayImageId == i2.overlayImageId;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
inline uint qHash(const iANModalSeed& key, uint seed)
#else
inline size_t qHash(const iANModalSeed& key, size_t seed)
#endif
{
	return qHash(key.x ^ key.y ^ key.z ^ key.overlayImageId, seed);
}

struct iANModalLabel
{
	iANModalLabel() : id(-1), opacity(0.0f)
	{
	}
	iANModalLabel(int i, QString n, QColor c, float o) : id(i), name(n), color(c), opacity(o)
	{
	}
	int id;
	QString name;
	QColor color;
	float opacity;

	bool null()
	{
		return id == -1;
	}
};

inline bool operator==(const iANModalLabel& i1, const iANModalLabel& i2)
{
	return i1.id == i2.id;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
inline uint qHash(const iANModalLabel& key, uint seed)
#else
inline size_t qHash(const iANModalLabel& key, size_t seed)
#endif
{
	return qHash(key.id, seed);
}
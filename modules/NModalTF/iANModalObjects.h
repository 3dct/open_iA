/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <QString>
#include <QColor>
#include <QList>

struct iANModalSeed {
	iANModalSeed(int X, int Y, int Z, int oiid)//, double s)
		: x(X), y(Y), z(Z), overlayImageId(oiid)//, scalar(s)
	{}
	int x;
	int y;
	int z;
	int overlayImageId;
};

inline bool operator==(const iANModalSeed& i1, const iANModalSeed& i2)
{
	return i1.x == i2.x && i1.y == i2.y && i1.z == i2.z && i1.overlayImageId == i2.overlayImageId;
}

inline uint qHash(const iANModalSeed& key, uint seed)
{
	return qHash(key.x ^ key.y ^ key.z ^ key.overlayImageId, seed);
}

struct iANModalLabel {
	iANModalLabel() :
		id(-1), opacity(0.0f), remover(false)
	{}
	iANModalLabel(int i, QString n, QColor c)
		: id(i), name(n), color(c), opacity(1.0f), remover(false)
	{}
	int id;
	QString name;
	QColor color;
	float opacity;
	bool remover;
};
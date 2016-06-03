/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iA4DCTDefects.h"
// Qt
#include <QFile>
#include <QTextStream>

bool iA4DCTDefects::save(VectorDataType defects, QString path)
{
	QFile file(path);
	QTextStream out(&file);
	if (file.open(QIODevice::WriteOnly)) {
		for (auto d : defects) {
			out << std::to_string(d).c_str() << '\n';
		}
		return true;
	}
	return false;
}

iA4DCTDefects::VectorDataType iA4DCTDefects::load(QString path)
{
	VectorDataType list;
	// open file
	QFile file(path);
	QTextStream stream(&file);
	// open and read file
	if (file.open(QIODevice::ReadOnly)) {
		QString line = stream.readLine();
		while (!line.isNull()) {
			list.push_back(line.toULong());
			line = stream.readLine();
		}
	}	
	return list;
}

iA4DCTDefects::HashDataType iA4DCTDefects::DefectDataToHash(VectorDataType defects)
{
	HashDataType hash;
	for (auto d : defects) {
		hash.insert(d, true);
	}
	return hash;
}

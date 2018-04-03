/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
 
#include "pch.h"
#include "iACharacteristicEnergy.h"

#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QRegExp>
#include <QStringList>

#include <cassert>

const double iACharacteristicEnergy::NotAvailable = -1;


iACharacteristicEnergy EnergyLoader::ConstructElement(QString line)
{
	iACharacteristicEnergy element;
	
	QRegExp splitter("\\s+");
	QStringList stringList = line.split(splitter);
	assert(stringList.size() > 2);
	if (stringList.size() < 2)
		return element;
	
	element.symbol = stringList[1];
	for (int i=2; i<stringList.size(); ++i)
	{
		element.energies.push_back(stringList[i].toDouble());
	}
	return element;
}

void EnergyLoader::Load(QString const & fileName, QVector<iACharacteristicEnergy>& output)
{
	if(!QFileInfo(fileName).exists())
		return;

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	while (!file.atEnd())
	{
		QString line = file.readLine().trimmed();
		if (line.length() > 0 && line[0].isDigit())
		{
			iACharacteristicEnergy element = EnergyLoader::ConstructElement(line);
			output.push_back(element);
		}
	}
	file.close(); //cleanup
}

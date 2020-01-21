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
#include "iALoader.h"

#include <iAConsole.h>

#include <QTextStream>

#include <sstream>
#include <fstream>

bool iALoader::loadCSV(const QString &Fname)
{
	if (Fname.isNull() || Fname.isEmpty())
	{
		return false;
	}
	std::ifstream inputF(Fname.toStdString(), std::ios::in);
	std::string line;
	std::getline(inputF, line);
	if (!m_greyValues.empty())
	{
		m_greyValues.clear();
	}
	if (!m_frequencies.empty())
	{
		m_frequencies.clear();
	}

	if (inputF.is_open())
	{
		DEBUG_LOG("Loading file");

		while (std::getline(inputF, line))
		{
			double x = 0; double y = 0;
			std::istringstream ss(line);
			ss >> x; ss >> y;
			DEBUG_LOG(QString("x y %1 %2").arg(x).arg(y));
			m_greyValues.push_back(x);
			m_frequencies.push_back(y);

		}

		inputF.close();
		return true;
	}
	else
	{
		return false;
	}
}

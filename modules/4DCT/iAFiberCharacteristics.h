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

#include <io/iAFileUtils.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class Fiber;
typedef std::vector<Fiber> FibersData;

class Fiber {
public:
	unsigned int	id;
	double			startPoint[3];
	double			endPoint[3];
	double			straightLength;
	double			curvedLength;
	double			diameter;
	double			surfaceArea;
	double			volume;
	bool			isSeparated;
	bool			isCurved;

	static FibersData ReadFromCSV(QString const & fileName, double spacing)
	{
		std::vector<Fiber> result;

		const int numRows = 14;
		const int skipRows = 5;
		std::ifstream fileStream( getLocalEncodingFileName(fileName) );
		std::string line;

		// row skipping
		for (int i = 0; i < skipRows; i++)
		{
			std::getline(fileStream, line);
		}

		// reading
		while (std::getline(fileStream, line))
		{
			std::stringstream stringStream(line);
			std::string cell[numRows];
			int readedRows = 0;
			for (int i = 0; i < numRows; i++)
			{
				if (std::getline(stringStream, cell[i], ','))
				{
					readedRows++;
				}
			}
			if (readedRows != numRows)
			{
				continue;
			}

			Fiber fiber;
			fiber.id = atoi(cell[0].c_str());
			fiber.startPoint[0] = atof(cell[1].c_str()) / spacing;
			fiber.startPoint[1] = atof(cell[2].c_str()) / spacing;
			fiber.startPoint[2] = atof(cell[3].c_str()) / spacing;
			fiber.endPoint[0] = atof(cell[4].c_str()) / spacing;
			fiber.endPoint[1] = atof(cell[5].c_str()) / spacing;
			fiber.endPoint[2] = atof(cell[6].c_str()) / spacing;
			fiber.straightLength = atof(cell[7].c_str()) / spacing;
			fiber.curvedLength = atof(cell[8].c_str()) / spacing;
			fiber.diameter = atof(cell[9].c_str()) / spacing;
			fiber.surfaceArea = atof(cell[10].c_str()) / (spacing * spacing);
			fiber.volume = atof(cell[11].c_str()) / (spacing * spacing * spacing);
			fiber.isSeparated = (atoi(cell[12].c_str()) == 1);
			fiber.isCurved = (atoi(cell[13].c_str()) == 1);

			result.push_back(fiber);
		}

		return result;
	}

	bool operator<(const Fiber& rhs)
	{
		return (this->curvedLength < rhs.curvedLength);
	}
};

/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
 
#ifndef IAFIBERCHARACTERISTICS_H
#define IAFIBERCHARACTERISTICS_H
// std
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct FiberCharacteristics {
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

	static std::vector<FiberCharacteristics> ReadFromCSV(std::string fileName, double spacing)
	{
		std::vector<FiberCharacteristics> result;

		const int numRows = 14;
		const int skipRows = 5;
		std::ifstream fileStream(fileName);
		std::string line;

		// row skipping
		for (int i = 0; i < skipRows; i++) {
			std::getline(fileStream, line);
		}

		double spacing2 = spacing * spacing;	// for surface conversion
		double spacing3 = spacing2 * spacing;	// for volume conversion

		// reading
		while (std::getline(fileStream, line)) {
			std::stringstream stringStream(line);
			std::string cell[numRows];
			int readedRows = 0;
			for (int i = 0; i < numRows; i++)
				if (std::getline(stringStream, cell[i], ',')) readedRows++;
			if (readedRows != numRows) continue;

			FiberCharacteristics fiber;
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
			fiber.surfaceArea = atof(cell[10].c_str()) / spacing2;
			fiber.volume = atof(cell[11].c_str()) / spacing3;
			fiber.isSeparated = (atoi(cell[12].c_str()) == 1);
			fiber.isCurved = (atoi(cell[13].c_str()) == 1);

			result.push_back(fiber);
		}

		return result;
	}
};

#endif // IAFIBERCHARACTERISTICS_H
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
#pragma once

#include "iAFeatureScoutObjectType.h"

#include <QMap>
#include <QString>
#include <QVector>

//! parameters for csv loading configuraton
struct iACsvConfig
{
	static const int LegacyFormatStartSkipLines = 5;
	iACsvConfig() :
		fileName(""),
		encoding("System"),
		skipLinesStart(LegacyFormatStartSkipLines),
		skipLinesEnd(0),
		colSeparator(";"),
		decimalSeparator("."),
		addAutoID(false),
		objectType(iAFeatureScoutObjectType::Voids),
		unit("microns"),
		spacing(0.0f),
		computeLength(false),
		computeAngles(false),
		computeTensors(false),
		containsHeader(true)
	{}

	QString fileName;                       //!< filename, not stored in registrys
	QString encoding;                       //!< text encoding of the csv file
	bool containsHeader;                    //!< whether the file contains a header
	size_t skipLinesStart, skipLinesEnd;    //!< how many lines to skip at start and end of the file
	QString colSeparator, decimalSeparator; //!< column and decimal separator strings
	bool addAutoID;                         //!< whether to add an automatic ID column
	iAFeatureScoutObjectType objectType;    //!< type of objects to be analyzed
	QString unit;                           //!< unit of measurement for the values given in the csv
	float spacing;                          //!< volume spacing to be used, currently unused

	QStringList currentHeaders;             //!< current headers of the table
	QStringList selectedHeaders;            //!< names of the selected headers

	enum MappedColumn {
		NotMapped=-1,
		StartX, StartY, StartZ,
		EndX, EndY, EndZ,
		CenterX, CenterY, CenterZ,
		Length,
		Diameter,
		Phi, Theta,
		MappedCount
	};
	bool computeLength, computeAngles, computeTensors;  //!< flags whether to compute additional columns
	QMap<uint, QString> columnMapping;
};

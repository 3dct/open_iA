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

#include "iAFeatureScoutObjectType.h"

#include "FeatureScout_export.h"

#include <QMap>
#include <QString>
#include <QVector>

class QSettings;

//! parameters for csv loading configuraton
struct FeatureScout_API iACsvConfig
{
	static const QString LegacyFiberFormat;
	static const QString LegacyVoidFormat;
	enum MappedColumn {                    //! "ID" for columns needed either in computation of other columns or later in FeatureScout
		NotMapped = -1,
		StartX, StartY, StartZ,
		EndX, EndY, EndZ,
		CenterX, CenterY, CenterZ,
		Length, Diameter,
		Phi, Theta,
		DimensionX, DimensionY, DimensionZ,
		MappedCount
	}; //!< must be the same order as dlg_CSVInput::m_mappingBoxes!
	enum VisualizationType
	{
		UseVolume,
		Lines,
		Cylinders,
		Ellipses,
		NoVis,
		VisTypeCount //must be last element
	}; //!< what visualization to use for the objects. Should match the entries of VisualizationTypeName iACsvConfig.cpp
	static const int LegacyFormatStartSkipLines = 5;
	iACsvConfig();
	bool isValid(QString & errorMsg) const;

	QString fileName;                       //!< filename, not stored in registrys
	QString curvedFiberFileName;            //!< filename for curved fiber information, also not stored in registry
	QString encoding;                       //!< text encoding of the csv file
	bool containsHeader;                    //!< whether the file contains a header
	size_t skipLinesStart, skipLinesEnd;    //!< how many lines to skip at start and end of the file
	QString columnSeparator;                //!< string separating the columns in the csv
	QString decimalSeparator;               //!< string separating the integer from the fractional part in the numbers
	bool addAutoID;                         //!< whether to add an automatic ID column
	iAFeatureScoutObjectType objectType;    //!< type of objects to be analyzed
	QString unit;                           //!< unit of measurement for the values given in the csv
	float spacing;                          //!< volume spacing to be used, currently unused
	QStringList currentHeaders;             //!< current headers of the table
	QStringList selectedHeaders;            //!< names of the selected headers
	bool computeLength, computeAngles, computeTensors, computeCenter, computeStartEnd;  //!< flags whether to compute additional columns
	VisualizationType visType;              //! how to visualize the given objects
	int cylinderQuality;                    //! how much sides are used for the cylinder visualization; the higher the number, the worse the quality (default=12)
	size_t segmentSkip;                     //! curved fiber optimization: if 1, all points along the fiber will be used; if larger, points will be skipped
	QMap<uint, uint> columnMapping;         //! map a specific value (denoted by an ID from MappedColumn) to the number of the column where it's stored
	double offset[3];                       //! offset to apply to all coordinates (start, end, center)
	bool isDiameterFixed;                   //! whether to insert a fixed diameter (given by fixedDiameterValue)
	double fixedDiameterValue;              //! value to use as diameter for all objects
	static iACsvConfig const & getLegacyFiberFormat(QString const & fileName);
	static iACsvConfig const & getLegacyPoreFormat(QString const & fileName);

	//! Return base key for a given format
	static QString getFormatKey(QString const & formatName);
	//! Return list of all csv configs stored in registry, list is empty if no format definitions exist
	static QStringList getListFromRegistry();

	//! Save this configuration under the given name in the given settings object
	void save(QSettings & settings, const QString & formatName);

	//! Load a given configuration name
	bool load(QSettings const & settings, const QString & formatName);
};

QString MapVisType2Str(iACsvConfig::VisualizationType visType);
iACsvConfig::VisualizationType MapStr2VisType(QString name);

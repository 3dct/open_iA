// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAColMap.h"
#include "iAObjectType.h"

#include "iaobjectvis_export.h"

#include <QString>
#include <QVector>

class QSettings;

//! Holds the configuration parameters for loading a specific csv file format.
struct iAobjectvis_API iACsvConfig
{
	static const QString FCPFiberFormat;
	static const QString FCVoidFormat;
	enum MappedColumn {                    //! "ID" for columns needed either in computation of other columns or later in FeatureScout
		NotMapped = -1,
		StartX, StartY, StartZ,
		EndX, EndY, EndZ,
		CenterX, CenterY, CenterZ,
		Length, Diameter,
		Phi, Theta,
		DimensionX, DimensionY, DimensionZ,
		CurvedLength,
		MappedCount
	}; //!< must be the same order as dlg_CSVInput::m_mappingBoxes!
	static const int FCPFormatStartSkipLines = 5;
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
	iAObjectType objectType;    //!< type of objects to be analyzed
	QString unit;                           //!< unit of measurement for the values given in the csv
	float spacing;                          //!< volume spacing to be used, currently unused
	QStringList currentHeaders;             //!< current headers of the table
	QStringList selectedHeaders;            //!< names of the selected headers
	bool computeLength, computeAngles, computeTensors, computeCenter, computeStartEnd;  //!< flags whether to compute additional columns
	iAObjectVisType visType;                //! how to visualize the given objects
	iAColMapT columnMapping;                //! map a specific value (denoted by an ID from MappedColumn) to the number of the column where it's stored
	double offset[3];                       //! offset to apply to all coordinates (start, end, center)
	bool isDiameterFixed;                   //! whether to insert a fixed diameter (given by fixedDiameterValue)
	double fixedDiameterValue;              //! value to use as diameter for all objects
	bool addClassID;                        //! whether to add class ID at the end. This setting is not stored, rather this is a use-case dependent setting
	static iACsvConfig const & getFCPFiberFormat(QString const & fileName);
	static iACsvConfig const & getFCVoidFormat(QString const & fileName);

	//! Return base key for a given format
	static QString getFormatKey(QString const & formatName);
	//! Return list of all csv configs stored in registry, list is empty if no format definitions exist
	static QStringList getListFromRegistry();

	//! Save this configuration under the given name in the given settings object
	void save(QSettings & settings, const QString & formatName);

	//! Load a given configuration name
	bool load(QSettings const & settings, const QString & formatName);
};

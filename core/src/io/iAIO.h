/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAAlgorithm.h"
#include "defines.h"          // for iAIOType
#include "open_iA_Core_export.h"
#include "iARawFileParameters.h"

#include <vtkSmartPointer.h>

#include <QDir>
#include <QString>
#include <QSharedPointer>

#include <vector>

class vtkCamera;
class vtkImageData;
class vtkPolyData;
class vtkStringArray;

class iAModalityList;

//! Class currently containing most IO operations (file reading and writing).
//! Should be split up into readers for specific formats!
class open_iA_Core_API iAIO : public iAAlgorithm
{
	Q_OBJECT
public:
	static const QString VolstackExtension;

	iAIO(vtkImageData* i, vtkPolyData* p, iALogger* logger, QWidget *parent = 0, std::vector<vtkSmartPointer<vtkImageData> > * volumes = 0, std::vector<QString> * fileNames = 0);
	iAIO(iALogger* logger, QWidget *parent, std::vector<vtkSmartPointer<vtkImageData> > * volumes, std::vector<QString> * fileNames = 0);//TODO: QNDH for XRF volume stack loading
	iAIO(QSharedPointer<iAModalityList> modalities, vtkCamera* cam, iALogger* logger);
	virtual ~iAIO();
	//! initialize variables
	void init(QWidget *par);
	//! Set up the file IO specified by the parameters.
	//! @param type type of the file to read
	//! @param fileName name of the file to read
	//! @param compression whether to use compression (if file format supports it)
	//! @param channel which channel to read/write (if file format supports more than one)
	//! @return true if successful, false if not.
	bool setupIO(iAIOType type, QString fileName, bool compression = false, int channel=-1);
	//! Set additional information for the current file
	void setAdditionalInfo(QString const & additionalInfo);
	//! Get additional information (if any, e.g. for a volume stack)
	QString const & additionalInfo();
	//! Get the name of the file that was read
	QString const & fileName();
	//! Get the list of modalities that were read
	QSharedPointer<iAModalityList> modalities();
	//! Get the type of file being read
	int ioID() const;

Q_SIGNALS:
	void done(bool active = false);
	void failed();

protected:
	void run() override;

private:
	bool setupRAWReader( QString const & f );
	bool setupPARSReader( QString const & f );
	bool setupVGIReader( QString const & f );
	bool setupStackReader( QString const & f );
	bool setupVolumeStackReader(QString const & f);
	bool setupVolumeStackMHDReader(QString const & f);
	bool setupVolumeStackVolstackReader(QString const & f);
	bool setupVolumeStackVolStackWriter(QString const & f);
	void fillFileNameArray(int * indexRange, int digitsInIndex, int stepSize = 1);

	void readImageStack();
	void readRawImage();
	void loadMetaImageFile(QString const & fileName);
	void readVTKFile();

	void readVolumeStack( );
	void readVolumeMHDStack( );
	void readImageData( );
	void readMetaImage( );
	void readSTL( );
	//! Reads a series of DICOM images.
	void readDCM( );
	//! Reads an NRRD image. See iAIOProvider.cpp why this is commented out
	//void readNRRD( );
	void readHDF5File();
	void readProject();

	void writeMetaImage(vtkSmartPointer<vtkImageData> imgToWrite, QString fileName);
	void writeVolumeStack();
	void writeSTL( );
	void writeImageStack( );
	void writeProject();

	void postImageReadActions();
	void printSTLFileInfos();
	void storeIOSettings();
	void loadIOSettings();

	QWidget *m_parent;
	QString m_fileName;
	QDir m_fileDir;

	QString m_extension;
	QString m_prefix;
	QString m_fileNamesBase;
	vtkStringArray* m_fileNameArray;
	bool m_compression;
	iARawFileParameters m_rawFileParams;

	int m_ioID;
	std::vector<vtkSmartPointer<vtkImageData> > * m_volumes;
	std::vector<QString> * m_fileNames_volstack;

	QString m_additionalInfo;
	int m_channel;

	QVector<QString> m_hdf5Path;
	QSharedPointer<iAModalityList> m_modalities;
	bool m_isITKHDF5;
	double m_hdf5Spacing[3];

	vtkCamera* m_camera;
};

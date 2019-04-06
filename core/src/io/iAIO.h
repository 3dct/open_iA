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

#include "iAAlgorithm.h"
#include "defines.h"          // for IOType
#include "open_iA_Core_export.h"

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

class open_iA_Core_API iAIO : public iAAlgorithm
{
	Q_OBJECT
public:
	static const QString VolstackExtension;

	iAIO(vtkImageData* i, vtkPolyData* p, iALogger* logger, QWidget *parent = 0, std::vector<vtkSmartPointer<vtkImageData> > * volumes = 0, std::vector<QString> * fileNames = 0);
	iAIO(iALogger* logger, QWidget *parent, std::vector<vtkSmartPointer<vtkImageData> > * volumes, std::vector<QString> * fileNames = 0);//TODO: QNDH for XRF volume stack loading
	iAIO(QSharedPointer<iAModalityList> modalities, vtkCamera* cam, iALogger* logger);
	virtual ~iAIO();
	void init(QWidget *par);
	bool setupIO( IOType type, QString f, bool comp = false, int channel=-1);
	void setAdditionalInfo(QString const & additionalInfo);
	QString additionalInfo();
	QString fileName();
	QSharedPointer<iAModalityList> modalities();
	int ioID() const;

Q_SIGNALS:
	void done(bool active = false);
	void failed();

protected:
	void run() override;

private:
	bool setupRAWReader( QString f );
	bool setupPARSReader( QString f );
	bool setupVGIReader( QString f );
	bool setupStackReader( QString f );
	bool setupVolumeStackReader(QString f);
	bool setupVolumeStackMHDReader(QString f);
	bool setupVolumeStackVolstackReader(QString f);
	bool setupVolumeStackVolStackWriter(QString f);
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
	void readDCM( );
	//void readNRRD( );	 // see iAIOProvider.cpp why this is commented out
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
	unsigned long m_headersize;
	int m_scalarType;
	int m_byteOrder;
	int m_extent[6];
	double m_spacing[3];
	double m_origin[3];
	bool m_compression;

	int m_rawSizeX, m_rawSizeY, m_rawSizeZ;
	double m_rawSpaceX, m_rawSpaceY, m_rawSpaceZ;
	double m_rawOriginX, m_rawOriginY, m_rawOriginZ;
	unsigned int m_rawHeaderSize, m_rawByteOrder, m_rawScalarType;

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

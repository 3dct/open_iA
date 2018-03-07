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
#pragma once

#include "iAAlgorithm.h"
#include "defines.h"          // for IOType
#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>

#include <QDir>
#include <QString>

#include <vector>

class vtkImageData;
class vtkMetaImageWriter;
class vtkPolyData;
class vtkSTLWriter;
class vtkStringArray;
class vtkTable;

class iAProgress;
class iASimReader;


class open_iA_Core_API iAIO : public iAAlgorithm
{
	Q_OBJECT
public:
	static const QString VolstackExtension;

	iAIO(vtkImageData* i, vtkPolyData* p, iALogger* logger, QWidget *parent = 0, std::vector<vtkSmartPointer<vtkImageData> > * volumes = 0, std::vector<QString> * fileNames = 0);
	iAIO(iALogger* logger, QWidget *parent, std::vector<vtkSmartPointer<vtkImageData> > * volumes, std::vector<QString> * fileNames = 0);//TODO: QNDH for XRF volume stack loading
	virtual ~iAIO();
	void init(QWidget *par);

	bool setupIO( IOType type, QString f, bool comp = false, int channel=-1);
	iAProgress* getProgressObserver() { return progressObserver; };
	void iosettingswriter();
	void iosettingsreader();

	void setAdditionalInfo(QString const & additionalInfo);
	QString getAdditionalInfo();

	QString getFileName();

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
	void FillFileNameArray(int * indexRange, int digitsInIndex);

	void readImageStack();
	void postImageReadActions();
	void readRawImage();
	void loadMetaImageFile(QString const & fileName);
	void readVolumeStack( );
	void readVolumeMHDStack( );
	void readImageData( );
	void readMetaImage( );
	void readSTL( );
	void readDCM( );
	void readNRRD( );

	void writeMetaImage(vtkSmartPointer<vtkImageData> imgToWrite, QString fileName);
	void writeVolumeStack();
	void writeSTL( );
	void writeImageStack( );

	void printSTLFileInfos();

	vtkMetaImageWriter *metaImageWriter;
	
	iAProgress* progressObserver;

	QWidget *parent;
	QString fileName;
	QDir f_dir; 

	QString extension;
	QString prefix;
	QString fileNamesBase;
	vtkStringArray* fileNameArray;
	unsigned long headersize;
	int scalarType;
	int byteOrder;
	int extent[6];
	double spacing[3];
	double origin[3];
	bool compression;
	
	int rawSizeX,rawSizeY, rawSizeZ;
	double rawSpaceX, rawSpaceY, rawSpaceZ;
	double rawOriginX,rawOriginY, rawOriginZ;
	unsigned int rawHeaderSize, rawByteOrder, rawScalarType;

	int ioID;
	std::vector<vtkSmartPointer<vtkImageData> > * m_volumes;
	std::vector<QString> * m_fileNames_volstack;

	QString m_additionalInfo;
	int m_channel;

	QVector<QString> m_hdf5Path;
	bool m_isITKHDF5;
	double m_hdf5Spacing[3];
	void loadHDF5File();
};

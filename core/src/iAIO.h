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

#include <itkVersorRigid3DTransform.h>

#include <vtkSmartPointer.h>

#include <QDir>
#include <QString>

#include <vector>

class vtkImageData;
class vtkMetaImageWriter;
class vtkPolyData;
class vtkSTLReader;
class vtkSTLWriter;
class vtkStringArray;
class vtkTable;

class iAObserverProgress;
class iASimReader;

typedef itk::VersorRigid3DTransform<double> VersorRigid3DTransformType;


class open_iA_Core_API iAIO : public iAAlgorithm
{
	Q_OBJECT
public:
	static const QString VolstackExtension;

	iAIO(vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0, std::vector<vtkSmartPointer<vtkImageData> > * volumes = 0, std::vector<QString> * fileNames = 0);
	iAIO(iALogger* logger, QObject *parent, std::vector<vtkSmartPointer<vtkImageData> > * volumes, std::vector<QString> * fileNames = 0);//TODO: QNDH for XRF volume stack loading
	virtual ~iAIO();
	void init(QObject *par);

	bool setupIO( IOType type, QString f, bool comp = false, int channel=-1);
	iAObserverProgress* getObserverProgress() { return observerProgress; };
	void iosettingswriter();
	void iosettingsreader();
	QStringList getFibreElementsName(bool withUnit);

	void setAdditionalInfo(QString const & additionalInfo);
	QString getAdditionalInfo();

	// TODO: move to Multimodal fusion
	// {
	void saveTransformFile(QString fName, VersorRigid3DTransformType*);
	VersorRigid3DTransformType* readTransformFile(QString fName);
	VersorRigid3DTransformType::Pointer mmFinalTransform;
	// }

	QString getFileName();

Q_SIGNALS:
	void msg(QString s);
	void done(bool active = false);
	void failed();

protected:
	virtual void run();

protected:
	
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

	bool readImageStack();

	void postImageReadActions();
	bool readRawImage();
	bool loadMetaImageFile(QString const & fileName);

	bool readVolumeStack( );
	bool readVolumeMHDStack( );
	bool readImageData( );
	bool readMetaImage( );
	bool readSTL( );
	bool readDCM( ); 
	bool writeDCM (); 
	bool readNRRD( ); 

	bool writeMetaImage( );
	bool writeVolumeStack();
	bool writeSTL( );
	bool writeImageStack( );
	
	void printFileInfos();
	void printSTLFileInfos();

	vtkSTLReader* stlReader;

	vtkMetaImageWriter *metaImageWriter;
	
	iAObserverProgress* observerProgress;

	QWidget *parent;
	QString fileName;
	QDir f_dir; 

	QString extension;
	QString prefix;
	QString fileNamesBase;
	vtkStringArray* fileNameArray;
	int dim;
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
	bool loadHDF5File();
};

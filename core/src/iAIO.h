/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IAIO_H
#define IAIO_H

#pragma once

#include "iAAlgorithms.h"
#include "open_iA_Core_export.h"

#include <itkVersorRigid3DTransform.h>

#include <vtkSmartPointer.h>

#include <QDir>
#include <QStringList>
#include <QString>
#include <QTextStream>

#include <vector>

class vtkBMPReader;
class vtkBMPWriter;
class vtkImageData;
class vtkJPEGReader;
class vtkJPEGWriter;
class vtkMetaImageWriter;
class vtkPNGReader;
class vtkPNGWriter;
class vtkPolyData;
class vtkSTLReader;
class vtkSTLWriter;
class vtkStringArray;
class vtkTable;
class vtkTIFFReader;
class vtkTIFFWriter;

class iAObserverProgress;
class iASimReader;

typedef itk::VersorRigid3DTransform<double> VersorRigid3DTransformType;


class open_iA_Core_API iAIO : public iAAlgorithms
{
	Q_OBJECT
public:
	static const QString VolstackExtension;

	iAIO(vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0, bool initVolumePlayer = false, std::vector<vtkSmartPointer<vtkImageData> > * volumes = 0, std::vector<QString> * fileNames = 0);
	iAIO(iALogger* logger, QObject *parent, std::vector<vtkSmartPointer<vtkImageData> > * volumes, std::vector<QString> * fileNames = 0);//TODO: QNDH for XRF volume stack loading
	virtual ~iAIO();
	void init(QObject *par);

	bool setupIO( IOType type, QString f, bool comp = false );
	iAObserverProgress* getObserverProgress() { return observerProgress; };
	void iosettingswriter();
	void iosettingsreader();
	bool loadCSVFile(vtkTable *table, FilterID fid, const QString &fileName);
	int calcTableLength(const QString &fileName);
	bool loadFibreCSV(vtkTable *talbe, const QString &fileName);
	bool loadPoreCSV(vtkTable *talbe, const QString &fileName);
	QStringList getFibreElementsName(bool withUnit);

	void setAdditionalInfo(QString const & additionalInfo);
	QString getAdditionalInfo();

	void saveTransformFile(QString fName, VersorRigid3DTransformType*);
	VersorRigid3DTransformType* readTransformFile(QString fName);

	VersorRigid3DTransformType::Pointer mmFinalTransform;

	QString getFileName();

Q_SIGNALS:
	void msg(QString s);
	void done(bool active = false);
	void failed();

protected:
	virtual void run();

protected:
	
private:
	bool setupPROReader( QString f );
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

	bool writeTIFImageStack( );
	bool writePNGImageStack( );
	bool writeJPGImageStack( );
	bool writeBMPImageStack( );
	
	void printFileInfos();
	void printSTLFileInfos();

	vtkSTLReader* stlReader;

	vtkMetaImageWriter *metaImageWriter;
	vtkSTLWriter *stlWriter;
	vtkTIFFWriter *tifWriter;
	vtkJPEGWriter *jpgWriter;
	vtkPNGWriter *pngWriter;
	vtkBMPWriter *bmpWriter;
	
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
	int rawSizeX,rawSizeY, rawSizeZ, rawScalar, rawByte;	double rawSpaceX, rawSpaceY, rawSpaceZ;	double rawOriginX,rawOriginY, rawOriginZ; unsigned int rawHeader;
	int proSizeX,proSizeY, proSizeZ, proScalar, proByte;	double proSpaceX, proSpaceY, proSpaceZ;	double proOriginX,proOriginY, proOriginZ; unsigned int proHeader;

	int ioID;
	std::vector<vtkSmartPointer<vtkImageData> > * m_volumes;
	std::vector<QString> * m_fileNames_volstack;

	QString m_additionalInfo;
};


#endif

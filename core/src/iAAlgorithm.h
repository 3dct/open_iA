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

#include "open_iA_Core_export.h"

#include <itkMesh.h>

#include <vtkSmartPointer.h>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QTime>
#include <QVector>

class vtkActor;
class vtkActor2D;
class vtkCornerAnnotation;
class vtkImageData;
class vtkPolyData;

class iAConnector;
class iALogger;
class iAProgress;

typedef itk::DefaultStaticMeshTraits< double, 3, 2, double, double >  MeshTraits;
typedef itk::Mesh< double, 3, MeshTraits > MeshType;
typedef itk::PointSet< double, 3, MeshTraits > PointSetType;

class open_iA_Core_API iAAlgorithm : public QThread
{
	Q_OBJECT
public:
	iAAlgorithm( QString fn, vtkImageData* i, vtkPolyData* p, iALogger * l, QObject *parent = 0 );
	iAAlgorithm( vtkImageData* i, vtkPolyData* p, iALogger * l, QObject *parent = 0 );
	virtual ~iAAlgorithm();

	QDateTime Start(); //< Start counting the running time and set the start time
	int Stop();	//Calculate and get the elapsed time

	void setup(QString fn, vtkImageData* i, vtkPolyData* p, iALogger * l );
	void addMsg(QString txt);

	iALogger* getLogger() const;
	QString getFilterName() const;
	vtkImageData* getVtkImageData();
	vtkPolyData* getVtkPolyData();
	iAConnector* getConnector() const;
	iAConnector* getFixedConnector() const;
	iAConnector* getConnector(int c);
	iAConnector *const * getConnectorArray() const;
	iAConnector ** getConnectorArray();
	bool deleteConnector(iAConnector* c);
	void allocConnectors(int size);

	iAProgress* getItkProgress();
	void vtkPolydata_itkMesh ( vtkPolyData* polyData, MeshType::Pointer mesh );
	void itkMesh_vtkPolydata( MeshType::Pointer mesh, vtkPolyData* polyData );
	virtual void SafeTerminate();


public slots:
	void updateVtkImageData(int ch);

signals:
	void startUpdate(int ch = 1);
	void aprogress(int i);

protected:
	virtual void run();
	void setImageData(vtkImageData* imgData);

private:
	bool m_isRunning;
	QTime m_time;
	int m_elapsed;
	QString m_filterName;
	vtkImageData *m_image;
	vtkPolyData *m_polyData;
	iAProgress *m_itkProgress;
	iALogger * m_logger;
	QMutex m_mutex;
	QWaitCondition m_condition;
	QVector<iAConnector*> m_connectors;
};

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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "pch.h"
#include "iAAlgorithm.h"

#include "iAConnector.h"
#include "iALogger.h"
#include "iAProgress.h"

#include <itkTriangleCell.h>

#include <vtkCellArray.h>
#include <vtkImageData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <QMessageBox>

iAAlgorithm::iAAlgorithm( QString fn, FilterID fid, vtkImageData* idata, vtkPolyData* p, iALogger * logger, QObject *parent )
	: QThread( parent )
{
	m_elapsed = 0;
	m_isRunning = false;
	m_filterName = fn;
	m_filterID = fid;
	m_image = idata;
	m_polyData = p;
	m_logger = logger;
	for (int i = 0; i < 2; ++i)
		m_connectors.push_back(new iAConnector());
	m_itkProgress = new iAProgress;

	connect(parent, SIGNAL( rendererDeactivated(int) ), this, SLOT( updateVtkImageData(int) ));
	connect(m_itkProgress, SIGNAL( pprogress(int) ), this, SIGNAL( aprogress(int) ));
}

iAAlgorithm::iAAlgorithm( vtkImageData* idata, vtkPolyData* p, iALogger* logger, QObject *parent )
: QThread( parent )
{
	m_image = idata;
	m_polyData = p;
	m_logger = logger;
	for (int i = 0; i < 2; ++i)
		m_connectors.push_back(new iAConnector());
	m_itkProgress = new iAProgress;
}


iAAlgorithm::~iAAlgorithm()
{
	foreach(iAConnector* c, m_connectors)
		delete c;
	m_connectors.clear();
	delete m_itkProgress;
}


void iAAlgorithm::run()
{
	addMsg(tr("  unknown filter type"));
}


void iAAlgorithm::setImageData(vtkImageData* imgData)
{
	m_image = imgData;
}

QDateTime iAAlgorithm::Start()
{
	m_elapsed = 0; 
	m_time.start();
	m_isRunning = true;
	return QDateTime::currentDateTime();
}


int iAAlgorithm::Stop()
{
	if (m_isRunning) 
	{	
		m_isRunning = false;
		m_elapsed = m_time.elapsed();
	}
	return m_elapsed;
}


void iAAlgorithm::setup(QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger * l)
{
	m_filterName = fn; 
	m_filterID = fid; 
	m_image = i; 
	m_polyData = p; 
	m_logger = l;
}

void iAAlgorithm::addMsg(QString txt)
{
	if (m_logger)
	{
		m_logger->log(txt);
	}
}


iALogger* iAAlgorithm::getLogger() const
{
	return m_logger;
}

QString iAAlgorithm::getFilterName() const
{
	return m_filterName;
}

vtkImageData* iAAlgorithm::getVtkImageData()
{
	return m_image;
}

vtkPolyData* iAAlgorithm::getVtkPolyData()
{
	return m_polyData;
}

iAConnector *iAAlgorithm::getConnector(int c)
{
	while (m_connectors.size() <= c)
		m_connectors.push_back(new iAConnector());
	return m_connectors[c];
}


iAConnector* iAAlgorithm::getConnector() const
{
	return m_connectors[0];
}

iAConnector *const * iAAlgorithm::getConnectorArray() const
{
	return m_connectors.data();
}

iAConnector ** iAAlgorithm::getConnectorArray()
{
	return m_connectors.data();
}

iAConnector* iAAlgorithm::getFixedConnector() const
{
	return m_connectors[1];
}

bool iAAlgorithm::deleteConnector(iAConnector* c)
{
	bool isDeleted = false;
	int ind = m_connectors.indexOf(c);
	if (ind >= 0)		
	{
		m_connectors.remove(ind);
		isDeleted = true;
	}
	delete c;
	return isDeleted;
}

void iAAlgorithm::allocConnectors(int size)
{
	while (m_connectors.size() < size)
		m_connectors.push_back(new iAConnector());
}


iAProgress* iAAlgorithm::getItkProgress()
{
	return m_itkProgress;
}

void iAAlgorithm::updateVtkImageData(int ch)
{
	// some remainder from working with multichannel images?
	// seems redundant to the m_image initialization in the constructor!
	m_image->ReleaseData();
	m_image->Initialize();
	m_image->DeepCopy(m_connectors[ch]->GetVTKImage());
	m_image->CopyInformationFromPipeline(m_connectors[ch]->GetVTKImage()->GetInformation());
	m_image->Modified();
}


void iAAlgorithm::itkMesh_vtkPolydata( MeshType::Pointer mesh, vtkPolyData* polyData )
{
	int numPoints =  mesh->GetNumberOfPoints();

	typedef MeshType::CellsContainerPointer	CellsContainerPointer;
	typedef MeshType::CellsContainerIterator CellsContainerIterator;
	typedef MeshType::CellType CellType; 
	typedef MeshType::PointsContainer MeshPointsContainer;
	typedef MeshType::PointType MeshPointType;
	typedef MeshPointsContainer::Pointer InputPointsContainerPointer;
	typedef MeshPointsContainer::Iterator InputPointsContainerIterator;
	InputPointsContainerPointer myPoints = mesh->GetPoints();
	InputPointsContainerIterator points = myPoints->Begin();
	MeshPointType point;

	vtkPoints  * pvtkPoints = vtkPoints::New();
	vtkCellArray * pvtkPolys = vtkCellArray::New();

	if (numPoints == 0)
		return; 

	pvtkPoints->SetNumberOfPoints(numPoints);

	int idx=0;
	double vpoint[3];
	while( points != myPoints->End() ) 	
	{   
		point = points.Value();
		vpoint[0]= point[0];
		vpoint[1]= point[1];
		vpoint[2]= point[2];
		pvtkPoints->SetPoint(idx++,vpoint);
		points++;
	}

	polyData->SetPoints(pvtkPoints);
	pvtkPoints->Delete();

	CellsContainerPointer cells = mesh->GetCells();
	CellsContainerIterator cellIt = cells->Begin();
	vtkIdType pts[3];

	while ( cellIt != cells->End() )
	{
		CellType *nextCell = cellIt->Value();
		CellType::PointIdIterator pointIt = nextCell->PointIdsBegin() ;
		MeshPointType  p;
		int i;

		switch (nextCell->GetType()) 
		{
		case CellType::VERTEX_CELL:
		case CellType::LINE_CELL:
		case CellType::POLYGON_CELL:
			break;        
		case CellType::TRIANGLE_CELL:
			i=0;
			while (pointIt != nextCell->PointIdsEnd() ) {
				pts[i++] = *pointIt++;  
			}
			pvtkPolys->InsertNextCell(3,pts);
			break;
		default:
			printf("something \n");
		}
		cellIt++;
	}
	polyData->SetPolys(pvtkPolys);
	pvtkPolys->Delete();
}


int iAAlgorithm::getFilterID() const
{
	return m_filterID;
}

void iAAlgorithm::vtkPolydata_itkMesh(vtkPolyData* polyData, MeshType::Pointer mesh)
{
	// Transfer the points from the vtkPolyData into the itk::Mesh
	const unsigned long numberOfPoints = polyData->GetNumberOfPoints();
	vtkPoints * vtkpoints = polyData->GetPoints();

	mesh->GetPoints()->Reserve( numberOfPoints );

	for( unsigned long p = 0; p < numberOfPoints; p++ )
	{
		double * apoint = vtkpoints->GetPoint( p );
		MeshType::PointType point = MeshType::PointType( apoint );
		mesh->SetPoint( p, point);
		mesh->SetPointData( p, 1);
	}

	// Transfer the cells from the vtkPolyData into the itk::Mesh
	vtkCellArray * triangleStrips = polyData->GetStrips();
	vtkIdType  * cellPoints;
	vtkIdType    numberOfCellPoints;

	// First count the total number of triangles from all the triangle strips.
	unsigned long numberOfTriangles = 0;
	triangleStrips->InitTraversal();
	while( triangleStrips->GetNextCell( numberOfCellPoints, cellPoints ) )
		numberOfTriangles += numberOfCellPoints-2;

	vtkCellArray * polygons = polyData->GetPolys();
	polygons->InitTraversal();
	while( polygons->GetNextCell( numberOfCellPoints, cellPoints ) )
		if( numberOfCellPoints == 3 )
			numberOfTriangles ++;

	// Reserve memory in the itk::Mesh for all those triangles
	mesh->GetCells()->Reserve( numberOfTriangles );

	// Copy the triangles from vtkPolyData into the itk::Mesh
	typedef MeshType::CellType   CellType;
	typedef itk::TriangleCell< CellType > TriangleCellType;

	int cellId = 0;

	// first copy the triangle strips
	triangleStrips->InitTraversal();
	while( triangleStrips->GetNextCell( numberOfCellPoints, cellPoints ) )
	{
		unsigned long numberOfTrianglesInStrip = numberOfCellPoints - 2;
		unsigned long pointIds[3];

		pointIds[0] = cellPoints[0];
		pointIds[1] = cellPoints[1];
		pointIds[2] = cellPoints[2];

		for( unsigned long t=0; t < numberOfTrianglesInStrip; t++ )
		{
			MeshType::CellAutoPointer c;
			TriangleCellType * tcell = new TriangleCellType;
			tcell->SetPointIds( (TriangleCellType::PointIdConstIterator)pointIds );
			c.TakeOwnership( tcell );
			mesh->SetCell( cellId, c );
			cellId++;
			pointIds[0] = pointIds[1];
			pointIds[1] = pointIds[2];
			pointIds[2] = cellPoints[t+3];
		}
	}

	// then copy the normal triangles
	polygons->InitTraversal();
	while( polygons->GetNextCell( numberOfCellPoints, cellPoints ) )
	{
		if( numberOfCellPoints !=3 ) // skip any non-triangle.
		{
			continue;
		}
		MeshType::CellAutoPointer c;
		TriangleCellType * t = new TriangleCellType;
		t->SetPointIds( (TriangleCellType::PointIdConstIterator)cellPoints );
		c.TakeOwnership( t );
		mesh->SetCell( cellId, c );
		cellId++;
	}

	std::cout << "Mesh  " << std::endl;
	std::cout << "Number of Points =   " << mesh->GetNumberOfPoints() << std::endl;
	std::cout << "Number of Cells  =   " << mesh->GetNumberOfCells()  << std::endl;
}


void iAAlgorithm::SafeTerminate()
{
	if(isRunning())
	{
		terminate();
	}
}

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
#include "iAAlgorithm.h"

#include "iAConnector.h"
#include "iALogger.h"
#include "iAProgress.h"

#include <itkTriangleCell.h>

#include <vtkCellArray.h>
#include <vtkImageData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <QLocale>
#include <QMessageBox>

iAAlgorithm::iAAlgorithm( QString fn, vtkImageData* idata, vtkPolyData* p, iALogger * logger, QObject *parent )
	: QThread( parent ),
	m_isRunning(false),
	m_filterName(fn),
	m_image(idata),
	m_polyData(p),
	m_logger(logger),
	m_progressObserver(new iAProgress)
{
	m_connectors.push_back(new iAConnector());
	if (parent)
		connect(parent, SIGNAL( rendererDeactivated(int) ), this, SLOT( updateVtkImageData(int) ));
	connect(m_progressObserver, SIGNAL( progress(int) ), this, SIGNAL( aprogress(int) ));
}

iAAlgorithm::~iAAlgorithm()
{
	foreach(iAConnector* c, m_connectors)
		delete c;
	m_connectors.clear();
	delete m_progressObserver;
}

void iAAlgorithm::run()
{
	Start();
	try
	{
		getConnector()->setImage(getVtkImageData());
		getConnector()->modified();
		performWork();
	}
	catch (itk::ExceptionObject &excep)
	{
		addMsg(tr("%1 terminated unexpectedly. Error: %2; in File %3, Line %4. Elapsed time: %5 ms.")
			.arg(getFilterName())
			.arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine())
			.arg(Stop()));
		return;
	}
	catch (const std::exception& e)
	{
		addMsg(tr("%1 terminated unexpectedly. Error: %2. Elapsed time: %3 ms.")
			.arg(getFilterName())
			.arg(e.what())
			.arg(Stop()));
		return;
	}
	addMsg(tr("%1 finished. Elapsed time: %2 ms.")
		.arg(getFilterName())
		.arg(Stop()));
	emit startUpdate();
}

void iAAlgorithm::performWork()
{
	addMsg(tr("Unknown filter type"));
}

void iAAlgorithm::setImageData(vtkImageData* imgData)
{
	m_image = imgData;
}

void iAAlgorithm::Start()
{
	m_time.start();
	m_isRunning = true;
}

int iAAlgorithm::Stop()
{
	m_isRunning = false;
	return m_time.elapsed();
}

void iAAlgorithm::setup(QString fn, vtkImageData* i, vtkPolyData* p, iALogger * l)
{
	m_filterName = fn;
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

iALogger* iAAlgorithm::logger() const
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

iAConnector* iAAlgorithm::getConnector() const
{
	return m_connectors[0];
}

QVector<iAConnector*> const & iAAlgorithm::Connectors() const
{
	return m_connectors;
}

void iAAlgorithm::AddImage(vtkImageData* i)
{
	auto con = new iAConnector();
	con->setImage(i);
	con->modified();
	m_connectors.push_back(con);
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

iAProgress* iAAlgorithm::ProgressObserver()
{
	return m_progressObserver;
}

void iAAlgorithm::updateVtkImageData(int ch)
{	// updates the vtk image data in the mdi child to be the one contained
	// in the m_connectors[ch].
	if (m_image == m_connectors[ch]->vtkImage().GetPointer())
		return;
	m_image->ReleaseData();
	m_image->Initialize();
	m_image->DeepCopy(m_connectors[ch]->vtkImage());
	m_image->CopyInformationFromPipeline(m_connectors[ch]->vtkImage()->GetInformation());
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
		CellType::PointIdIterator pointIt = nextCell->PointIdsBegin();
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

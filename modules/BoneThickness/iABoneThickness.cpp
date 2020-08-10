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
#include "iABoneThickness.h"

#include "iABoneThicknessChartBar.h"
#include "iABoneThicknessTable.h"
#include "iABoneThicknessMouseInteractor.h"
#include "iABoneThicknessAttachment.h"

#include <iARenderer.h>

#include <QFile>
#include <QStandardItemModel>
#include <QTextStream>


#include <vtkOBBTree.h> // new instead of CellLocator

#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkLineSource.h>
#include <vtkMath.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPCAStatistics.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointLocator.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>
#include <vtkTable.h>
#include <vtkTubeFilter.h>
#include <qvector.h>


iABoneThickness::iABoneThickness()
{
	m_pColorNormal  [0] = 1.0; m_pColorNormal  [1] = 0.0; m_pColorNormal  [2] = 0.0;
	m_pColorSelected[0] = 0.0; m_pColorSelected[1] = 1.0; m_pColorSelected[2] = 0.0;
	m_pColorMark    [0] = 0.0; m_pColorMark    [1] = 0.0; m_pColorMark    [2] = 1.0;
	std::fill(m_pBound, m_pBound+6, 0.0);
	std::fill(m_pRange, m_pRange+3, 0.0);
}

double iABoneThickness::axisXMax() const
{
	return m_pBound[1];
}

double iABoneThickness::axisXMin() const
{
	return m_pBound[0];
}

double iABoneThickness::axisYMax() const
{
	return m_pBound[3];
}

double iABoneThickness::axisYMin() const
{
	return m_pBound[2];
}

double iABoneThickness::axisZMax() const
{
	return m_pBound[5];
}

double iABoneThickness::axisZMin() const
{
	return m_pBound[4];
}

double iABoneThickness::meanThickness() const {
	return m_dThicknessMean;
}

double iABoneThickness::stdThickness() const {
	return m_dThicknessSTD;
}

double iABoneThickness::meanSurfaceDistance() const {
	return m_dSurfaceDistanceMean;
}

double iABoneThickness::stdSurfaceDistance() const {
	return m_dSurfaceDistanceSTD;
}

void iABoneThickness::calculate()
{
	// if there are landmarks present
	if (m_pPoints)
	{
		// Initialize point locator variable for detecting mesh vertices
		vtkSmartPointer<vtkPointLocator> pPointLocator(vtkSmartPointer<vtkPointLocator>::New());
		pPointLocator->SetDataSet(m_pPolyData);
		pPointLocator->BuildLocator();

		// Initialize OBBTree variable (similar to point locator) for detecting landmarks
		vtkSmartPointer<vtkOBBTree> pLandmarks(vtkSmartPointer<vtkOBBTree>::New());
		pLandmarks->SetDataSet(m_pPolyData);
		pLandmarks->BuildLocator();

		// landmark ids
		const vtkIdType idPoints(m_pPoints->GetNumberOfPoints());

		// length of the normal vector for intersection test
		const double dLength(0.5 * m_dRangeMax);

		// Go through each Landmark
		for (vtkIdType id (0); id < idPoints; ++id)
		{
			// Get landmark position
			double* pPoint (m_pPoints->GetPoint(id));

			// Calculate normal of mesh at the landmark position
			// normal vector computed with PC3 of vertex cloud within certain radius around landmark
			double pNormal[3];
			getNormalInPoint(pPointLocator, pPoint, pNormal);

			// Allocate intersection variable
			vtkSmartPointer<vtkPoints> intersectPoints1 = vtkSmartPointer<vtkPoints>::New();	// Intersection of positive normal vector
			vtkSmartPointer<vtkPoints> intersectPoints2 = vtkSmartPointer<vtkPoints>::New();	// intersection of negative normal vector
			vtkSmartPointer<vtkIdList> subIds = vtkSmartPointer<vtkIdList>::New();

			// start and end points of finite line to intersect with mesh
			double pStart[3]{ pPoint[0], pPoint[1], pPoint[2] };
			double pEnd[3] = { pPoint[0] - dLength * pNormal[0], pPoint[1] - dLength * pNormal[1], pPoint[2] - dLength * pNormal[2] };
			double pEndShift[3] = { pPoint[0] + dLength * pNormal[0], pPoint[1] + dLength * pNormal[1], pPoint[2] + dLength * pNormal[2] };

			// Initialize relevant intersection points
			double x1[3] { pStart[0], pStart[1], pStart[2] };
			double x2[3] { pStart[0], pStart[1], pStart[2] };

			// Temp variables
			int flag1 = 0;		// flag of return value of intersection function (1: intersection/outside of surface, -1: intersection/indise of surface, 0: no intersection)
			int flag2 = 0;
			double distance1;	// distances of start point to intersection points
			double distance2;

			// check all three intersection variations:
			// Landmark is inside of mesh
			// Landmark is outside of mesh, but surfaces are on both sides of the landmark (positive and negative normal direction)
			// Landmark is outside of mesh and surfaces are only on one normal direction

			// If both normal direction have intersection
			if ((flag1 = pLandmarks->IntersectWithLine(pStart, pEnd, intersectPoints1, subIds))&
				(flag2 = pLandmarks->IntersectWithLine(pStart, pEndShift, intersectPoints2, subIds))) {

				// Unknown case where landmark is inside and outside of the mesh at the same time (depends on certain normal vector directions)
				if (flag1 == -flag2) {
					if (intersectPoints1->GetNumberOfPoints() >=2) {
						intersectPoints1->GetPoint(0, x1);
						intersectPoints1->GetPoint(1, x2);
					}
					else {
						intersectPoints2->GetPoint(0, x1);
						intersectPoints2->GetPoint(1, x2);
					}
				}
				else {
					// Get the first intersection of each normal direction
					intersectPoints1->GetPoint(0, x1);
					intersectPoints2->GetPoint(0, x2);

					// Compute the distance of each first intersection
					distance1 = vtkMath::Distance2BetweenPoints(pStart, x1);
					distance2 = vtkMath::Distance2BetweenPoints(pStart, x2);

					// if landmark is outside of the STL
					if (flag1 == 1) {

						// the closest of both intersections will be starting point of thickness calculation
						if (distance1 < distance2) {
							intersectPoints1->GetPoint(0, x1);
							intersectPoints1->GetPoint(1, x2);
						}
						else {
							intersectPoints2->GetPoint(0, x1);
							intersectPoints2->GetPoint(1, x2);
						}
					}
					// if landmark is inside the STL
					else {

						// Project landmark onto the STL surface shortest away from the landmark
						if (distance1 < distance2) {
							intersectPoints1->GetPoint(0, x1);
							intersectPoints2->GetPoint(0, x2);
						}
						else {
							intersectPoints2->GetPoint(0, x1);
							intersectPoints1->GetPoint(0, x2);
						}
					}
				}
			}
			// If landmark is outside of the mesh and only one normal direction has intersections
			else {
				// check in which direction the intersection occured and set first two intersection and start and end point of thickness computation
				if (flag1) {
					if (intersectPoints1->GetNumberOfPoints() >= 2) {
						intersectPoints1->GetPoint(0, x1);
						intersectPoints1->GetPoint(1, x2);
					}
				}
				else if (flag2) {
					if (intersectPoints2->GetNumberOfPoints() >= 2) {
						intersectPoints2->GetPoint(0, x1);
						intersectPoints2->GetPoint(1, x2);
					}
				}
			}

			// Calculate distances:
			// Landmark to first intersection
			// Thickness of mesh: intersection1 to intersection2
			if (flag1 == -1) {
				setResults(id, sqrt(vtkMath::Distance2BetweenPoints(x1, x2)), -sqrt(vtkMath::Distance2BetweenPoints(pStart, x1)));
			}
			else {
				setResults(id, sqrt(vtkMath::Distance2BetweenPoints(x1, x2)), sqrt(vtkMath::Distance2BetweenPoints(pStart, x1)));
			}

			// Store coordinates in order to draw thickness and projection lines
			// Projection lines = green
			// Thickness line = blue
			m_pThLines[id]->SetPoint1(x1);
			m_pThLines[id]->SetPoint2(x2);

			m_pDaLines[id]->SetPoint1(pStart);
			m_pDaLines[id]->SetPoint2(x1);
		}

		// Calculate mean thickness
		m_dThicknessMean = 0.0;

		for (int i = 0; i < m_daThickness->GetNumberOfTuples(); ++i) {
			m_dThicknessMean += m_daThickness->GetValue(i);
		}
		m_dThicknessMean /= m_daThickness->GetNumberOfTuples();

		// Calculate STD of thickness
		m_dThicknessSTD = 0.0;

		for (int i = 0; i < m_daThickness->GetNumberOfTuples(); ++i) {
			m_dThicknessSTD += (m_daThickness->GetValue(i) - m_dThicknessMean) * (m_daThickness->GetValue(i) - m_dThicknessMean);
		}
		m_dThicknessSTD /= m_daThickness->GetNumberOfTuples() - 1;
		m_dThicknessSTD = sqrt(m_dThicknessSTD);

		// Calculate mean surface distance
		m_dSurfaceDistanceMean = 0.0;

		for (int i = 0; i < m_daDistance->GetNumberOfTuples(); ++i) {
			m_dSurfaceDistanceMean += m_daDistance->GetValue(i);
		}
		m_dSurfaceDistanceMean /= m_daDistance->GetNumberOfTuples();

		// Calculate STD of surface distance
		m_dSurfaceDistanceSTD = 0.0;

		for (int i = 0; i < m_daDistance->GetNumberOfTuples(); ++i) {
			m_dSurfaceDistanceSTD += (m_daDistance->GetValue(i) - m_dSurfaceDistanceMean) * (m_daDistance->GetValue(i) - m_dSurfaceDistanceMean);
		}
		m_dSurfaceDistanceSTD /= m_daDistance->GetNumberOfTuples() - 1;
		m_dSurfaceDistanceSTD = sqrt(m_dSurfaceDistanceSTD);

	}
}
bool iABoneThickness::getNormalFromPCA(vtkIdList* _pIdList, double* _pNormal)
{
	const vtkIdType idList(_pIdList->GetNumberOfIds());

	if (idList > 2)
	{
		vtkSmartPointer<vtkDoubleArray> pArrayX(vtkSmartPointer<vtkDoubleArray>::New());
		pArrayX->SetNumberOfComponents(1);
		pArrayX->SetName("x");

		vtkSmartPointer<vtkDoubleArray> pArrayY(vtkSmartPointer<vtkDoubleArray>::New());
		pArrayY->SetNumberOfComponents(1);
		pArrayY->SetName("y");

		vtkSmartPointer<vtkDoubleArray> pArrayZ(vtkSmartPointer<vtkDoubleArray>::New());
		pArrayZ->SetNumberOfComponents(1);
		pArrayZ->SetName("z");

		for (vtkIdType id (0) ; id < idList ; ++id)
		{
			const double* pPoint (m_pPolyData->GetPoint(_pIdList->GetId(id)));

			pArrayX->InsertNextValue(pPoint[0]);
			pArrayY->InsertNextValue(pPoint[1]);
			pArrayZ->InsertNextValue(pPoint[2]);
		}

		vtkSmartPointer<vtkTable> pDatasetTable(vtkSmartPointer<vtkTable>::New());
		pDatasetTable->AddColumn(pArrayX);
		pDatasetTable->AddColumn(pArrayY);
		pDatasetTable->AddColumn(pArrayZ);

		vtkSmartPointer<vtkPCAStatistics> pPCAStatistics(vtkSmartPointer<vtkPCAStatistics>::New());
		pPCAStatistics->SetDeriveOption(true);
		pPCAStatistics->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, pDatasetTable);

		pPCAStatistics->SetColumnStatus("x", 1);
		pPCAStatistics->SetColumnStatus("y", 1);
		pPCAStatistics->SetColumnStatus("z", 1);
		pPCAStatistics->RequestSelectedColumns();
		pPCAStatistics->Update();

		vtkSmartPointer<vtkDoubleArray> pEigenVector(vtkSmartPointer<vtkDoubleArray>::New());
		pPCAStatistics->GetEigenvector(2, pEigenVector);

		for (vtkIdType id(0); id < 3; ++id)
		{
			_pNormal[id] = pEigenVector->GetValue(id);
		}

		return true;
	}

	return false;
}

void iABoneThickness::getNormalInPoint(vtkPointLocator* _pPointLocator, double* _pPoint, double* _pNormal)
{
	vtkSmartPointer<vtkIdList> idListPointsWithinRadius(vtkSmartPointer<vtkIdList>::New());
	_pPointLocator->FindPointsWithinRadius(m_dSphereRadius, _pPoint, idListPointsWithinRadius);

	if (getNormalFromPCA(idListPointsWithinRadius, _pNormal))
	{
		vtkMath::Normalize(_pNormal);
	}
	else
	{
		_pNormal[0] = _pNormal[1] = _pNormal[2] = 0.0;
	}
}

void iABoneThickness::getSphereColor(const vtkIdType& _id, const double& _dRadius, double* pColor)
{
	if (m_idSelected == _id)
	{
		memcpy(pColor, m_pColorSelected, 3 * sizeof(double));
	}
	else if (_dRadius < FloatTolerance)
	{
		memcpy(pColor, m_pColorMark, 3 * sizeof(double));
	}
	else
	{
		memcpy(pColor, m_pColorNormal, 3 * sizeof(double));
	}
}

void iABoneThickness::open(const QString& _sFilename)
{
	QFile fFile(_sFilename);

	const bool bOpened(fFile.open(QIODevice::ReadOnly));

	if (bOpened)
	{
		m_pPoints = vtkSmartPointer<vtkPoints>::New();
		m_pThLines.clear();
		m_pDaLines.clear();

		m_daDistance = vtkSmartPointer<vtkDoubleArray>::New();
		m_daThickness = vtkSmartPointer<vtkDoubleArray>::New();

		QTextStream tsIn(&fFile);

		const double dZero(0.0);

		while (!tsIn.atEnd())
		{
			const QString sLine(tsIn.readLine());
			const QStringList slLine(sLine.split(" "));

			if (slLine.size() >= 3)
			{
				const double Point[3] = { slLine.at(0).toDouble(), slLine.at(1).toDouble(), slLine.at(2).toDouble() };
				m_pPoints->InsertNextPoint(Point);

				m_pThLines.push_back(vtkSmartPointer<vtkLineSource>::New());
				m_pDaLines.push_back(vtkSmartPointer<vtkLineSource>::New());

				m_daDistance->InsertNextTuple(&dZero);
				m_daThickness->InsertNextTuple(&dZero);
			}
		}

		fFile.close();
	}
}

double iABoneThickness::rangeMax() const
{
	return m_dRangeMax;
}

double iABoneThickness::rangeMin() const
{
	return m_dRangeMin;
}

double iABoneThickness::rangeX() const
{
	return m_pRange[0];
}

double iABoneThickness::rangeY() const
{
	return m_pRange[1];
}

double iABoneThickness::rangeZ() const
{
	return m_pRange[2];
}

void iABoneThickness::save(const QString& _sFilename) const
{
	QFile fFile(_sFilename);

	const bool bOpened(fFile.open(QIODevice::WriteOnly));

	if (bOpened)
	{
		QTextStream tsOut(&fFile);
		tsOut << "Index,X,Y,Z,Surface distance,Thickness\n";

		const vtkIdType idPoints(m_pPoints->GetNumberOfPoints());

		vtkIdType ii(1);

		for (vtkIdType i(0); i < idPoints; ++i, ++ii)
		{
			const double* pPoint(m_pPoints->GetPoint(i));

			tsOut << ii << "," << pPoint[0] << "," << pPoint[1] << "," << pPoint[2]
			      << "," << m_daDistance->GetTuple1(i) << "," << m_daThickness->GetTuple1(i)
			      << "," << "\n";
		}

		fFile.close();
	}
}

vtkIdType iABoneThickness::selected() const
{
	return m_idSelected;
}

void iABoneThickness::set(iARenderer* _iARenderer, vtkPolyData* _pPolyData, iABoneThicknessChartBar* _pBoneThicknessChartBar, iABoneThicknessTable* _pBoneThicknessTable)
{
	m_iARenderer = _iARenderer;
	m_iARenderer->polyActor()->GetProperty()->SetOpacity(m_dSurfaceOpacity);

	m_pPolyData = _pPolyData;
	m_pPolyData->GetBounds(m_pBound);

	m_pRange[0] = m_pBound[1] - m_pBound[0];
	m_pRange[1] = m_pBound[3] - m_pBound[2];
	m_pRange[2] = m_pBound[5] - m_pBound[4];

	m_dRangeMax = vtkMath::Max(m_pRange[0], vtkMath::Max(m_pRange[1], m_pRange[2]));
	m_dRangeMin = vtkMath::Min(m_pRange[0], vtkMath::Min(m_pRange[1], m_pRange[2]));

	m_dSphereRadius = 0.2 * vtkMath::Ceil(0.2 * m_dRangeMax);

	m_pThicknessLines = vtkActorCollection::New();
	m_pDistanceLines = vtkActorCollection::New();
	m_pSpheres = vtkActorCollection::New();

	vtkSmartPointer<iABoneThicknessMouseInteractor> pMouseInteractor(vtkSmartPointer<iABoneThicknessMouseInteractor>::New());
	pMouseInteractor->SetDefaultRenderer(m_iARenderer->renderer());
	pMouseInteractor->set(this, _pBoneThicknessChartBar, _pBoneThicknessTable, m_pSpheres);

	vtkRenderWindowInteractor* pWindowInteractor(m_iARenderer->interactor());
	pWindowInteractor->SetInteractorStyle(pMouseInteractor);

	//pWindowInteractor->Initialize();
}

void iABoneThickness::setChart(iABoneThicknessChartBar* _pBoneThicknessChartBar)
{
	_pBoneThicknessChartBar->setData(m_daThickness);
}

void iABoneThickness::setSelected(const vtkIdType& _idSelected)
{
	if (m_pSpheres)
	{
		const vtkIdType idSelectedBack (m_idSelected);

		m_idSelected = _idSelected;

		if (idSelectedBack > -1)
		{
			const double dRadius((m_bShowThickness) ? 0.5 * m_daThickness->GetTuple1(idSelectedBack) : m_dSphereRadius);

			double pColor[3];
			getSphereColor(idSelectedBack, dRadius, pColor);

			vtkActor* pActor1((vtkActor*)m_pSpheres->GetItemAsObject(idSelectedBack));
			pActor1->GetProperty()->SetColor(pColor);
		}

		if (m_idSelected > -1)
		{
			const double dRadius((m_bShowThickness) ? 0.5 * m_daThickness->GetTuple1(m_idSelected) : m_dSphereRadius);

			double pColor[3];
			getSphereColor(m_idSelected, dRadius, pColor);

			vtkActor* pActor2((vtkActor*)m_pSpheres->GetItemAsObject(m_idSelected));
			pActor2->GetProperty()->SetColor(pColor);
		}

		m_iARenderer->update();
	}
}

void iABoneThickness::setShowThickness(const bool& _bShowThickness)
{
	m_bShowThickness = _bShowThickness;
}

void iABoneThickness::setShowThicknessLines(const bool& _bShowThicknessLines)
{
	m_bShowThicknessLines = _bShowThicknessLines;
}

void iABoneThickness::setSphereOpacity(const double& _dSphereOpacity)
{
	m_dSphereOpacity = _dSphereOpacity;

	setTranslucent();

	const vtkIdType idSpheres(m_pSpheres->GetNumberOfItems());

	m_pSpheres->InitTraversal();

	for (int i(0); i < idSpheres; ++i)
	{
		m_pSpheres->GetNextActor()->GetProperty()->SetOpacity(m_dSphereOpacity);
	}
}

void iABoneThickness::setSphereRadius(const double& _dSphereRadius)
{
	m_dSphereRadius = _dSphereRadius;
}

void iABoneThickness::setSurfaceOpacity(const double& _dSurfaceOpacity)
{
	m_dSurfaceOpacity = _dSurfaceOpacity;

	setTranslucent();
	m_iARenderer->polyActor()->GetProperty()->SetOpacity(m_dSurfaceOpacity);
}

void iABoneThickness::setTable(iABoneThicknessTable* _iABoneThicknessTable)
{
	if (!m_pPoints)
		return;

	QStandardItemModel* pModel((QStandardItemModel*)_iABoneThicknessTable->model());

	pModel->removeRows(0, pModel->rowCount());

	const int iPointsSize((int) m_pPoints->GetNumberOfPoints());

	for (int i(0); i < iPointsSize; ++i)
	{
		pModel->insertRow(pModel->rowCount());

		for (int j(0); j < 3; ++j)
		{
			const QModelIndex miValue(pModel->index(i, j));

			pModel->setData(miValue, Qt::AlignCenter, Qt::TextAlignmentRole);
			pModel->setData(miValue, m_pPoints->GetPoint(i)[j], Qt::DisplayRole);
		}

		const QModelIndex miValue3(pModel->index(i, 3));

		pModel->setData(miValue3, Qt::AlignCenter, Qt::TextAlignmentRole);
		pModel->setData(miValue3, m_daDistance->GetTuple1(i), Qt::DisplayRole);

		const QModelIndex miValue4(pModel->index(i, 4));

		pModel->setData(miValue4, Qt::AlignCenter, Qt::TextAlignmentRole);
		pModel->setData(miValue4, m_daThickness->GetTuple1(i), Qt::DisplayRole);
	}
}

void iABoneThickness::setResults(const int& _iPoint, const double& _dThickness, const double& _dSurfaceDistance)
{
	if ((m_dSurfaceDistanceMaximum < FloatTolerance) || (_dSurfaceDistance < m_dSurfaceDistanceMaximum)) {

		m_daDistance->SetTuple1(_iPoint, _dSurfaceDistance);

		if ((m_dThicknessMaximum < FloatTolerance) || (_dThickness < m_dThicknessMaximum))
		{
			m_daThickness->SetTuple1(_iPoint, _dThickness);
		}
		else
		{
			m_daThickness->SetTuple1(_iPoint, 0.0);
		}
	}
	else {
		m_daThickness->SetTuple1(_iPoint, 0.0);
		m_daDistance->SetTuple1(_iPoint, 0.0);
	}

}


void iABoneThickness::setThicknessMaximum(const double& _dThicknessMaximum)
{
	m_dThicknessMaximum = _dThicknessMaximum;
}

void iABoneThickness::setSurfaceDistanceMaximum(const double& _dSurfaceDistanceMaximum)
{
	m_dSurfaceDistanceMaximum = _dSurfaceDistanceMaximum;
}

void iABoneThickness::setTranslucent()
{
	vtkOpenGLRenderer* pRenderer(m_iARenderer->renderer());
	vtkRenderWindow* pWindow(m_iARenderer->renderWindow());

	if ((m_dSphereOpacity < 0.99) || (m_dSurfaceOpacity < 0.99))
	{
		pWindow->SetAlphaBitPlanes(true);
		pWindow->SetMultiSamples(0);

		pRenderer->SetMaximumNumberOfPeels(100);
		pRenderer->SetOcclusionRatio(0.1);
		pRenderer->SetUseDepthPeeling(true);
	}
	else
	{
		pWindow->SetAlphaBitPlanes(false);
		pWindow->SetMultiSamples(8);

		pRenderer->SetMaximumNumberOfPeels(4);
		pRenderer->SetOcclusionRatio(0.0);
		pRenderer->SetUseDepthPeeling(false);
	}
}

void iABoneThickness::setTransparency(const bool& _bTransparency)
{
	if (_bTransparency)
	{
		setSphereOpacity(0.4);
		setSurfaceOpacity(0.6);
	}
	else
	{
		setSphereOpacity(1.0);
		setSurfaceOpacity(1.0);
	}

	m_iARenderer->update();
}

void iABoneThickness::setWindow()
{
	setWindowSpheres();
	setWindowThicknessLines();

	m_iARenderer->update();
}

void iABoneThickness::setWindowSpheres()
{
	if (m_pSpheres)
	{
		const vtkIdType idSpheresSize(m_pSpheres->GetNumberOfItems());

		m_pSpheres->InitTraversal();

		for (int i(0); i < idSpheresSize; ++i)
		{
			m_iARenderer->renderer()->RemoveActor(m_pSpheres->GetNextActor());
		}

		while (m_pSpheres->GetNumberOfItems())
		{
			m_pSpheres->RemoveItem(0);
		}
	}
	if (!m_pPoints)
		return;

	const vtkIdType idPointsSize(m_pPoints->GetNumberOfPoints());

	setTranslucent();

	for (vtkIdType id(0); id < idPointsSize; ++id)
	{
		const double dRadius((m_bShowThickness) ? 0.5 * m_daThickness->GetTuple1(id) : m_dSphereRadius);

		vtkSmartPointer<vtkSphereSource> pSphere(vtkSmartPointer<vtkSphereSource>::New());
		const double* pPoint(m_pPoints->GetPoint(id));
		pSphere->SetCenter(pPoint[0], pPoint[1], pPoint[2]);
		pSphere->SetRadius((dRadius < FloatTolerance) ? 0.5 : dRadius);
		pSphere->Update();

		vtkSmartPointer<vtkPolyDataMapper> pMapper(vtkSmartPointer<vtkPolyDataMapper>::New());
		pMapper->SetInputConnection(pSphere->GetOutputPort());

		vtkSmartPointer<vtkActor> pActor(vtkSmartPointer<vtkActor>::New());
		double pColor[3];
		getSphereColor(id, dRadius, pColor);
		pActor->GetProperty()->SetColor(pColor);
		pActor->GetProperty()->SetOpacity(m_dSphereOpacity);
		pActor->SetMapper(pMapper);

		m_pSpheres->AddItem(pActor);
		m_iARenderer->renderer()->AddActor(pActor);
	}
}

void iABoneThickness::setWindowThicknessLines()
{
	if (m_pThicknessLines)
	{
		const vtkIdType idThicknessLinesSize(m_pThicknessLines->GetNumberOfItems());

		m_pThicknessLines->InitTraversal();

		for (int i(0); i < idThicknessLinesSize; ++i)
		{
			m_iARenderer->renderer()->RemoveActor(m_pThicknessLines->GetNextActor());
		}

		while (m_pThicknessLines->GetNumberOfItems())
		{
			m_pThicknessLines->RemoveItem(0);
		}
	}

	if (m_pDistanceLines)
	{
		const vtkIdType idThicknessLinesSize(m_pDistanceLines->GetNumberOfItems());

		m_pDistanceLines->InitTraversal();

		for (int i(0); i < idThicknessLinesSize; ++i)
		{
			m_iARenderer->renderer()->RemoveActor(m_pDistanceLines->GetNextActor());
		}

		while (m_pDistanceLines->GetNumberOfItems())
		{
			m_pDistanceLines->RemoveItem(0);
		}
	}

	if (m_bShowThicknessLines)
	{
		const vtkIdType idLinesSize(m_pThLines.size());

		for (vtkIdType i(0); i < idLinesSize; ++i)
		{
			if (m_daThickness->GetTuple1(i) > 0.01)
			{
				vtkSmartPointer<vtkTubeFilter> pTubeFilter(vtkSmartPointer<vtkTubeFilter>::New());
				pTubeFilter->SetInputConnection(m_pThLines[i]->GetOutputPort());
				pTubeFilter->SetRadius(0.05);
				pTubeFilter->SetNumberOfSides(50);
				pTubeFilter->Update();

				vtkSmartPointer<vtkPolyDataMapper> pTubeMapper(vtkSmartPointer<vtkPolyDataMapper>::New());
				pTubeMapper->SetInputConnection(pTubeFilter->GetOutputPort());

				vtkSmartPointer<vtkActor> pTubeActor(vtkSmartPointer<vtkActor>::New());
				pTubeActor->GetProperty()->SetColor(m_pColorMark);
				pTubeActor->SetMapper(pTubeMapper);

				m_pThicknessLines->AddItem(pTubeActor);
				m_iARenderer->renderer()->AddActor(pTubeActor);
			}

			if (m_daDistance->GetTuple1(i) > 0.01)
			{
				vtkSmartPointer<vtkTubeFilter> pTubeFilter(vtkSmartPointer<vtkTubeFilter>::New());
				pTubeFilter->SetInputConnection(m_pDaLines[i]->GetOutputPort());
				pTubeFilter->SetRadius(0.05);
				pTubeFilter->SetNumberOfSides(50);
				pTubeFilter->Update();

				vtkSmartPointer<vtkPolyDataMapper> pTubeMapper(vtkSmartPointer<vtkPolyDataMapper>::New());
				pTubeMapper->SetInputConnection(pTubeFilter->GetOutputPort());

				vtkSmartPointer<vtkActor> pTubeActor(vtkSmartPointer<vtkActor>::New());
				pTubeActor->GetProperty()->SetColor(m_pColorSelected);
				pTubeActor->SetMapper(pTubeMapper);

				m_pDistanceLines->AddItem(pTubeActor);
				m_iARenderer->renderer()->AddActor(pTubeActor);
			}
		}
	}

	/*if (m_bShowThicknessLines)
	{
		const vtkIdType idLinesSize(m_pDaLines.size());

		for (vtkIdType i(0); i < idLinesSize; ++i)
		{
			if (m_daDistance->GetTuple1(i) > 0.01)
			{
				vtkSmartPointer<vtkTubeFilter> pTubeFilter(vtkSmartPointer<vtkTubeFilter>::New());
				pTubeFilter->SetInputConnection(m_pDaLines[i]->GetOutputPort());
				pTubeFilter->SetRadius(0.05);
				pTubeFilter->SetNumberOfSides(50);
				pTubeFilter->Update();

				vtkSmartPointer<vtkPolyDataMapper> pTubeMapper(vtkSmartPointer<vtkPolyDataMapper>::New());
				pTubeMapper->SetInputConnection(pTubeFilter->GetOutputPort());

				vtkSmartPointer<vtkActor> pTubeActor(vtkSmartPointer<vtkActor>::New());
				pTubeActor->GetProperty()->SetColor(m_pColorSelected);
				pTubeActor->SetMapper(pTubeMapper);

				m_pDistanceLines->AddItem(pTubeActor);
				m_iARenderer->GetRenderer()->AddActor(pTubeActor);
			}
		}
	}*/
}

bool iABoneThickness::showThickness() const
{
	return m_bShowThickness;
}

double iABoneThickness::sphereOpacity() const
{
	return m_dSphereOpacity;
}

double iABoneThickness::sphereRadius() const
{
	return m_dSphereRadius;
}

double iABoneThickness::surfaceOpacity() const
{
	return m_dSurfaceOpacity;
}

vtkDoubleArray* iABoneThickness::thickness()
{
	return m_daThickness.GetPointer();
}

double iABoneThickness::thicknessMaximum() const
{
	return m_dThicknessMaximum;
}

double iABoneThickness::surfaceDistanceMaximum() const
{
	return m_dSurfaceDistanceMaximum;
}

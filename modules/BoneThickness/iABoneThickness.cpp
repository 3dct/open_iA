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

#include "iABoneThickness.h"

#include <QFile>
#include <QStandardItemModel>
#include <QTextStream>

#include <vtkCellLocator.h>
#include <vtkDoubleArray.h>
#include <vtkLineSource.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPCAStatistics.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointLocator.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>
#include <vtkTable.h>
#include <vtkTubeFilter.h>

#include <iARenderer.h>

#include "iABoneThicknessChart.h"
#include "iABoneThicknessTable.h"
#include "iABoneThicknessMouseInteractor.h"

vtkStandardNewMacro(iABoneThicknessMouseInteractor);

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

void iABoneThickness::calculate()
{
	if (m_pPoints)
	{
		getDistance();

		vtkSmartPointer<vtkCellLocator> CellLocator(vtkCellLocator::New());
		CellLocator->SetDataSet(m_pPolyData);
		CellLocator->BuildLocator();
		CellLocator->Update();

		vtkSmartPointer<vtkPoints> PointNormals(vtkPoints::New());
		getNormalsInPoint(PointNormals);

		const vtkIdType idPoints(m_pPoints->GetNumberOfPoints());

		const double dLength(0.5 * vtkMath::Max(m_pRange[0], vtkMath::Max(m_pRange[1], m_pRange[2])));

		double tol(0.0);
		double t(0.0);
		double pcoords[3];
		double x1[3];
		double x2[3];

		int subId (0);

		for (int i(0); i < idPoints; ++i)
		{
			double* pPoint (m_pPoints->GetPoint(i));
			
			const double* pNormal (PointNormals->GetPoint(i));
			double pPoint21[3] = { pPoint[0] + dLength * pNormal[0], pPoint[1] + dLength * pNormal[1], pPoint[2] + dLength * pNormal[2] };
			double pPoint22[3] = { pPoint[0] - dLength * pNormal[0], pPoint[1] - dLength * pNormal[1], pPoint[2] - dLength * pNormal[2] };

			double dThickness1(0.0);

			if (CellLocator->IntersectWithLine(pPoint21, pPoint, tol, t, x1, pcoords, subId))
			{
				dThickness1 = sqrt(vtkMath::Distance2BetweenPoints(pPoint, x1));

				setThickness(i, dThickness1);

				m_pLines[i]->SetPoint1(pPoint);
				m_pLines[i]->SetPoint2(x1);
			}
			else
			{
				setThickness(i, 0.0);

				m_pLines[i]->SetPoint1(x1);
				m_pLines[i]->SetPoint2(x1);
			}

			if (CellLocator->IntersectWithLine(pPoint22, pPoint, tol, t, x2, pcoords, subId))
			{
				const double dThickness2(sqrt(vtkMath::Distance2BetweenPoints(pPoint, x2)));

				if (dThickness2 > dThickness1)
				{
					setThickness(i, dThickness2);

					m_pLines[i]->SetPoint1(pPoint);
					m_pLines[i]->SetPoint2(x2);
				}
			}
		}
	}
}

void iABoneThickness::findPoints(QVector<vtkSmartPointer<vtkPoints>>& _vPoints)
{
	vtkSmartPointer<vtkPointLocator> pPointLocator(vtkSmartPointer<vtkPointLocator>::New());
	pPointLocator->SetDataSet(m_pPolyData);
	pPointLocator->BuildLocator();

	const vtkIdType idPoints(m_pPoints->GetNumberOfPoints());

	for (vtkIdType i(0); i < idPoints; ++i)
	{
		const double* pPoint (m_pPoints->GetPoint(i));

		vtkSmartPointer<vtkIdList> idListPointsWithinRadius(vtkSmartPointer<vtkIdList>::New());
		pPointLocator->FindPointsWithinRadius(m_dSphereRadius, pPoint, idListPointsWithinRadius);

		const vtkIdType idPointsWithinRadius(idListPointsWithinRadius->GetNumberOfIds());

		for (vtkIdType ii(0); ii < idPointsWithinRadius; ++ii)
		{
			const double* pPointFromlist (m_pPolyData->GetPoint(ii));

			_vPoints[i]->InsertNextPoint(pPointFromlist);
		}
	}
}

void iABoneThickness::getDistance()
{
	vtkSmartPointer<vtkPointLocator> pPointLocator(vtkSmartPointer<vtkPointLocator>::New());
	pPointLocator->SetDataSet(m_pPolyData);
	pPointLocator->BuildLocator();

	const vtkIdType idPoints(m_pPoints->GetNumberOfPoints());

	for (int i(0); i < idPoints; ++i)
	{
		const double* pPoint(m_pPoints->GetPoint(i));
		const double* pPointClosest (m_pPolyData->GetPoint(pPointLocator->FindClosestPoint(pPoint)));

		m_daDistance->SetTuple1(i, vtkMath::Distance2BetweenPoints(pPoint, pPointClosest));
	}
}

bool iABoneThickness::getNormalFromPCA(vtkPoints* _pPoints, double* _pNormal)
{
	const vtkIdType idPoints(_pPoints->GetNumberOfPoints());

	if (idPoints > 2)
	{
		vtkSmartPointer<vtkDoubleArray> xArray(vtkSmartPointer<vtkDoubleArray>::New());
		xArray->SetNumberOfComponents(1);
		xArray->SetName("x");

		vtkSmartPointer<vtkDoubleArray> yArray(vtkSmartPointer<vtkDoubleArray>::New());
		yArray->SetNumberOfComponents(1);
		yArray->SetName("y");

		vtkSmartPointer<vtkDoubleArray> zArray(vtkSmartPointer<vtkDoubleArray>::New());
		zArray->SetNumberOfComponents(1);
		zArray->SetName("z");

		double pPoint[3];

		for (vtkIdType i (0); i < idPoints; i++)
		{
			_pPoints->GetPoint(i, pPoint);

			xArray->InsertNextValue(pPoint[0]);
			yArray->InsertNextValue(pPoint[1]);
			zArray->InsertNextValue(pPoint[2]);
		}

		vtkSmartPointer<vtkTable> datasetTable(vtkSmartPointer<vtkTable>::New());
		datasetTable->AddColumn(xArray);
		datasetTable->AddColumn(yArray);
		datasetTable->AddColumn(zArray);

		vtkSmartPointer<vtkPCAStatistics> pcaStatistics(vtkSmartPointer<vtkPCAStatistics>::New());
		pcaStatistics->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, datasetTable);

		pcaStatistics->SetColumnStatus("x", 1);
		pcaStatistics->SetColumnStatus("y", 1);
		pcaStatistics->SetColumnStatus("z", 1);
		pcaStatistics->RequestSelectedColumns();
		pcaStatistics->SetDeriveOption(true);
		pcaStatistics->Update();

		vtkSmartPointer<vtkDoubleArray> eigenVector(vtkSmartPointer<vtkDoubleArray>::New());
		pcaStatistics->GetEigenvector(2, eigenVector);

		for (vtkIdType i(0); i < 3; i++)
		{
			_pNormal[i] = eigenVector->GetValue(i);
		}

		return true;
	}

	return false;
}

void iABoneThickness::getNormalsInPoint(vtkPoints* _pPointNormals)
{
	if (m_pPoints)
	{
		const vtkIdType idPoints(m_pPoints->GetNumberOfPoints());

		QVector<vtkSmartPointer<vtkPoints>> vPoints;
		vPoints.resize(idPoints);

		for (vtkIdType i(0); i < idPoints; ++i)
		{
			vPoints[i] = vtkPoints::New();
		}

		findPoints(vPoints);

		double pNormal[3];

		for (vtkIdType i(0); i < idPoints; ++i)
		{
			if (getNormalFromPCA(vPoints[i], pNormal))
			{
				vtkMath::Normalize(pNormal);
			}
			else
			{
				pNormal[0] = pNormal[1] = pNormal[2] = 0.0;
			}

			_pPointNormals->InsertNextPoint(pNormal);
		}
	}
}

void iABoneThickness::open(const QString& _sFilename)
{
	QFile fFile(_sFilename);

	const bool bOpened(fFile.open(QIODevice::ReadOnly));

	if (bOpened)
	{
		m_pPoints = vtkSmartPointer<vtkPoints>::New();
		m_pLines.clear();

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

				m_pLines.push_back(vtkSmartPointer<vtkLineSource>::New());

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

void iABoneThickness::set(iARenderer* _iARenderer, vtkPolyData* _pPolyData, iABoneThicknessChart* _pBoneThicknessChart, iABoneThicknessTable* _pBoneThicknessTable)
{
	m_iARenderer = _iARenderer;
	m_iARenderer->GetPolyActor()->GetProperty()->SetOpacity(m_dSurfaceOpacity);

	m_pPolyData = _pPolyData;
	m_pPolyData->GetBounds(m_pBound);

	m_pRange[0] = m_pBound[1] - m_pBound[0];
	m_pRange[1] = m_pBound[3] - m_pBound[2];
	m_pRange[2] = m_pBound[5] - m_pBound[4];

	m_dRangeMax = vtkMath::Max(m_pRange[0], vtkMath::Max(m_pRange[1], m_pRange[2]));
	m_dRangeMin = vtkMath::Min(m_pRange[0], vtkMath::Min(m_pRange[1], m_pRange[2]));

	m_dSphereRadius = 0.2 * vtkMath::Ceil(0.2 * m_dRangeMax);

	m_pThicknessLines = vtkActorCollection::New();
	m_pSpheres = vtkActorCollection::New();

	vtkSmartPointer<iABoneThicknessMouseInteractor> pMouseInteractor(vtkSmartPointer<iABoneThicknessMouseInteractor>::New());
	pMouseInteractor->SetDefaultRenderer(m_iARenderer->GetRenderer());
	pMouseInteractor->set(this, _pBoneThicknessChart, _pBoneThicknessTable, m_pSpheres);

	vtkRenderWindowInteractor* pWindowInteractor(m_iARenderer->GetInteractor());
	pWindowInteractor->SetInteractorStyle(pMouseInteractor);

	pWindowInteractor->Initialize();
	pWindowInteractor->Start();
}

void iABoneThickness::setChart(iABoneThicknessChart* _pBoneThicknessChart)
{
	_pBoneThicknessChart->setData(m_daThickness);
}

void iABoneThickness::setSelected(const vtkIdType& _idSelected)
{
	if (m_pSpheres)
	{
		if (m_idSelected > -1)
		{
			vtkActor* pActor1((vtkActor*)m_pSpheres->GetItemAsObject(m_idSelected));
			pActor1->GetProperty()->SetColor(1.0, 0.0, 0.0);
		}

		m_idSelected = _idSelected;

		if (m_idSelected > -1)
		{
			vtkActor* pActor2((vtkActor*)m_pSpheres->GetItemAsObject(m_idSelected));
			pActor2->GetProperty()->SetColor(0.0, 1.0, 0.0);
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
	m_iARenderer->GetPolyActor()->GetProperty()->SetOpacity(m_dSurfaceOpacity);
}

void iABoneThickness::setTable(iABoneThicknessTable* _iABoneThicknessTable)
{
	QStandardItemModel* pModel((QStandardItemModel*)_iABoneThicknessTable->model());

	pModel->removeRows(0, pModel->rowCount());

	const vtkIdType idPointsSize(m_pPoints->GetNumberOfPoints());

	for (int i(0); i < idPointsSize; ++i)
	{
		pModel->insertRow(pModel->rowCount());

		for (int j(0); j < 3; ++j)
		{
			const QModelIndex miValue(pModel->index(i, j));

			pModel->setData(miValue, Qt::AlignCenter, Qt::TextAlignmentRole);
			pModel->setData(miValue, m_pPoints->GetPoint(i)[j], Qt::DisplayRole);
		}

	}

	const vtkIdType idDistance(m_daDistance->GetNumberOfTuples());

	for (int i(0); i < idDistance ; ++i)
	{
		const QModelIndex miValue(pModel->index(i, 3));

		pModel->setData(miValue, Qt::AlignCenter, Qt::TextAlignmentRole);
		pModel->setData(miValue, m_daDistance->GetTuple1(i), Qt::DisplayRole);
	}

	const vtkIdType idThickness(m_daThickness->GetNumberOfTuples());

	for (int i(0); i < idThickness ; ++i)
	{
		const QModelIndex miValue(pModel->index(i, 4));

		pModel->setData(miValue, Qt::AlignCenter, Qt::TextAlignmentRole);
		pModel->setData(miValue, m_daThickness->GetTuple1(i), Qt::DisplayRole);
	}
}

void iABoneThickness::setThickness(const int& _iPoint, const double& _dThickness)
{
	if ((m_dThicknessMaximum == 0.0) || (_dThickness < m_dThicknessMaximum))
	{
		m_daThickness->SetTuple1(_iPoint, _dThickness);
	}
	else
	{
		m_daThickness->SetTuple1(_iPoint, 0.0);
	}
}

void iABoneThickness::setThicknessMaximum(const double& _dThicknessMaximum)
{
	m_dThicknessMaximum = _dThicknessMaximum;
}

void iABoneThickness::setTranslucent()
{
	vtkOpenGLRenderer* pRenderer(m_iARenderer->GetRenderer());
	vtkRenderWindow* pWindow(m_iARenderer->GetRenderWindow());

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
			m_iARenderer->GetRenderer()->RemoveActor(m_pSpheres->GetNextActor());
		}

		while (m_pSpheres->GetNumberOfItems())
		{
			m_pSpheres->RemoveItem(0);
		}
	}

	const vtkIdType idPointsSize(m_pPoints->GetNumberOfPoints());

	setTranslucent();

	double pColorDeselected[3] = { 1.0, 0.0, 0.0 };
	double pColorSelected[3] = { 0.0, 1.0, 0.0 };
		
	for (vtkIdType i(0); i < idPointsSize; ++i)
	{
		vtkSmartPointer<vtkSphereSource> pSphere(vtkSmartPointer<vtkSphereSource>::New());
		pSphere->SetCenter(m_pPoints->GetPoint(i)[0], m_pPoints->GetPoint(i)[1], m_pPoints->GetPoint(i)[2]);
		pSphere->SetRadius((m_bShowThickness) ? vtkMath::Max(0.1, 0.5 * m_daThickness->GetTuple1(i)) : m_dSphereRadius);
		pSphere->Update();

		vtkSmartPointer<vtkPolyDataMapper> pMapper(vtkSmartPointer<vtkPolyDataMapper>::New());
		pMapper->SetInputConnection(pSphere->GetOutputPort());

		vtkSmartPointer<vtkActor> pActor(vtkSmartPointer<vtkActor>::New());
		pActor->GetProperty()->SetColor((m_idSelected == i) ? pColorSelected : pColorDeselected);
		pActor->GetProperty()->SetOpacity(m_dSphereOpacity);
		pActor->SetMapper(pMapper);

		m_pSpheres->AddItem(pActor);
		m_iARenderer->GetRenderer()->AddActor(pActor);
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
			m_iARenderer->GetRenderer()->RemoveActor(m_pThicknessLines->GetNextActor());
		}

		while (m_pThicknessLines->GetNumberOfItems())
		{
			m_pThicknessLines->RemoveItem(0);
		}
	}

	if (m_bShowThicknessLines)
	{
		const vtkIdType idLinesSize(m_pLines.size());

		for (vtkIdType i(0); i < idLinesSize; ++i)
		{
			if (m_daThickness->GetTuple1(i) > 0.01)
			{
				vtkSmartPointer<vtkTubeFilter> pTubeFilter(vtkSmartPointer<vtkTubeFilter>::New());
				pTubeFilter->SetInputConnection(m_pLines[i]->GetOutputPort());
				pTubeFilter->SetRadius(0.05);
				pTubeFilter->SetNumberOfSides(50);
				pTubeFilter->Update();

				vtkSmartPointer<vtkPolyDataMapper> pTubeMapper(vtkSmartPointer<vtkPolyDataMapper>::New());
				pTubeMapper->SetInputConnection(pTubeFilter->GetOutputPort());

				vtkSmartPointer<vtkActor> pTubeActor(vtkSmartPointer<vtkActor>::New());
				pTubeActor->GetProperty()->SetColor(0.0, 0.0, 1.0);
				pTubeActor->SetMapper(pTubeMapper);

				m_pThicknessLines->AddItem(pTubeActor);
				m_iARenderer->GetRenderer()->AddActor(pTubeActor);
			}
		}
	}
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

double iABoneThickness::thicknessMaximum() const
{
	return m_dThicknessMaximum;
}

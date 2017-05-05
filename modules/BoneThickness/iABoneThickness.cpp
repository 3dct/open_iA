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

#include "iABoneThicknessChartBar.h"
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
        vtkSmartPointer<vtkPointLocator> pPointLocator(vtkSmartPointer<vtkPointLocator>::New());
		pPointLocator->SetDataSet(m_pPolyData);
		pPointLocator->BuildLocator();

		vtkSmartPointer<vtkCellLocator> CellLocator(vtkCellLocator::New());
		CellLocator->SetDataSet(m_pPolyData);
		CellLocator->BuildLocator();
		CellLocator->Update();

		const vtkIdType idPoints(m_pPoints->GetNumberOfPoints());

		const double dLength(0.5 * m_dRangeMax);

		double tol(0.0);
		double t(0.0);
		double pcoords[3];
		double x1[3];
		double x2[3];

		int subId (0);

		for (vtkIdType id (0); id < idPoints; ++id)
		{
			double* pPoint (m_pPoints->GetPoint(id));
		
			const double* pPointClosest(m_pPolyData->GetPoint(pPointLocator->FindClosestPoint(pPoint)));
			m_daDistance->SetTuple1(id, vtkMath::Distance2BetweenPoints(pPoint, pPointClosest));

			double pNormal[3];
			getNormalInPoint(pPointLocator, pPoint, pNormal);
			double pPoint21[3] = { pPoint[0] + dLength * pNormal[0], pPoint[1] + dLength * pNormal[1], pPoint[2] + dLength * pNormal[2] };
			double pPoint22[3] = { pPoint[0] - dLength * pNormal[0], pPoint[1] - dLength * pNormal[1], pPoint[2] - dLength * pNormal[2] };

			double dThickness1(0.0);

			if (CellLocator->IntersectWithLine(pPoint21, pPoint, tol, t, x1, pcoords, subId))
			{
				dThickness1 = sqrt(vtkMath::Distance2BetweenPoints(pPoint, x1));

				setThickness(id, dThickness1);

				m_pLines[id]->SetPoint1(pPoint);
				m_pLines[id]->SetPoint2(x1);
			}
			else
			{
				setThickness(id, 0.0);

				m_pLines[id]->SetPoint1(x1);
				m_pLines[id]->SetPoint2(x1);
			}

			if (CellLocator->IntersectWithLine(pPoint22, pPoint, tol, t, x2, pcoords, subId))
			{
				const double dThickness2(sqrt(vtkMath::Distance2BetweenPoints(pPoint, x2)));

				if (dThickness2 > dThickness1)
				{
					setThickness(id, dThickness2);

					m_pLines[id]->SetPoint1(pPoint);
					m_pLines[id]->SetPoint2(x2);
				}
			}
		}
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

vtkIdType iABoneThickness::selected() const
{
	return m_idSelected;
}

void iABoneThickness::set(iARenderer* _iARenderer, vtkPolyData* _pPolyData, iABoneThicknessChartBar* _pBoneThicknessChartBar, iABoneThicknessTable* _pBoneThicknessTable)
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
	pMouseInteractor->set(this, _pBoneThicknessChartBar, _pBoneThicknessTable, m_pSpheres);

	vtkRenderWindowInteractor* pWindowInteractor(m_iARenderer->GetInteractor());
	pWindowInteractor->SetInteractorStyle(pMouseInteractor);

	pWindowInteractor->Initialize();
	pWindowInteractor->Start();
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
	m_iARenderer->GetPolyActor()->GetProperty()->SetOpacity(m_dSurfaceOpacity);
}

void iABoneThickness::setTable(iABoneThicknessTable* _iABoneThicknessTable)
{
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

void iABoneThickness::setThickness(const int& _iPoint, const double& _dThickness)
{
	if ((m_dThicknessMaximum < FloatTolerance) || (_dThickness < m_dThicknessMaximum))
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
				pTubeActor->GetProperty()->SetColor(m_pColorMark);
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

vtkDoubleArray* iABoneThickness::thickness()
{
	return m_daThickness.GetPointer();
}

double iABoneThickness::thicknessMaximum() const
{
	return m_dThicknessMaximum;
}

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

#include "iABoneThicknessTable.h"

#include <QFile>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QTextStream>

#include <iARenderer.h>

#include <vtkObjectFactory.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>

vtkStandardNewMacro(iABoneThicknessMouseInteractor);

iABoneThicknessTable::iABoneThicknessTable(iARenderer* _iARenderer, QWidget* _pParent) : QTableView (_pParent), m_iARenderer (_iARenderer)
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);

	QStandardItemModel* pItemModel (new QStandardItemModel(0, 5, this));
	pItemModel->setHorizontalHeaderItem(0, new QStandardItem("X"));
	pItemModel->setHorizontalHeaderItem(1, new QStandardItem("Y"));
	pItemModel->setHorizontalHeaderItem(2, new QStandardItem("Z"));
	pItemModel->setHorizontalHeaderItem(3, new QStandardItem("Surface distance"));
	pItemModel->setHorizontalHeaderItem(4, new QStandardItem("Thickness"));

	horizontalHeader()->setSectionsClickable(false);
	verticalHeader()->setSectionsClickable(false);

	setModel(pItemModel);

	m_pThicknessLines = vtkActorCollection::New();
	m_pSpheres = vtkActorCollection::New();

	m_iARenderer->GetPolyActor()->GetProperty()->SetOpacity(m_dSurfaceOpacity);
/*
	vtkSmartPointer<iABoneThicknessMouseInteractor> pMouseInteractor(vtkSmartPointer<iABoneThicknessMouseInteractor>::New());
	pMouseInteractor->SetDefaultRenderer(m_iARenderer->GetRenderer());
	pMouseInteractor->setBoneThicknessTable(this);
	pMouseInteractor->setSphereCollection(m_pSpheres);

	vtkSmartPointer<vtkRenderWindowInteractor> pWindowInteractor(vtkSmartPointer<vtkRenderWindowInteractor>::New());
	pWindowInteractor->SetRenderWindow(m_iARenderer->GetRenderWindow());
	pWindowInteractor->SetInteractorStyle(pMouseInteractor);
	pWindowInteractor->Initialize();
	pWindowInteractor->Start();
*/
}

QVector<double>* iABoneThicknessTable::distance()
{
	return &m_vDistance;
}

QVector<vtkSmartPointer<vtkLineSource>>* iABoneThicknessTable::lines()
{
	return &m_pLines;
}

void iABoneThicknessTable::open(const QString& _sFilename)
{
	QFile fFile(_sFilename);

	const bool bOpened(fFile.open(QIODevice::ReadOnly));

	if (bOpened)
	{
		m_pPoints = vtkSmartPointer<vtkPoints>::New();
		m_pLines.clear();

		m_vDistance.clear();
		m_vThickness.clear();

		QTextStream tsIn(&fFile);

		int i (0);

		while (!tsIn.atEnd())
		{
			const QString sLine(tsIn.readLine());
			const QStringList slLine(sLine.split(" "));

			if (slLine.size() >= 3)
			{
				const double Point[3] = {slLine.at(0).toDouble(), slLine.at(1).toDouble(), slLine.at(2).toDouble()};
				m_pPoints->InsertNextPoint(Point);
				m_pLines.push_back(vtkSmartPointer<vtkLineSource>::New());
				m_vDistance.push_back(0.0f);
				m_vThickness.push_back(0.0f);
				++i;
			}
		}

		fFile.close();
	}
}

vtkSmartPointer<vtkPoints> iABoneThicknessTable::point()
{
	return m_pPoints;
}

void iABoneThicknessTable::save(const QString& _sFilename)
{
	QFile fFile(_sFilename);

	const bool bOpened(fFile.open(QIODevice::WriteOnly));

	if (bOpened)
	{
		QTextStream tsOut(&fFile);
		tsOut << "Index,X,Y,Z,Surface distance,Thickness\n";

		const vtkIdType idPointsSize(m_pPoints->GetNumberOfPoints());

		vtkIdType ii(1);

		for (vtkIdType i(0); i < idPointsSize; ++i, ++ii)
		{
			const double* pPoint(m_pPoints->GetPoint(i));

			tsOut << ii << "," << pPoint[0] << "," << pPoint[1] << "," << pPoint[2] << "," << m_vDistance.at(i) << "," << m_vThickness.at(i) << "," << "\n";
		}

		fFile.close();
	}
}

void iABoneThicknessTable::selectionChanged(const QItemSelection& _itemSelected, const QItemSelection& _itemDeselected)
{
	QTableView::QTableView::selectionChanged(_itemSelected, _itemDeselected);

	if (selectionModel()->selectedRows().size())
	{
		m_idSphereSelected = selectionModel()->selectedRows().at(0).row();

		const vtkIdType idSpheresSize(m_pSpheres->GetNumberOfItems());

		m_pSpheres->InitTraversal();

		for (int i(0); i < idSpheresSize; ++i)
		{
			if (m_idSphereSelected == i)
			{
				m_pSpheres->GetNextActor()->GetProperty()->SetColor(0.0, 1.0, 0.0);
			}
			else
			{
				m_pSpheres->GetNextActor()->GetProperty()->SetColor(1.0, 0.0, 0.0);
			}
		}

		m_iARenderer->update();
	}
}

void iABoneThicknessTable::setOpacity(const double& _dOpacity)
{
	setSphereOpacity(0.5 * _dOpacity);
	setSurfaceOpacity(_dOpacity);

	m_iARenderer->update();
}

void iABoneThicknessTable::setSphereOpacity(const double& _dSphereOpacity)
{
	m_dSphereOpacity = _dSphereOpacity;

	setTranslucent();

	const vtkIdType idSpheresSize(m_pSpheres->GetNumberOfItems());

	m_pSpheres->InitTraversal();

	for (int i(0); i < idSpheresSize; ++i)
	{
		m_pSpheres->GetNextActor()->GetProperty()->SetOpacity(m_dSphereOpacity);
	}
}

void iABoneThicknessTable::setSphereRadius(const double& _dSphereRadius)
{
	m_dSphereRadius = _dSphereRadius;
}

void iABoneThicknessTable::setSphereSelected(const vtkIdType& _idSphereSelected)
{
	m_idSphereSelected = _idSphereSelected;
	selectRow(m_idSphereSelected);
}

void iABoneThicknessTable::setSurfaceOpacity(const double& _dSurfaceOpacity)
{
	m_dSurfaceOpacity = _dSurfaceOpacity;

	setTranslucent();
	m_iARenderer->GetPolyActor()->GetProperty()->SetOpacity(m_dSurfaceOpacity);
}

void iABoneThicknessTable::setTable()
{
	model()->removeRows(0, model()->rowCount());

	const vtkIdType idPointsSize(m_pPoints->GetNumberOfPoints());

	for (int i(0); i < idPointsSize; ++i)
	{
		model()->insertRow(model()->rowCount());

		for (int j(0); j < 3; ++j)
		{
			const QModelIndex miValue(model()->index(i, j));

			model()->setData(miValue, Qt::AlignCenter, Qt::TextAlignmentRole);
			model()->setData(miValue, m_pPoints->GetPoint(i)[j], Qt::DisplayRole);
		}

	}

	for (int i(0); i < m_vDistance.size(); ++i)
	{
		const QModelIndex miValue(model()->index(i, 3));

		model()->setData(miValue, Qt::AlignCenter, Qt::TextAlignmentRole);
		model()->setData(miValue, m_vDistance.at(i), Qt::DisplayRole);
	}

	for (int i(0); i < m_vThickness.size(); ++i)
	{
		const QModelIndex miValue(model()->index(i, 4));

		model()->setData(miValue, Qt::AlignCenter, Qt::TextAlignmentRole);
		model()->setData(miValue, m_vThickness.at(i), Qt::DisplayRole);
	}
}

void iABoneThicknessTable::setTranslucent()
{
	vtkOpenGLRenderer* pRenderer(m_iARenderer->GetRenderer());
	vtkRenderWindow* pWindow (m_iARenderer->GetRenderWindow());

	if ((m_dSphereOpacity < 1.0) || (m_dSurfaceOpacity < 1.0))
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

void iABoneThicknessTable::setWindow()
{
	setWindowSpheres();
	setWindowThicknessLines();

	m_iARenderer->update();
}

void iABoneThicknessTable::setWindowSpheres()
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

	for (vtkIdType i(0); i < idPointsSize; ++i)
	{
		vtkSmartPointer<vtkSphereSource> pSphere(vtkSmartPointer<vtkSphereSource>::New());
		pSphere->SetCenter(m_pPoints->GetPoint(i)[0], m_pPoints->GetPoint(i)[1], m_pPoints->GetPoint(i)[2]);
		pSphere->SetRadius(m_dSphereRadius);
		pSphere->Update();

		vtkSmartPointer<vtkPolyDataMapper> pMapper(vtkSmartPointer<vtkPolyDataMapper>::New());
		pMapper->SetInputConnection(pSphere->GetOutputPort());

		vtkSmartPointer<vtkActor> pActor(vtkSmartPointer<vtkActor>::New());
		pActor->SetMapper(pMapper);
		pActor->GetProperty()->SetOpacity(m_dSphereOpacity);

		if (m_idSphereSelected == i)
		{
			pActor->GetProperty()->SetColor(0.0, 1.0, 0.0);
		}
		else
		{
			pActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
		}

		m_pSpheres->AddItem(pActor);
		m_iARenderer->GetRenderer()->AddActor(pActor);
	}
}

void iABoneThicknessTable::setWindowThicknessLines()
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

	const vtkIdType idLinesSize(m_pLines.size());

	for (vtkIdType i(0); i < idLinesSize; ++i)
	{
		vtkSmartPointer<vtkPolyDataMapper> pMapper(vtkSmartPointer<vtkPolyDataMapper>::New());
		pMapper->SetInputConnection(m_pLines[i]->GetOutputPort());

		vtkSmartPointer<vtkActor> pActor(vtkSmartPointer<vtkActor>::New());
		pActor->GetProperty()->SetColor(0.0, 0.0, 1.0);
		pActor->SetMapper(pMapper);

		m_pThicknessLines->AddItem(pActor);
		m_iARenderer->GetRenderer()->AddActor(pActor);
	}
}

double iABoneThicknessTable::sphereOpacity() const
{
	return m_dSphereOpacity;
}

double iABoneThicknessTable::sphereRadius() const
{
	return m_dSphereRadius;
}

double iABoneThicknessTable::surfaceOpacity() const
{
	return m_dSurfaceOpacity;
}

QVector<double>* iABoneThicknessTable::thickness()
{
	return &m_vThickness;
}

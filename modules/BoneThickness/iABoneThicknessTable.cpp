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
#include <vtkOpenGLRenderer.h>

#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

iABoneThicknessTable::iABoneThicknessTable(QWidget* _pParent) : QTableView (_pParent)
{
	setFocusPolicy(Qt::NoFocus);
	setSelectionBehavior(QAbstractItemView::SelectRows);

	QStandardItemModel* pItemModel (new QStandardItemModel(0, 5, this));
	pItemModel->setHorizontalHeaderItem(0, new QStandardItem("X"));
	pItemModel->setHorizontalHeaderItem(1, new QStandardItem("Y"));
	pItemModel->setHorizontalHeaderItem(2, new QStandardItem("Z"));
	pItemModel->setHorizontalHeaderItem(3, new QStandardItem("Surface distance"));
	pItemModel->setHorizontalHeaderItem(4, new QStandardItem("Thickness"));

	horizontalHeader()->setSectionsClickable(false);
	verticalHeader()->setSectionsClickable(false);

	setModel(pItemModel);
}

QVector<double>* iABoneThicknessTable::distance()
{
	return &m_vDistance;
}

void iABoneThicknessTable::open(const QString& _sFilename)
{
	QFile fFile(_sFilename);

	const bool bOpened(fFile.open(QIODevice::ReadOnly));

	if (bOpened)
	{
		m_points = vtkSmartPointer<vtkPoints>::New();

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
				m_points->InsertNextPoint(Point);
				m_vDistance.push_back(0.0f);
				m_vThickness.push_back(0.0f);
				++i;
			}
		}

		fFile.close();

		setTable();
	}
}

vtkSmartPointer<vtkPoints> iABoneThicknessTable::point()
{
	return m_points;
}

QVector<double>* iABoneThicknessTable::thickness()
{
	return &m_vThickness;
}

void iABoneThicknessTable::setTable()
{
	model()->removeRows(0, model()->rowCount());

	for (int i(0); i < m_points->GetNumberOfPoints(); ++i)
	{
		model()->insertRow(model()->rowCount());

		for (int j(0); j < 3; ++j)
		{
			const QModelIndex miValue(model()->index(i, j));

			model()->setData(miValue, Qt::AlignCenter, Qt::TextAlignmentRole);
			model()->setData(miValue, m_points->GetPoint(i)[j], Qt::DisplayRole);
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

void iABoneThicknessTable::setWindow(iARenderer* _iARenderer)
{
	if (m_actors)
	{
		const vtkIdType ActorSize(m_actors->GetNumberOfItems());

		for (int i(0); i < ActorSize; ++i)
		{
			_iARenderer->GetRenderer()->RemoveActor((vtkActor*) m_actors->GetItemAsObject(i));
		}

		for (int i(0); i < ActorSize; ++i)
		{
			m_actors->RemoveItem(i);
		}
	}

	m_actors = vtkActorCollection::New();

	const vtkIdType PointsSize(m_points->GetNumberOfPoints());

	for (vtkIdType i(0); i < PointsSize ; ++i)
	{
		vtkSmartPointer<vtkSphereSource> sphereSource (vtkSmartPointer<vtkSphereSource>::New());
		sphereSource->SetCenter(m_points->GetPoint(i)[0], m_points->GetPoint(i)[1], m_points->GetPoint(i)[2]);
		sphereSource->SetRadius(0.3);

		vtkSmartPointer<vtkPolyDataMapper> mapper (vtkSmartPointer<vtkPolyDataMapper>::New());
		mapper->SetInputConnection(sphereSource->GetOutputPort());

		vtkSmartPointer<vtkActor> actor (vtkSmartPointer<vtkActor>::New());
		actor->SetMapper(mapper);
		actor->GetProperty()->SetColor(1.0, 0.0, 0.0);

		m_actors->AddItem(actor);

		_iARenderer->GetRenderer()->AddActor(actor);
	}
}

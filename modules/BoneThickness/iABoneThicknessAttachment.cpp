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

#include "iABoneThicknessAttachment.h"

#include "mdichild.h"
#include "mainwindow.h"

#include<vtkCellLocator.h>
#include<vtkMath.h>
#include<vtkPolyData.h>
#include<vtkPoints.h>

#include "iADockWidgetWrapper.h"

#include <QPushButton>
#include <QFileDialog>

iABoneThicknessAttachment::iABoneThicknessAttachment(MainWindow * mainWnd, iAChildData childData):
	iAModuleAttachmentToChild(mainWnd, childData)
{
	QWidget* pBoneThicknessWidget (new QWidget());

	m_pBoneThicknessTable = new iABoneThicknessTable(pBoneThicknessWidget);

	const int iPushButtonWidth (2 * pBoneThicknessWidget->logicalDpiX());

	QPushButton* pPushButtonBoneThicknessOpen(new QPushButton("Open point file...", pBoneThicknessWidget));
	pPushButtonBoneThicknessOpen->setFixedWidth(iPushButtonWidth);
	connect(pPushButtonBoneThicknessOpen, SIGNAL(clicked()), this, SLOT(slotPushButtonBoneThicknessOpen()));

	QGridLayout* pBoneThicknessLayout (new QGridLayout(pBoneThicknessWidget));
	pBoneThicknessLayout->addWidget(m_pBoneThicknessTable);
	pBoneThicknessLayout->addWidget(pPushButtonBoneThicknessOpen);

	childData.child->tabifyDockWidget(childData.logs, new iADockWidgetWrapper(pBoneThicknessWidget, tr("Bone thickness"), "BoneThickness"));
}

void iABoneThicknessAttachment::Calculate()
{
	qApp->setOverrideCursor(Qt::WaitCursor);

	vtkSmartPointer<vtkCellLocator> CellLocator(vtkCellLocator::New());
	CellLocator->SetDataSet(m_childData.polyData);
	CellLocator->BuildLocator();
	CellLocator->Update();

	vtkSmartPointer<vtkPoints> Points(m_pBoneThicknessTable->point());

	const vtkIdType PointsTable (Points->GetNumberOfPoints());

	QVector<double>* pvDistance(m_pBoneThicknessTable->distance());
	QVector<double>* pvThickness(m_pBoneThicknessTable->thickness());
	
	vtkSmartPointer<vtkPoints> PointNormals(vtkPoints::New());
	AddPointNormalsIn(PointNormals);

	for (int i(0); i < PointsTable ; ++i)
	{
		double Point1[3];
		Points->GetPoint(i, Point1);

		vtkIdType cellId;
		int subId;
		double closestPointDist2 (0.0);
		double PointClosest1[3];
		CellLocator->FindClosestPoint(Point1, PointClosest1, cellId, subId, closestPointDist2);

		(*pvDistance)[i] = sqrt(closestPointDist2);

		double Point2[3];
		PointNormals->GetPoint(i, Point2);

		for (int ii(0); ii < 3; ++ii)
		{
			Point2[ii] += PointClosest1[ii];
			Point2[ii] *= -100.0;
		}

		double tol (0.0);
		double t (0.0);
		double x[3];
		double pcoords[3];

		const int result (CellLocator->IntersectWithLine(Point2, PointClosest1, tol, t, x, pcoords, subId));

		if (result == 1)
		{
			(*pvThickness)[i] = sqrt(vtkMath::Distance2BetweenPoints(PointClosest1, x));
		}
		else
		{
			(*pvThickness)[i] = 0.0;
		}
	}

	m_pBoneThicknessTable->setTable();
	qApp->restoreOverrideCursor();
}

void iABoneThicknessAttachment::AddPointNormalsIn(vtkPoints* PointNormals)
{
	vtkPolyData* PolyData(m_childData.polyData);
	const vtkIdType PointsData (PolyData->GetNumberOfPoints());

	vtkSmartPointer<vtkPoints> Points(m_pBoneThicknessTable->point());
	const vtkIdType PointsTable(Points->GetNumberOfPoints());

	const double PointRadius(m_pBoneThicknessTable->pointRadius());

	QVector<vtkSmartPointer<vtkPoints>> vPoints;
	vPoints.resize(PointsTable);

	for (vtkIdType i(0); i < PointsTable; ++i)
	{
		vPoints[i] = vtkPoints::New();
	}

	for (vtkIdType j (0); j < PointsData; ++j)
	{
		double PointData[3];
		PolyData->GetPoint(j, PointData);

		for (vtkIdType i(0); i < PointsTable; ++i)
		{
			double Point1[3];
			Points->GetPoint(i, Point1);

			const double Distance(sqrt(vtkMath::Distance2BetweenPoints(Point1, PointData)));

			if (Distance < PointRadius)
			{
				vPoints[i]->InsertNextPoint(PointData);
			}
		}
	}

	double Normal[3];
		
	for (vtkIdType i(0); i < PointsTable; ++i)
	{
		GetNormal(vPoints[i], Normal);
		vtkMath::Normalize(Normal);

		PointNormals->InsertNextPoint(Normal);
	}
}

void iABoneThicknessAttachment::GetNormal(vtkPoints* Points, double* Normal)
{
	const vtkIdType PointsArea(Points->GetNumberOfPoints());

	double Point1[3];
	Points->GetPoint(0, Point1);

	double sumX(Point1[0]);
	double sumXX(Point1[0] * Point1[0]);
	double sumXY(Point1[0] * Point1[1]);

	double sumY(Point1[1]);
	double sumYY(Point1[1] * Point1[1]);

	double sumXZ(Point1[0] * Point1[2]);
	double sumYZ(Point1[1] * Point1[2]);
	double sumZ(Point1[2]);

	for (vtkIdType i(1); i < PointsArea ; ++i)
	{
		Points->GetPoint(i, Point1);

		sumX += Point1[0];
		sumXX += Point1[0] * Point1[0];
		sumXY += Point1[0] * Point1[1];
		sumXZ += Point1[0] * Point1[2];

		sumY += Point1[1];
		sumYY += Point1[1] * Point1[1];
		sumYZ += Point1[1] * Point1[2];

		sumZ += Point1[2];
	}

	const double AB(sumXX * sumYY - sumXY * sumXY);

	Normal[0] = (sumXZ * sumYY - sumYZ * sumXY) / AB; // A
	Normal[1] = (sumXX * sumXZ - sumXZ * sumXY) / AB; // B
	Normal[2] = (sumZ - Normal[0] * sumX - Normal[1] * sumY) / ((double)PointsArea); // C
}

void iABoneThicknessAttachment::slotPushButtonBoneThicknessOpen()
{
	QPushButton* pPushButtonOpen ((QPushButton*) sender());

	QFileDialog* pFileDialog (new QFileDialog());
	pFileDialog->setAcceptMode(QFileDialog::AcceptOpen);
	pFileDialog->setDefaultSuffix("txt");
	pFileDialog->setFileMode(QFileDialog::ExistingFile);
	pFileDialog->setNameFilter("Point file (*.txt)");
	pFileDialog->setWindowTitle(pPushButtonOpen->text());

	if (pFileDialog->exec())
	{
		m_pBoneThicknessTable->open(pFileDialog->selectedFiles().first());
		m_pBoneThicknessTable->setWindow(m_childData.child->getRaycaster());
	}

	delete pFileDialog;

	Calculate();
}

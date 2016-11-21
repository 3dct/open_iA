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
#include<vtkIdList.h>

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

void iABoneThicknessAttachment::calculate()
{
	qApp->setOverrideCursor(Qt::WaitCursor);

	vtkSmartPointer<vtkPoints> Points(m_pBoneThicknessTable->point());
    const vtkIdType PointsNumber (Points->GetNumberOfPoints());

	QVector<double>* pvDistance(m_pBoneThicknessTable->distance());
	QVector<double>* pvThickness(m_pBoneThicknessTable->thickness());

	vtkSmartPointer<vtkCellLocator> CellLocator(vtkCellLocator::New());
	CellLocator->SetDataSet(m_childData.polyData);
	CellLocator->BuildLocator();

	vtkSmartPointer<vtkIdList> ptIds(vtkIdList::New());

	for (int i(0); i < PointsNumber; ++i)
	{
		double Point1[3];
		Points->GetPoint(i, Point1);

		vtkIdType cellId;
		int subId;
		double closestPointDist2 (0.0);
		double PointClosest1[3];
		CellLocator->FindClosestPoint(Point1, PointClosest1, cellId, subId, closestPointDist2);

		(*pvDistance)[i] = sqrt(closestPointDist2);

		double Point2[3] = { PointClosest1[0], PointClosest1[1] - 100.0, PointClosest1[2] };

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

	calculate();
}

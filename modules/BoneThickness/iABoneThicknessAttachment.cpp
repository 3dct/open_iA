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
#include<vtkOpenGLRenderer.h>
#include<vtkPolyData.h>
#include<vtkPoints.h>

#include <iADockWidgetWrapper.h>
#include <iARenderer.h>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>

iABoneThicknessAttachment::iABoneThicknessAttachment(MainWindow* _pMainWnd, iAChildData _iaChildData):
	iAModuleAttachmentToChild(_pMainWnd, _iaChildData)
{
	vtkPolyData* pPolyData(m_childData.polyData);
	pPolyData->GetBounds(m_pBound);

	m_pRange[0] = m_pBound[1] - m_pBound[0];
	m_pRange[1] = m_pBound[3] - m_pBound[2];
	m_pRange[2] = m_pBound[3] - m_pBound[4];

	QWidget* pWidget(new QWidget());

	QPushButton* pPushButtonOpen(new QPushButton("Open point file...", pWidget));
	pPushButtonOpen->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogOpenButton));
	connect(pPushButtonOpen, SIGNAL(clicked()), this, SLOT(slotPushButtonOpen()));

	QPushButton* pPushButtonSave(new QPushButton("Save table to CSV format...", pWidget));
	pPushButtonSave->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogSaveButton));
	connect(pPushButtonSave, SIGNAL(clicked()), this, SLOT(slotPushButtonSave()));

	QGroupBox* pGroupBoxBound(new QGroupBox("Surface bounds", pWidget));
	QLabel* pLabelBoundXMin(new QLabel(QString("X min: %1").arg(m_pBound[0]), pGroupBoxBound));
	QLabel* pLabelBoundXMax(new QLabel(QString("X max: %1").arg(m_pBound[1]), pGroupBoxBound));
	QLabel* pLabelBoundXRng(new QLabel(QString("X rng: %1").arg(m_pRange[0]), pGroupBoxBound));
	QLabel* pLabelBoundYMin(new QLabel(QString("Y min: %1").arg(m_pBound[2]), pGroupBoxBound));
	QLabel* pLabelBoundYMax(new QLabel(QString("Y max: %1").arg(m_pBound[3]), pGroupBoxBound));
	QLabel* pLabelBoundYRng(new QLabel(QString("Y rng: %1").arg(m_pRange[1]), pGroupBoxBound));
	QLabel* pLabelBoundZMin(new QLabel(QString("Z min: %1").arg(m_pBound[4]), pGroupBoxBound));
	QLabel* pLabelBoundZMax(new QLabel(QString("Z max: %1").arg(m_pBound[5]), pGroupBoxBound));
	QLabel* pLabelBoundZRng(new QLabel(QString("Z rng: %1").arg(m_pRange[2]), pGroupBoxBound));

	QGridLayout* pGridLayoutBound(new QGridLayout(pGroupBoxBound));
	pGridLayoutBound->addWidget(pLabelBoundXMin, 0, 0);
	pGridLayoutBound->addWidget(pLabelBoundXMax, 0, 1);
	pGridLayoutBound->addWidget(pLabelBoundXRng, 0, 2);
	pGridLayoutBound->addWidget(pLabelBoundYMin, 0, 3);
	pGridLayoutBound->addWidget(pLabelBoundYMax, 0, 4);
	pGridLayoutBound->addWidget(pLabelBoundYRng, 0, 5);
	pGridLayoutBound->addWidget(pLabelBoundZMin, 0, 6);
	pGridLayoutBound->addWidget(pLabelBoundZMax, 0, 7);
	pGridLayoutBound->addWidget(pLabelBoundZRng, 0, 8);

	m_pBoneThicknessTable = new iABoneThicknessTable(m_childData.child->getRaycaster(), pWidget);
	m_pBoneThicknessTable->setSphereRadius(0.1 * vtkMath::Ceil(0.2 * vtkMath::Max(m_pRange[0], vtkMath::Max(m_pRange[1], m_pRange[2]))));

	QGroupBox* pGroupBoxSettings(new QGroupBox("Settings", pWidget));

	QCheckBox* pCheckBoxTransparency(new QCheckBox("Transparency", pGroupBoxSettings));
	connect(pCheckBoxTransparency, SIGNAL(clicked(const bool&)), this, SLOT(slotCheckBoxTransparency(const bool&)));

	QLabel* pLabelSphereRadius(new QLabel("Spheres radius:", pGroupBoxSettings));
	QDoubleSpinBox* pDoubleSpinBoxSphereRadius(new QDoubleSpinBox(pGroupBoxSettings));
	pDoubleSpinBoxSphereRadius->setAlignment(Qt::AlignRight);
	pDoubleSpinBoxSphereRadius->setMinimum(0.1);
	pDoubleSpinBoxSphereRadius->setSingleStep(0.1);
	pDoubleSpinBoxSphereRadius->setValue(m_pBoneThicknessTable->sphereRadius());
	connect(pDoubleSpinBoxSphereRadius, SIGNAL(valueChanged(const double&)), this, SLOT(slotDoubleSpinBoxSphereRadius(const double&)));

	QGridLayout* pGridLayoutSettings(new QGridLayout(pGroupBoxSettings));
	pGridLayoutSettings->addWidget(pLabelSphereRadius, 0, 0);
	pGridLayoutSettings->addWidget(pDoubleSpinBoxSphereRadius, 0, 1);
	pGridLayoutSettings->addWidget(pCheckBoxTransparency, 0, 2);

	QGridLayout* pGridLayout(new QGridLayout(pWidget));
	pGridLayout->addWidget(pPushButtonOpen, 0, 0);
	pGridLayout->addWidget(pPushButtonSave, 0, 1);
	pGridLayout->addWidget(pGroupBoxBound, 1, 0, 1, 2);
	pGridLayout->addWidget(m_pBoneThicknessTable, 2, 0, 1, 2);
	pGridLayout->addWidget(pGroupBoxSettings, 3, 0, 1, 2);

	_iaChildData.child->tabifyDockWidget(_iaChildData.logs, new iADockWidgetWrapper(pWidget, tr("Bone thickness"), "BoneThickness"));
}

void iABoneThicknessAttachment::addPointNormalsIn(vtkPoints* _pPointNormals)
{
	vtkPolyData* pPolyData(m_childData.polyData);
	const vtkIdType idPointsData(pPolyData->GetNumberOfPoints());

	vtkSmartPointer<vtkPoints> pPoints(m_pBoneThicknessTable->point());
	const vtkIdType idPointsTable(pPoints->GetNumberOfPoints());

	const double dPointRadius(m_pBoneThicknessTable->sphereRadius());

	QVector<vtkSmartPointer<vtkPoints>> vPoints;
	vPoints.resize(idPointsTable);

	for (vtkIdType i(0); i < idPointsTable; ++i)
	{
		vPoints[i] = vtkPoints::New();
	}

	for (vtkIdType j(0); j < idPointsData; ++j)
	{
		double pPointData[3];
		pPolyData->GetPoint(j, pPointData);

		for (vtkIdType i(0); i < idPointsTable; ++i)
		{
			double pPoint1[3];
			pPoints->GetPoint(i, pPoint1);

			const double Distance(sqrt(vtkMath::Distance2BetweenPoints(pPoint1, pPointData)));

			if (Distance < dPointRadius)
			{
				vPoints[i]->InsertNextPoint(pPointData);
			}
		}
	}

	double pNormal[3];

	for (vtkIdType i(0); i < idPointsTable; ++i)
	{
		getNormal(vPoints[i], pNormal);
		vtkMath::Normalize(pNormal);

		_pPointNormals->InsertNextPoint(pNormal);
	}
}

void iABoneThicknessAttachment::calculate()
{
	vtkSmartPointer<vtkCellLocator> CellLocator(vtkCellLocator::New());
	CellLocator->SetDataSet(m_childData.polyData);
	CellLocator->BuildLocator();
	CellLocator->Update();

	vtkSmartPointer<vtkPoints> Points(m_pBoneThicknessTable->point());
	QVector<vtkSmartPointer<vtkCylinderSource>>* Lines (m_pBoneThicknessTable->lines());

	const vtkIdType idPointsTable (Points->GetNumberOfPoints());

	QVector<double>* pvDistance(m_pBoneThicknessTable->distance());
	QVector<double>* pvThickness(m_pBoneThicknessTable->thickness());
	
	vtkSmartPointer<vtkPoints> PointNormals(vtkPoints::New());
	addPointNormalsIn(PointNormals);

	for (int i(0); i < idPointsTable; ++i)
	{
		double Point1[3];
		Points->GetPoint(i, Point1);

		vtkIdType cellId;
		int subId;
		double closestPointDist2 (0.0);
		double PointClosest1[3];
		CellLocator->FindClosestPoint(Point1, PointClosest1, cellId, subId, closestPointDist2);

		(*pvDistance)[i] = sqrt(closestPointDist2);

		double pNormal[3];
		PointNormals->GetPoint(i, pNormal);

		const double dLength (vtkMath::Max(m_pRange[0], vtkMath::Max(m_pRange[1], m_pRange[2])));
		double pPoint21[3] = { PointClosest1[0] + dLength * pNormal[0], PointClosest1[1] + dLength * pNormal[1], PointClosest1[2] + dLength * pNormal[2] };
		double pPoint22[3] = { PointClosest1[0] - dLength * pNormal[0], PointClosest1[1] - dLength * pNormal[1], PointClosest1[2] - dLength * pNormal[2] };

		double tol (0.0), t (0.0), x1[3], x2[3], pcoords[3];

		if (CellLocator->IntersectWithLine(pPoint21, PointClosest1, tol, t, x1, pcoords, subId))
		{
			(*pvThickness)[i] = sqrt(vtkMath::Distance2BetweenPoints(PointClosest1, x1));

			double pCenter[3] = { 0.5 * (PointClosest1[0] + x1[0]) , 0.5 * (PointClosest1[1] + x1[1]) , 0.5 * (PointClosest1[2] + x1[2]) };
			(*Lines)[i]->SetCenter(pCenter);
			(*Lines)[i]->SetHeight((*pvThickness)[i]);
			(*Lines)[i]->SetRadius(0.05);
		}
		else
		{
			(*pvThickness)[i] = 0.0;

			(*Lines)[i]->SetHeight(0.0);
			(*Lines)[i]->SetRadius(0.0);
		}

		if (CellLocator->IntersectWithLine(pPoint22, PointClosest1, tol, t, x2, pcoords, subId))
		{
			const double dThickness(sqrt(vtkMath::Distance2BetweenPoints(PointClosest1, x2)));

			if (dThickness > (*pvThickness)[i])
			{
				(*pvThickness)[i] = dThickness;

				double pCenter[3] = { 0.5 * (PointClosest1[0] + x2[0]) , 0.5 * (PointClosest1[1] + x2[1]) , 0.5 * (PointClosest1[2] + x2[2]) };
				(*Lines)[i]->SetCenter(pCenter);
				(*Lines)[i]->SetHeight((*pvThickness)[i]);
				(*Lines)[i]->SetRadius(0.05);
			}
		}
	}

	m_pBoneThicknessTable->setTable();
	m_pBoneThicknessTable->setWindow();
}

void iABoneThicknessAttachment::getNormal(vtkPoints* _pPoints, double* _pNormal)
{
	const vtkIdType idPointsArea(_pPoints->GetNumberOfPoints());

	if (idPointsArea > 0)
	{
		double pPoint1[3];
		_pPoints->GetPoint(0, pPoint1);

		double dSumX(pPoint1[0]);
		double dSumXX(pPoint1[0] * pPoint1[0]);
		double dSumXY(pPoint1[0] * pPoint1[1]);

		double dSumY(pPoint1[1]);
		double dSumYY(pPoint1[1] * pPoint1[1]);

		double dSumXZ(pPoint1[0] * pPoint1[2]);
		double dSumYZ(pPoint1[1] * pPoint1[2]);
		double dSumZ(pPoint1[2]);

		for (vtkIdType i(1); i < idPointsArea; ++i)
		{
			_pPoints->GetPoint(i, pPoint1);

			dSumX += pPoint1[0];
			dSumXX += pPoint1[0] * pPoint1[0];
			dSumXY += pPoint1[0] * pPoint1[1];
			dSumXZ += pPoint1[0] * pPoint1[2];

			dSumY += pPoint1[1];
			dSumYY += pPoint1[1] * pPoint1[1];
			dSumYZ += pPoint1[1] * pPoint1[2];

			dSumZ += pPoint1[2];
		}

		const double AB(dSumXX * dSumYY - dSumXY * dSumXY);

		_pNormal[2] = (dSumXZ * dSumYY - dSumYZ * dSumXY) / AB; // A
		_pNormal[0] = (dSumXX * dSumXZ - dSumXZ * dSumXY) / AB; // B
		_pNormal[1] = (dSumZ - _pNormal[0] * dSumX - _pNormal[1] * dSumY) / ((double)idPointsArea); // C
	}
	else
	{
		_pNormal[0] = _pNormal[1] = _pNormal[2] = 0.0;
	}
}

void iABoneThicknessAttachment::slotDoubleSpinBoxSphereRadius(const double& _dValue)
{
	qApp->setOverrideCursor(Qt::WaitCursor);
	m_pBoneThicknessTable->setSphereRadius(_dValue);
	calculate();
	qApp->restoreOverrideCursor();
}

void iABoneThicknessAttachment::slotPushButtonOpen()
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
		qApp->setOverrideCursor(Qt::WaitCursor);
		qApp->processEvents();
		m_pBoneThicknessTable->open(pFileDialog->selectedFiles().first());
		calculate();
		qApp->restoreOverrideCursor();
	}

	delete pFileDialog;
}

void iABoneThicknessAttachment::slotPushButtonSave()
{
	QPushButton* pPushButtonSave((QPushButton*)sender());

	QFileDialog* pFileDialog(new QFileDialog());
	pFileDialog->setAcceptMode(QFileDialog::AcceptSave);
	pFileDialog->setDefaultSuffix("csv");
	pFileDialog->setFileMode(QFileDialog::ExistingFile);
	pFileDialog->setNameFilter("CSV file (*.csv)");
	pFileDialog->setWindowTitle(pPushButtonSave->text());

	if (pFileDialog->exec())
	{
		qApp->setOverrideCursor(Qt::WaitCursor);
		qApp->processEvents();
		m_pBoneThicknessTable->save(pFileDialog->selectedFiles().first());
		qApp->restoreOverrideCursor();
	}

	delete pFileDialog;
}

void iABoneThicknessAttachment::slotCheckBoxTransparency(const bool& _bChecked)
{
	if (_bChecked)
	{
		m_pBoneThicknessTable->setOpacity(0.5);
	}
	else
	{
		m_pBoneThicknessTable->setOpacity(1.0);
	}
}

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

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
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
	m_pRange[2] = m_pBound[5] - m_pBound[4];

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
	QLabel* pLabelBoundXRng(new QLabel(QString("X range: %1").arg(m_pRange[0]), pGroupBoxBound));
	QLabel* pLabelBoundYMin(new QLabel(QString("Y min: %1").arg(m_pBound[2]), pGroupBoxBound));
	QLabel* pLabelBoundYMax(new QLabel(QString("Y max: %1").arg(m_pBound[3]), pGroupBoxBound));
	QLabel* pLabelBoundYRng(new QLabel(QString("Y range: %1").arg(m_pRange[1]), pGroupBoxBound));
	QLabel* pLabelBoundZMin(new QLabel(QString("Z min: %1").arg(m_pBound[4]), pGroupBoxBound));
	QLabel* pLabelBoundZMax(new QLabel(QString("Z max: %1").arg(m_pBound[5]), pGroupBoxBound));
	QLabel* pLabelBoundZRng(new QLabel(QString("Z range: %1").arg(m_pRange[2]), pGroupBoxBound));

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

	const double dRangeMax(vtkMath::Max(m_pRange[0], vtkMath::Max(m_pRange[1], m_pRange[2])));
	const double dRangeMin(vtkMath::Min(m_pRange[0], vtkMath::Min(m_pRange[1], m_pRange[2])));

	m_pBoneThicknessTable = new iABoneThicknessTable(m_childData.child->getRaycaster(), pWidget);
	m_pBoneThicknessTable->setSphereRadius(0.2 * vtkMath::Ceil(0.2 * dRangeMax));
	m_pBoneThicknessTable->setThicknessMaximum(0.5 * vtkMath::Ceil(dRangeMin));

	QGroupBox* pGroupBoxSettings(new QGroupBox("Settings", pWidget));

	QLabel* pLabelSphereRadius(new QLabel("Calculation radius:", pGroupBoxSettings));
	QDoubleSpinBox* pDoubleSpinBoxSphereRadius(new QDoubleSpinBox(pGroupBoxSettings));
	pDoubleSpinBoxSphereRadius->setAlignment(Qt::AlignRight);
	pDoubleSpinBoxSphereRadius->setMinimum(0.01);
	pDoubleSpinBoxSphereRadius->setMaximum(1.0E+6);
	pDoubleSpinBoxSphereRadius->setSingleStep(0.1);
	pDoubleSpinBoxSphereRadius->setValue(m_pBoneThicknessTable->sphereRadius());
	connect(pDoubleSpinBoxSphereRadius, SIGNAL(valueChanged(const double&)), this, SLOT(slotDoubleSpinBoxSphereRadius(const double&)));

	QLabel* pLabelMethod(new QLabel("Calculation method:", pGroupBoxSettings));
	QComboBox* pComboBoxMethod(new QComboBox(pGroupBoxSettings));
	pComboBoxMethod->addItem("Centroid", 0);
	pComboBoxMethod->addItem("Plane fitting from XY", 1);
	connect(pComboBoxMethod, SIGNAL(currentIndexChanged(const int&)), this, SLOT(slotComboBoxMethod(const int&)));

	QLabel* pLabelThicknessMaximum(new QLabel("Maximum thickness:", pGroupBoxSettings));
	QDoubleSpinBox* pDoubleSpinBoxThicknessMaximum(new QDoubleSpinBox(pGroupBoxSettings));
	pDoubleSpinBoxThicknessMaximum->setAlignment(Qt::AlignRight);
	pDoubleSpinBoxThicknessMaximum->setMinimum(0.0);
	pDoubleSpinBoxThicknessMaximum->setSingleStep(1.0);
	pDoubleSpinBoxThicknessMaximum->setValue(m_pBoneThicknessTable->thicknessMaximum());
	connect(pDoubleSpinBoxThicknessMaximum, SIGNAL(valueChanged(const double&)), this, SLOT(slotDoubleSpinBoxThicknessMaximum(const double&)));

	QCheckBox* pCheckBoxTransparency(new QCheckBox("Use transparency", pGroupBoxSettings));
	connect(pCheckBoxTransparency, SIGNAL(clicked(const bool&)), this, SLOT(slotCheckBoxTransparency(const bool&)));

	QCheckBox* pCheckBoxShowThickness(new QCheckBox("Thickness representation", pGroupBoxSettings));
	pCheckBoxShowThickness->setChecked(m_pBoneThicknessTable->showThickness());
	connect(pCheckBoxShowThickness, SIGNAL(clicked(const bool&)), this, SLOT(slotCheckBoxShowThickness(const bool&)));

	QGridLayout* pGridLayoutSettings(new QGridLayout(pGroupBoxSettings));
	pGridLayoutSettings->addWidget(pLabelSphereRadius, 0, 0, Qt::AlignRight);
	pGridLayoutSettings->addWidget(pDoubleSpinBoxSphereRadius, 0, 1, Qt::AlignLeft);
	pGridLayoutSettings->addWidget(pLabelMethod, 0, 2, Qt::AlignRight);
	pGridLayoutSettings->addWidget(pComboBoxMethod, 0, 3, Qt::AlignLeft);
	pGridLayoutSettings->addWidget(pLabelThicknessMaximum, 0, 4, Qt::AlignRight);
	pGridLayoutSettings->addWidget(pDoubleSpinBoxThicknessMaximum, 0, 5, Qt::AlignLeft);
	pGridLayoutSettings->addWidget(pCheckBoxTransparency, 0, 6);
	pGridLayoutSettings->addWidget(pCheckBoxShowThickness, 0, 7);

	QGridLayout* pGridLayout(new QGridLayout(pWidget));
	pGridLayout->addWidget(pPushButtonOpen, 0, 0);
	pGridLayout->addWidget(pPushButtonSave, 0, 1);
	pGridLayout->addWidget(pGroupBoxBound, 1, 0, 1, 2);
	pGridLayout->addWidget(m_pBoneThicknessTable, 2, 0, 1, 2);
	pGridLayout->addWidget(pGroupBoxSettings, 3, 0, 1, 2);

	_iaChildData.child->tabifyDockWidget(_iaChildData.logs, new iADockWidgetWrapper(pWidget, tr("Bone thickness"), "BoneThickness"));
}

void iABoneThicknessAttachment::addNormalsInPoint(vtkPoints* _pPointNormals)
{
	vtkPolyData* pPolyData(m_childData.polyData);
	const vtkIdType idPolyPoints(pPolyData->GetNumberOfPoints());

	vtkSmartPointer<vtkPoints> pPoints(m_pBoneThicknessTable->point());
	const vtkIdType idPoints(pPoints->GetNumberOfPoints());

	const double dPointRadius(m_pBoneThicknessTable->sphereRadius());

	QVector<vtkSmartPointer<vtkPoints>> vPoints;
	vPoints.resize(idPoints);

	for (vtkIdType i(0); i < idPoints; ++i)
	{
		vPoints[i] = vtkPoints::New();
	}

	for (vtkIdType j(0); j < idPolyPoints; ++j)
	{
		double pPolyPoint[3];
		pPolyData->GetPoint(j, pPolyPoint);

		for (vtkIdType i(0); i < idPoints; ++i)
		{
			double pPoint[3];
			pPoints->GetPoint(i, pPoint);

			const double Distance(sqrt(vtkMath::Distance2BetweenPoints(pPoint, pPolyPoint)));

			if (Distance < dPointRadius)
			{
				vPoints[i]->InsertNextPoint(pPolyPoint);
			}
		}
	}

	double pNormal[3] = { 0.0 , 0.0 , 0.0 };

	if (m_eMethod == eCentroid)
	{
		for (vtkIdType i(0); i < idPoints; ++i)
		{
			double pCenter[3] = { 0.0 , 0.0 , 0.0 };

			if (getCenterFromPoints(vPoints[i], pCenter))
			{
				double pPoint[3];
				pPoints->GetPoint(i, pPoint);

				pNormal[0] = pPoint[0] - pCenter[0];
				pNormal[1] = pPoint[1] - pCenter[1];
				pNormal[2] = pPoint[2] - pCenter[2];
				vtkMath::Normalize(pNormal);
			}
			else
			{
				pNormal[0] = pNormal[1] = pNormal[2] = 0.0;
			}

			_pPointNormals->InsertNextPoint(pNormal);
		}
	}
	else
	{
		for (vtkIdType i(0); i < idPoints; ++i)
		{
			if (getNormalFromPoints(vPoints[i], pNormal))
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

void iABoneThicknessAttachment::calculate()
{
	vtkSmartPointer<vtkCellLocator> CellLocator(vtkCellLocator::New());
	CellLocator->SetDataSet(m_childData.polyData);
	CellLocator->BuildLocator();
	CellLocator->Update();

	vtkSmartPointer<vtkPoints> Points(m_pBoneThicknessTable->point());
	QVector<vtkSmartPointer<vtkLineSource>>* Lines (m_pBoneThicknessTable->lines());

	const vtkIdType idPoints (Points->GetNumberOfPoints());

	QVector<double>* pvDistance(m_pBoneThicknessTable->distance());

	vtkSmartPointer<vtkPoints> PointNormals(vtkPoints::New());
	addNormalsInPoint(PointNormals);

	const double dLength(0.5 * vtkMath::Max(m_pRange[0], vtkMath::Max(m_pRange[1], m_pRange[2])));

	double tol(0.0), t(0.0), pcoords[3];

	for (int i(0); i < idPoints; ++i)
	{
		double pPoint[3];
		Points->GetPoint(i, pPoint);

		vtkIdType cellId;
		int subId;
		double closestPointDist2 (0.0);
		double PointClosest1[3];
		CellLocator->FindClosestPoint(pPoint, PointClosest1, cellId, subId, closestPointDist2);

		(*pvDistance)[i] = sqrt(closestPointDist2);

		double pNormal[3];
		PointNormals->GetPoint(i, pNormal);
		if ((pNormal[0] == 0.0) && (pNormal[1] == 0.0) && (pNormal[2] == 0.0))
		{
			m_pBoneThicknessTable->setThickness(i, 0.0);

			(*Lines)[i]->SetPoint1(pNormal);
			(*Lines)[i]->SetPoint2(pNormal);

			continue;
		}

		double pPoint21[3] = { PointClosest1[0] + dLength * pNormal[0], PointClosest1[1] + dLength * pNormal[1], PointClosest1[2] + dLength * pNormal[2] };
		double pPoint22[3] = { PointClosest1[0] - dLength * pNormal[0], PointClosest1[1] - dLength * pNormal[1], PointClosest1[2] - dLength * pNormal[2] };

		double x1[3] = { 0.0, 0.0, 0.0 };

		double dThickness1(0.0);

		if (CellLocator->IntersectWithLine(pPoint21, pPoint, tol, t, x1, pcoords, subId))
		{
			dThickness1 = sqrt(vtkMath::Distance2BetweenPoints(pPoint, x1));

			m_pBoneThicknessTable->setThickness(i, dThickness1);

			(*Lines)[i]->SetPoint1(pPoint);
			(*Lines)[i]->SetPoint2(x1);
		}
		else
		{
			(*Lines)[i]->SetPoint1(x1);
			(*Lines)[i]->SetPoint2(x1);
		}

		double x2[3] = { 0.0, 0.0, 0.0 };

		if (CellLocator->IntersectWithLine(pPoint22, pPoint, tol, t, x2, pcoords, subId))
		{
			const double dThickness2(sqrt(vtkMath::Distance2BetweenPoints(pPoint, x2)));

			if (dThickness2 > dThickness1)
			{
				m_pBoneThicknessTable->setThickness(i, dThickness2);

				(*Lines)[i]->SetPoint1(pPoint);
				(*Lines)[i]->SetPoint2(x2);
			}
		}
	}

	m_pBoneThicknessTable->setTable();
	m_pBoneThicknessTable->setWindow();
}

bool iABoneThicknessAttachment::getCenterFromPoints(vtkPoints* _pPoints, double* _pCenter)
{
	const vtkIdType idPoints(_pPoints->GetNumberOfPoints());

	if (idPoints > 1)
	{
		_pPoints->GetPoint(0, _pCenter);

		for (vtkIdType i (1) ; i < idPoints; ++i)
		{
			double pPoint[3];
			_pPoints->GetPoint(i, pPoint);

			_pCenter[0] += pPoint[0];
			_pCenter[1] += pPoint[1];
			_pCenter[2] += pPoint[2];
		}

		const double dPoints ((double) idPoints);

		_pCenter[0] /= dPoints;
		_pCenter[1] /= dPoints;
		_pCenter[2] /= dPoints;

		return true;
	}
	else
	{
		return false;
	}
}

bool iABoneThicknessAttachment::getNormalFromPoints(vtkPoints* _pPoints, double* _pNormal)
{
	double pCenter[3];

	if (getCenterFromPoints(_pPoints, pCenter))
	{
		const vtkIdType idPoints(_pPoints->GetNumberOfPoints());

		if (idPoints > 3)
		{
			// With coordinates from centroid, Plane: Z = a * X + b * Y + D

			double pPoint[3];
			_pPoints->GetPoint(0, pPoint);

			double pPoint0[3] = { pPoint[0] - pCenter[0], pPoint[1] - pCenter[1], pPoint[2] - pCenter[2] };

			double dSumXX(pPoint0[0] * pPoint0[0]);
			double dSumXY(pPoint0[0] * pPoint0[1]);
			double dSumXZ(pPoint0[0] * pPoint0[2]);
			double dSumYY(pPoint0[1] * pPoint0[1]);
			double dSumYZ(pPoint0[1] * pPoint0[2]);

			for (vtkIdType i(0); i < idPoints; ++i)
			{
				_pPoints->GetPoint(i, pPoint);

				pPoint0[0] = pPoint[0] - pCenter[0];
				pPoint0[1] = pPoint[1] - pCenter[1];
				pPoint0[2] = pPoint[2] - pCenter[2];

				dSumXX += pPoint0[0] * pPoint0[0];
				dSumXY += pPoint0[0] * pPoint0[1];
				dSumXZ += pPoint0[0] * pPoint0[2];
				dSumYY += pPoint0[1] * pPoint0[1];
				dSumYZ += pPoint0[1] * pPoint0[2];
			}

			const double D(dSumXX * dSumYY - dSumXY * dSumXY);
			const double a((dSumYZ * dSumXY - dSumXZ * dSumYY) / D);
			const double b((dSumXY * dSumXZ - dSumXX * dSumYZ) / D);

			_pNormal[0] = a;
			_pNormal[1] = b;
			_pNormal[2] = -1.0;

			return true;
		}
	}

	return false;
}

void iABoneThicknessAttachment::slotDoubleSpinBoxSphereRadius(const double& _dValue)
{
	qApp->setOverrideCursor(Qt::WaitCursor);
	m_pBoneThicknessTable->setSphereRadius(_dValue);
	calculate();
	qApp->restoreOverrideCursor();
}

void iABoneThicknessAttachment::slotDoubleSpinBoxThicknessMaximum(const double& _dValue)
{
	qApp->setOverrideCursor(Qt::WaitCursor);
	m_pBoneThicknessTable->setThicknessMaximum(_dValue);
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

void iABoneThicknessAttachment::slotCheckBoxShowThickness(const bool& _bChecked)
{
	m_pBoneThicknessTable->setShowThickness(_bChecked);
	m_pBoneThicknessTable->setWindowSpheres();
	m_childData.child->getRaycaster()->update();
}

void iABoneThicknessAttachment::slotCheckBoxShowLines(const bool& _bChecked)
{
	m_pBoneThicknessTable->setShowThicknessLines(_bChecked);
	m_pBoneThicknessTable->setWindowThicknessLines();
	m_childData.child->getRaycaster()->update();
}

void iABoneThicknessAttachment::slotCheckBoxTransparency(const bool& _bChecked)
{
	m_pBoneThicknessTable->setTransparency(_bChecked);
}

void iABoneThicknessAttachment::slotComboBoxMethod(const int& _iIndex)
{
	qApp->setOverrideCursor(Qt::WaitCursor);
	qApp->processEvents();
	m_eMethod = (EMethod) _iIndex;
	calculate();
	qApp->restoreOverrideCursor();
}

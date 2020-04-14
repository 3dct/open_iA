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
#include "iAFoamCharacterizationDialogAnalysis.h"

#include "iAFoamCharacterizationTableAnalysis.h"

#include <iAConnector.h>

#include <itkLabelMap.h>
#include <itkLabelGeometryImageFilter.h>
#include <itkLabelImageToShapeLabelMapFilter.h>
#include <itkShapeLabelObject.h>

#include <vtkImageData.h>

#include <QApplication>
#include <QGridLayout>
#include <QStyle>
#include <QPushButton>
#include <QStandardItemModel>
#include <QtMath>

iAFoamCharacterizationDialogAnalysis::iAFoamCharacterizationDialogAnalysis(vtkImageData* _pImageData, QWidget* _pParent)
																									  : QDialog(_pParent)
																									  , m_pImageData (_pImageData)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::WindowMaximizeButtonHint);
	setWindowTitle("Analysis");

	m_pTable = new iAFoamCharacterizationTableAnalysis(this);

	analyse();

	QGridLayout* pGridLayout(new QGridLayout(this));
	pGridLayout->addWidget(m_pTable);
}

void iAFoamCharacterizationDialogAnalysis::analyse()
{
	qApp->setOverrideCursor(Qt::WaitCursor);
	qApp->processEvents();

	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->setImage(m_pImageData);

	typedef itk::LabelGeometryImageFilter<itk::Image<unsigned short, 3>> itkLabelGeometryImageFilterType;
	itkLabelGeometryImageFilterType::Pointer pLabelGeometryImageFilter(itkLabelGeometryImageFilterType::New());
	pLabelGeometryImageFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->itkImage()));
	pLabelGeometryImageFilter->Update();

	const int iLabels(pLabelGeometryImageFilter->GetNumberOfLabels());

	m_pTable->setRowCount(iLabels - 1);

	itkLabelGeometryImageFilterType::LabelsType ltLabels (pLabelGeometryImageFilter->GetLabels());

	int i (0);

	for ( itkLabelGeometryImageFilterType::LabelsType::iterator ltLabelsIt (ltLabels.begin())
		; ltLabelsIt != ltLabels.end() ; ++ltLabelsIt
		)
	{
		itkLabelGeometryImageFilterType::LabelPixelType ltLabel (*ltLabelsIt);

		if (ltLabel > 1)
		{
			const itkLabelGeometryImageFilterType::LabelPointType lptCenter(pLabelGeometryImageFilter->GetCentroid(ltLabel));
			const double* pCenter((double*)lptCenter.GetDataPointer());

			const double dVolume ((double)pLabelGeometryImageFilter->GetVolume(ltLabel));
			const double dDiameter (2.0 * qPow(3.0 * dVolume / M_PI / 4.0, 1.0 / 3.0));

			const itk::FixedArray<itk::Index<3>::IndexValueType, 6> pBoundingBox
																			 (pLabelGeometryImageFilter->GetBoundingBox(ltLabel));
			m_pTable->setRow ( i++
				             , ltLabel
				             , pCenter[0], pCenter[1], pCenter[2]
				             , dVolume, dDiameter
							 , pBoundingBox
							 );
		}
	}

	qApp->restoreOverrideCursor();
}

QSize iAFoamCharacterizationDialogAnalysis::sizeHint() const
{
	return QSize(6 * logicalDpiX(), 6 * logicalDpiY());
}

void iAFoamCharacterizationDialogAnalysis::slotPushButtonOk()
{
	accept();
}

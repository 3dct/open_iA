// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFoamCharacterizationDialogAnalysis.h"

#include "iAFoamCharacterizationTableAnalysis.h"

#include <iAImageData.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkLabelMap.h>
#include <itkLabelGeometryImageFilter.h>
#include <itkLabelImageToShapeLabelMapFilter.h>
#include <itkShapeLabelObject.h>
#ifdef __clang__
#pragma clang diagnostic push
#endif

#include <vtkMath.h>

#include <QApplication>
#include <QGridLayout>
#include <QStyle>
#include <QPushButton>
#include <QStandardItemModel>
#include <QtMath>

iAFoamCharacterizationDialogAnalysis::iAFoamCharacterizationDialogAnalysis(iAImageData const* dataSet, QWidget* _pParent) : QDialog(_pParent)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint | Qt::WindowMaximizeButtonHint));
	setWindowTitle("Analysis");

	m_pTable = new iAFoamCharacterizationTableAnalysis(this);

	analyse(dataSet);

	QGridLayout* pGridLayout(new QGridLayout(this));
	pGridLayout->addWidget(m_pTable);
}

void iAFoamCharacterizationDialogAnalysis::analyse(iAImageData const* dataSet)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();

	typedef itk::LabelGeometryImageFilter<itk::Image<unsigned short, 3>> itkLabelGeometryImageFilterType;
	itkLabelGeometryImageFilterType::Pointer pLabelGeometryImageFilter(itkLabelGeometryImageFilterType::New());
	pLabelGeometryImageFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (dataSet->itkImage()));
	pLabelGeometryImageFilter->Update();

	const int iLabels(pLabelGeometryImageFilter->GetNumberOfLabels());

	m_pTable->setRowCount(iLabels - 1);

	auto ltLabels = pLabelGeometryImageFilter->GetLabels();

	int i (0);

	for (auto ltLabelsIt (ltLabels.begin())
		; ltLabelsIt != ltLabels.end() ; ++ltLabelsIt
		)
	{
		itkLabelGeometryImageFilterType::LabelPixelType ltLabel (*ltLabelsIt);

		if (ltLabel > 1)
		{
			auto lptCenter(pLabelGeometryImageFilter->GetCentroid(ltLabel));
			const double* pCenter((double*)lptCenter.GetDataPointer());

			const double dVolume ((double)pLabelGeometryImageFilter->GetVolume(ltLabel));
			const double dDiameter (2.0 * qPow(3.0 * dVolume / vtkMath::Pi() / 4.0, 1.0 / 3.0));

			const auto pBoundingBox(pLabelGeometryImageFilter->GetBoundingBox(ltLabel));
			m_pTable->setRow ( i++
				             , ltLabel
				             , pCenter[0], pCenter[1], pCenter[2]
				             , dVolume, dDiameter
							 , pBoundingBox
							 );
		}
	}

	QApplication::restoreOverrideCursor();
}

QSize iAFoamCharacterizationDialogAnalysis::sizeHint() const
{
	return QSize(6 * logicalDpiX(), 6 * logicalDpiY());
}

void iAFoamCharacterizationDialogAnalysis::slotPushButtonOk()
{
	accept();
}

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFoamCharacterizationDialogAnalysis.h"

#include "iAFoamCharacterizationTableAnalysis.h"

#include <iAImageData.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkLabelMap.h>
#include <itkLabelImageToShapeLabelMapFilter.h>
#include <itkShapeLabelObject.h>
#ifdef __clang__
#pragma clang diagnostic push
#endif

#include <vtkMath.h>

#include <QApplication>
#include <QGridLayout>

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

	auto labelGeometryFilter = itk::LabelImageToShapeLabelMapFilter<itk::Image<unsigned short, 3>>::New();
	labelGeometryFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (dataSet->itkImage()));
	labelGeometryFilter->Update();
	auto labelMap = labelGeometryFilter->GetOutput();
	const itk::SizeValueType iLabels = labelMap->GetNumberOfLabelObjects();
	m_pTable->setRowCount(iLabels);
	for (itk::SizeValueType labelValue = 0; labelValue < iLabels; ++labelValue)
	{
		auto labelObject = labelMap->GetNthLabelObject(labelValue);
		auto lptCenter = labelObject->GetCentroid();
		const double* pCenter = lptCenter.GetDataPointer();
		const double dVolume = labelObject->GetPhysicalSize();
		const double dDiameter (2.0 * qPow(3.0 * dVolume / vtkMath::Pi() / 4.0, 1.0 / 3.0));
		const auto bb = labelObject->GetBoundingBox();
		std::array<int64_t, 3> bbOrigin = { bb.GetIndex()[0], bb.GetIndex()[1], bb.GetIndex()[2] };
		std::array<uint64_t, 3> bbSize = { bb.GetSize()[0] , bb.GetSize()[1], bb.GetSize()[2] };
		m_pTable->setRow(labelValue, labelValue + 1, pCenter[0], pCenter[1], pCenter[2], dVolume, dDiameter, bbOrigin, bbSize);
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

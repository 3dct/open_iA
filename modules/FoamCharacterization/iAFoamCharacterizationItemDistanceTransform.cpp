// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFoamCharacterizationItemDistanceTransform.h"

#include "iAFoamCharacterizationItemBinarization.h"
#include "iAFoamCharacterizationDialogDistanceTransform.h"

#include <iAImageData.h>
#include <iAProgress.h>

#include <itkDanielssonDistanceMapImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>

#include <vtkImageData.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QFile>

iAFoamCharacterizationItemDistanceTransform::iAFoamCharacterizationItemDistanceTransform(
	iAFoamCharacterizationTable* _pTable) :
	iAFoamCharacterizationItem(_pTable, iAFoamCharacterizationItem::itDistanceTransform)
{
}

iAFoamCharacterizationItemDistanceTransform::iAFoamCharacterizationItemDistanceTransform(
	iAFoamCharacterizationItemDistanceTransform* _pDistanceTransform) :
	iAFoamCharacterizationItem(_pDistanceTransform)
{
	setName(_pDistanceTransform->name());

	m_bImageSpacing = _pDistanceTransform->useImageSpacing();

	m_iItemMask = _pDistanceTransform->itemMask();
}

void iAFoamCharacterizationItemDistanceTransform::dialog()
{
	QScopedPointer<iAFoamCharacterizationDialogDistanceTransform> pDialog
											       (new iAFoamCharacterizationDialogDistanceTransform(this, QApplication::focusWidget()));
	pDialog->exec();
}

std::shared_ptr<iADataSet> iAFoamCharacterizationItemDistanceTransform::execute(std::shared_ptr<iADataSet> dataSet)
{
	setExecuting(true);

	QElapsedTimer t;
	t.start();

	typedef itk::DanielssonDistanceMapImageFilter<itk::Image<unsigned short, 3>, itk::Image<float, 3>> itkFilter;
	itkFilter::Pointer pFilter(itkFilter::New());
	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*>(dynamic_cast<iAImageData*>(dataSet.get())->itkImage()));
	pFilter->SetUseImageSpacing(m_bImageSpacing);
	pFilter->InputIsBinaryOn();

	QScopedPointer<iAProgress> pObserver(new iAProgress());
	pObserver->observe(pFilter);
	connect(pObserver.data(), &iAProgress::progress, this, &iAFoamCharacterizationItemDistanceTransform::slotObserver);

	pFilter->Update();

	typedef itk::MinimumMaximumImageCalculator<itk::Image<float, 3>> itkCalculator;
	itkCalculator::Pointer pCalculator(itkCalculator::New());
	pCalculator->SetImage(pFilter->GetOutput());
	pCalculator->ComputeMaximum();

	typedef itk::InvertIntensityImageFilter<itk::Image<float, 3>, itk::Image<float, 3>> itkInvert;
	itkInvert::Pointer pInvert (itkInvert::New());
	pInvert->SetInput(pFilter->GetOutput());
	pInvert->SetMaximum(pCalculator->GetMaximum());
	pInvert->Update();

	if (m_iItemMask > -1)
	{
		//if (m_pItemMask->isMask())
		{

		}
	}

	m_dExecuteTime = 0.001 * (double) t.elapsed();

	setExecuting(false);
	return std::make_shared<iAImageData>(pInvert->GetOutput());
}

int iAFoamCharacterizationItemDistanceTransform::itemMask() const
{
	return m_iItemMask;
}

void iAFoamCharacterizationItemDistanceTransform::open(QFile* _pFileOpen)
{
	iAFoamCharacterizationItem::open(_pFileOpen);

	_pFileOpen->read((char*)&m_bImageSpacing, sizeof(m_bImageSpacing));
	_pFileOpen->read((char*)&m_iItemMask, sizeof(m_iItemMask));

	setItemText();
}

void iAFoamCharacterizationItemDistanceTransform::save(QFile* _pFileSave)
{
	iAFoamCharacterizationItem::save(_pFileSave);

	_pFileSave->write((char*)&m_bImageSpacing, sizeof(m_bImageSpacing));
	_pFileSave->write((char*)&m_iItemMask, sizeof(m_iItemMask));
}

void iAFoamCharacterizationItemDistanceTransform::setItemMask(const int& _iItemMask)
{
	m_iItemMask = _iItemMask;
}

void iAFoamCharacterizationItemDistanceTransform::setUseImageSpacing(const bool& _bImageSpacing)
{
	m_bImageSpacing = _bImageSpacing;
}

bool iAFoamCharacterizationItemDistanceTransform::useImageSpacing() const
{
	return m_bImageSpacing;
}

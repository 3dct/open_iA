/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAFoamCharacterizationItemDistanceTransform.h"

#include "iAFoamCharacterizationItemBinarization.h"
#include "iAFoamCharacterizationDialogDistanceTransform.h"

#include "iAConnector.h"
#include "iAProgress.h"

#include <itkDanielssonDistanceMapImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>

#include <vtkImageData.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QFile>

iAFoamCharacterizationItemDistanceTransform::iAFoamCharacterizationItemDistanceTransform
																 (iAFoamCharacterizationTable* _pTable, vtkImageData* _pImageData)
							   : iAFoamCharacterizationItem(_pTable ,_pImageData, iAFoamCharacterizationItem::itDistanceTransform)
{

}

iAFoamCharacterizationItemDistanceTransform::iAFoamCharacterizationItemDistanceTransform
																(iAFoamCharacterizationItemDistanceTransform* _pDistanceTransform)
																: iAFoamCharacterizationItem(_pDistanceTransform)
{
	setName(_pDistanceTransform->name());

	m_bImageSpacing = _pDistanceTransform->useImageSpacing();

	m_iItemMask = _pDistanceTransform->itemMask();
}

void iAFoamCharacterizationItemDistanceTransform::dialog()
{
	QScopedPointer<iAFoamCharacterizationDialogDistanceTransform> pDialog
											       (new iAFoamCharacterizationDialogDistanceTransform(this, qApp->focusWidget()));
	pDialog->exec();
}

void iAFoamCharacterizationItemDistanceTransform::execute()
{
	setExecuting(true);

	QElapsedTimer t;
	t.start();

	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->setImage(m_pImageData);

	typedef itk::DanielssonDistanceMapImageFilter<itk::Image<unsigned short, 3>, itk::Image<float, 3>> itkFilter;
	itkFilter::Pointer pFilter(itkFilter::New());
	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->itkImage()));
	pFilter->SetUseImageSpacing(m_bImageSpacing);
	pFilter->InputIsBinaryOn();

	QScopedPointer<iAProgress> pObserver(new iAProgress());
	pObserver->observe(pFilter);
	connect(pObserver.data(), SIGNAL(progress(const int&)), this, SLOT(slotObserver(const int&)));

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

	pConnector->setImage(pInvert->GetOutput());

	if (m_iItemMask > -1)
	{
		//if (m_pItemMask->isMask())
		{

		}
	}

	m_pImageData->DeepCopy(pConnector->vtkImage());
	m_pImageData->CopyInformationFromPipeline(pConnector->vtkImage()->GetInformation());

	m_dExecuteTime = 0.001 * (double) t.elapsed();

	setExecuting(false);
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

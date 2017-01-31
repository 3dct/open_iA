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

#include "iAFoamCharacterizationItemBinarization.h"

#include <QApplication>
#include <QFile>
#include <QTime>

#include <itkBinaryThresholdImageFilter.h>
#include <itkOtsuThresholdImageFilter.h>

#include "iAConnector.h"
#include "iAProgress.h"

#include "iAFoamCharacterizationDialogBinarization.h"

iAFoamCharacterizationItemBinarization::iAFoamCharacterizationItemBinarization
																 (iAFoamCharacterizationTable* _pTable, vtkImageData* _pImageData)
	                                : iAFoamCharacterizationItem(_pTable, _pImageData, iAFoamCharacterizationItem::itBinarization)
{
	m_pImageDataMask = vtkSmartPointer<vtkImageData>::New();
}

iAFoamCharacterizationItemBinarization::iAFoamCharacterizationItemBinarization
                                                                          (iAFoamCharacterizationItemBinarization* _pBinarization)
	                                                                                  : iAFoamCharacterizationItem(_pBinarization)
{
	setName(_pBinarization->name());

	m_eItemFilterType = _pBinarization->itemFilterType();

	m_usLowerThreshold = _pBinarization->lowerThreshold();
	m_usUpperThreshold = _pBinarization->upperThreshold();
	m_uiOtzuHistogramBins = _pBinarization->otzuHistogramBins();

	m_bIsMask = _pBinarization->isMask();

	m_pImageDataMask = vtkSmartPointer<vtkImageData>::New();
	m_pImageDataMask->DeepCopy(_pBinarization->imageDataMask());
	m_pImageDataMask->CopyInformationFromPipeline(_pBinarization->imageDataMask()->GetInformation());
}

void iAFoamCharacterizationItemBinarization::dialog()
{
	QScopedPointer<iAFoamCharacterizationDialogBinarization> pDialog
	                                                    (new iAFoamCharacterizationDialogBinarization(this, qApp->focusWidget()));
	pDialog->exec();
}

void iAFoamCharacterizationItemBinarization::execute()
{
	setExecuting(true);

	QTime t;
	t.start();

	switch (m_eItemFilterType)
	{
		case iftBinarization:
		executeBinarization();
		break;

		default:
		executeOtzu(); 
		break;
	}

	if (m_bIsMask)
	{
		m_pImageDataMask->DeepCopy(m_pImageData);
		m_pImageDataMask->CopyInformationFromPipeline(m_pImageData->GetInformation());
	}

	m_dExecuteTime = 0.001 * (double)t.elapsed();

	setExecuting(false);
}

void iAFoamCharacterizationItemBinarization::executeBinarization()
{
	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->SetImage(m_pImageData);

	typedef itk::BinaryThresholdImageFilter<itk::Image<unsigned short, 3>, itk::Image<unsigned short, 3>> itkFilter;
	itkFilter::Pointer pFilter(itkFilter::New());

	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->GetITKImage()));
	pFilter->SetLowerThreshold(m_usLowerThreshold);
	pFilter->SetUpperThreshold(m_usUpperThreshold);
	pFilter->SetInsideValue(0);
	pFilter->SetOutsideValue(1);
	
	QScopedPointer<iAProgress> pObserver(new iAProgress());
	pObserver->Observe(pFilter);
	connect(pObserver.data(), SIGNAL(pprogress(const int&)), this, SLOT(slotObserver(const int&)));

	pFilter->Update();

	pConnector->SetImage(pFilter->GetOutput());

	m_pImageData->DeepCopy(pConnector->GetVTKImage());
	m_pImageData->CopyInformationFromPipeline(pConnector->GetVTKImage()->GetInformation());
}

void iAFoamCharacterizationItemBinarization::executeOtzu()
{
	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->SetImage(m_pImageData);

	typedef itk::OtsuThresholdImageFilter<itk::Image<unsigned short, 3>, itk::Image<unsigned short, 3>> itkFilter;
	itkFilter::Pointer pFilter(itkFilter::New());
	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->GetITKImage()));
	pFilter->SetInsideValue(0);
	pFilter->SetOutsideValue(1);
	pFilter->SetNumberOfHistogramBins(m_uiOtzuHistogramBins);
	
	QScopedPointer<iAProgress> pObserver(new iAProgress());
	pObserver->Observe(pFilter);
	connect(pObserver.data(), SIGNAL(pprogress(const int&)), this, SLOT(slotObserver(const int&)));

	pFilter->Update();

	pConnector->SetImage(pFilter->GetOutput());

	m_pImageData->DeepCopy(pConnector->GetVTKImage());
	m_pImageData->CopyInformationFromPipeline(pConnector->GetVTKImage()->GetInformation());
}

iAFoamCharacterizationItemBinarization::EItemFilterType iAFoamCharacterizationItemBinarization::itemFilterType() const
{
	return m_eItemFilterType;
}

QString iAFoamCharacterizationItemBinarization::itemFilterTypeString() const
{
	switch (m_eItemFilterType)
	{
		case iftBinarization:
		return "B";
		break;

		default:
		return "O";
		break;
	}
}

vtkImageData* iAFoamCharacterizationItemBinarization::imageDataMask()
{
	return m_pImageDataMask.Get();
}

bool iAFoamCharacterizationItemBinarization::isMask() const
{
	return m_bIsMask;
}

unsigned short iAFoamCharacterizationItemBinarization::lowerThreshold() const
{
	return m_usLowerThreshold;
}

void iAFoamCharacterizationItemBinarization::open(QFile* _pFileOpen)
{
	iAFoamCharacterizationItem::open(_pFileOpen);

	_pFileOpen->read((char*)&m_eItemFilterType, sizeof(m_eItemFilterType));
	_pFileOpen->read((char*)&m_usLowerThreshold, sizeof(m_usLowerThreshold));
	_pFileOpen->read((char*)&m_usUpperThreshold, sizeof(m_usUpperThreshold));
	_pFileOpen->read((char*)&m_uiOtzuHistogramBins, sizeof(m_uiOtzuHistogramBins));
	_pFileOpen->read((char*)&m_bIsMask, sizeof(m_bIsMask));
	
	setItemText();
}

unsigned int iAFoamCharacterizationItemBinarization::otzuHistogramBins() const
{
	return m_uiOtzuHistogramBins;
}

void iAFoamCharacterizationItemBinarization::save(QFile* _pFileSave)
{
	iAFoamCharacterizationItem::save(_pFileSave);

	_pFileSave->write((char*)&m_eItemFilterType, sizeof(m_eItemFilterType));
	_pFileSave->write((char*)&m_usLowerThreshold, sizeof(m_usLowerThreshold));
	_pFileSave->write((char*)&m_usUpperThreshold, sizeof(m_usUpperThreshold));
	_pFileSave->write((char*)&m_uiOtzuHistogramBins, sizeof(m_uiOtzuHistogramBins));
	_pFileSave->write((char*)&m_bIsMask, sizeof(m_bIsMask));
}

void iAFoamCharacterizationItemBinarization::setItemFilterType
                                                 (const iAFoamCharacterizationItemBinarization::EItemFilterType& _eItemFilterType)
{
	m_eItemFilterType = _eItemFilterType;
}

void iAFoamCharacterizationItemBinarization::setItemText()
{
	setText(m_sName + QString(" [%1]").arg(itemFilterTypeString()));
}

void iAFoamCharacterizationItemBinarization::setIsMask(const bool& _bIsMask)
{
	m_bIsMask = _bIsMask;
}

void iAFoamCharacterizationItemBinarization::setLowerThreshold(const unsigned short& _usLowerThreshold)
{
	m_usLowerThreshold = _usLowerThreshold;
}

void iAFoamCharacterizationItemBinarization::setOtzuHistogramBins(const unsigned int& _uiOtzuHistogramBins)
{
	m_uiOtzuHistogramBins = _uiOtzuHistogramBins;
}

void iAFoamCharacterizationItemBinarization::setUpperThreshold(const unsigned short& _usUpperThreshold)
{
	m_usUpperThreshold = _usUpperThreshold;
}

unsigned short iAFoamCharacterizationItemBinarization::upperThreshold() const
{
	return m_usUpperThreshold;
}

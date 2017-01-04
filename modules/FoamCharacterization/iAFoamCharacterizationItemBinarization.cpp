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

#include "iAFoamCharacterizationDialogBinarization.h"

iAFoamCharacterizationItemBinarization::iAFoamCharacterizationItemBinarization(vtkImageData* _pImageData)
	                                         : iAFoamCharacterizationItem(_pImageData, iAFoamCharacterizationItem::itBinarization)
{

}

iAFoamCharacterizationItemBinarization::iAFoamCharacterizationItemBinarization
                                                                          (iAFoamCharacterizationItemBinarization* _pBinarization)
	                                                                                  : iAFoamCharacterizationItem(_pBinarization)
{
	setName(_pBinarization->name());
}

void iAFoamCharacterizationItemBinarization::dialog()
{
	QScopedPointer<iAFoamCharacterizationDialogBinarization> pDialog
	                                                    (new iAFoamCharacterizationDialogBinarization(this, qApp->focusWidget()));
	pDialog->exec();
	pDialog.reset();
}

void iAFoamCharacterizationItemBinarization::execute()
{
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

	m_dExecuteTime = 0.001 * (double)t.elapsed();

	setItemText();
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

	setItemText();
}

void iAFoamCharacterizationItemBinarization::save(QFile* _pFileSave)
{
	iAFoamCharacterizationItem::save(_pFileSave);

	_pFileSave->write((char*)&m_eItemFilterType, sizeof(m_eItemFilterType));
	_pFileSave->write((char*)&m_usLowerThreshold, sizeof(m_usLowerThreshold));
	_pFileSave->write((char*)&m_usUpperThreshold, sizeof(m_usUpperThreshold));
}

void iAFoamCharacterizationItemBinarization::setItemFilterType
                                                 (const iAFoamCharacterizationItemBinarization::EItemFilterType& _eItemFilterType)
{
	m_eItemFilterType = _eItemFilterType;
}

void iAFoamCharacterizationItemBinarization::setItemText()
{
	if (m_dExecuteTime > 0.0)
	{
		setText(m_sName + QString(" [%1] (%2)").arg(itemFilterTypeString()).arg(executeTimeString()));
	}
	else
	{
		setText(m_sName + QString(" [%1]").arg(itemFilterTypeString()));
	}
}

void iAFoamCharacterizationItemBinarization::setLowerThreshold(const unsigned short& _usLowerThreshold)
{
	m_usLowerThreshold = _usLowerThreshold;
}

void iAFoamCharacterizationItemBinarization::setUpperThreshold(const unsigned short& _usUpperThreshold)
{
	m_usUpperThreshold = _usUpperThreshold;
}

unsigned short iAFoamCharacterizationItemBinarization::upperThreshold() const
{
	return m_usUpperThreshold;
}

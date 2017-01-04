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

#include "iAFoamCharacterizationItemWatershed.h"

#include <QApplication>
#include <QFile>
#include <QTime>

#include <itkGradientMagnitudeImageFilter.h>
#include <itkWatershedImageFilter.h>

#include "iAConnector.h"

#include "iAFoamCharacterizationDialogWatershed.h"

iAFoamCharacterizationItemWatershed::iAFoamCharacterizationItemWatershed(vtkImageData* _pImageData)
	                                            : iAFoamCharacterizationItem(_pImageData, iAFoamCharacterizationItem::itWatershed)
{

}

iAFoamCharacterizationItemWatershed::iAFoamCharacterizationItemWatershed(iAFoamCharacterizationItemWatershed* _pWatershed)
	                                                                                     : iAFoamCharacterizationItem(_pWatershed)
{
	setName(_pWatershed->name());
}

void iAFoamCharacterizationItemWatershed::dialog()
{
	QScopedPointer<iAFoamCharacterizationDialogWatershed> pDialog
	                                                      (new iAFoamCharacterizationDialogWatershed(this, qApp->focusWidget()));
	pDialog->exec();
	pDialog.reset();
}

void iAFoamCharacterizationItemWatershed::execute()
{
	QTime t;
	t.start();

	QScopedPointer<iAConnector> pConnector (new iAConnector());
	pConnector->SetImage(m_pImageData);

	typedef itk::GradientMagnitudeImageFilter<itk::Image<unsigned short, 3>, itk::Image<unsigned short, 3>> itkGradient;

	itkGradient::Pointer pGradient (itkGradient::New());
	pGradient->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->GetITKImage()));
	pGradient->Update();

	typedef itk::WatershedImageFilter<itk::Image<unsigned short, 3>> itkWatershed;

	itkWatershed::Pointer pWatershed (itkWatershed::New());
	pWatershed->SetInput(pGradient->GetOutput());
	pWatershed->SetLevel(m_dLevel);
	pWatershed->SetThreshold(m_dThreshold);
	pWatershed->Update();

	typedef itk::Image<itkWatershed::OutputImagePixelType, 3> IntImageType;
	typedef itk::CastImageFilter<IntImageType, itk::Image<unsigned short, 3>> itkCaster;
	itkCaster::Pointer pCaster(itkCaster::New());
	pCaster->SetInput(0, pWatershed->GetOutput());

	pConnector->SetImage(pCaster->GetOutput());

	m_pImageData->DeepCopy(pConnector->GetVTKImage());
	m_pImageData->CopyInformationFromPipeline(pConnector->GetVTKImage()->GetInformation());

	m_dExecuteTime = 0.001 * (double) t.elapsed();

	setItemText();
}

double iAFoamCharacterizationItemWatershed::level() const
{
	return m_dLevel;
}

double iAFoamCharacterizationItemWatershed::threshold() const
{
	return m_dThreshold;
}

void iAFoamCharacterizationItemWatershed::open(QFile* _pFileOpen)
{
	iAFoamCharacterizationItem::open(_pFileOpen);

	_pFileOpen->read((char*)&m_dLevel, sizeof(m_dLevel));
	_pFileOpen->read((char*)&m_dThreshold, sizeof(m_dThreshold));

	setItemText();
}

void iAFoamCharacterizationItemWatershed::save(QFile* _pFileSave)
{
	iAFoamCharacterizationItem::save(_pFileSave);

	_pFileSave->write((char*)&m_dLevel, sizeof(m_dLevel));
	_pFileSave->write((char*)&m_dThreshold, sizeof(m_dThreshold));
}

void iAFoamCharacterizationItemWatershed::setLevel(const double& _dLevel)
{
	m_dLevel = _dLevel;
}

void iAFoamCharacterizationItemWatershed::setThreshold(const double& _dThreshold)
{
	m_dThreshold = _dThreshold;
}

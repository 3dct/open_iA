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

#include "iAFoamCharacterizationItemFilter.h"

#include <QApplication>
#include <QFile>
#include <QTime>

#include <itkDiscreteGaussianImageFilter.h>
#include <itkGradientAnisotropicDiffusionImageFilter.h>
#include <itkMedianImageFilter.h>

#include "iAConnector.h"

#include "iAFoamCharacterizationDialogFilter.h"

iAFoamCharacterizationItemFilter::iAFoamCharacterizationItemFilter(vtkImageData* _pImageData)
	                                               : iAFoamCharacterizationItem(_pImageData, iAFoamCharacterizationItem::itFilter)
{

}

iAFoamCharacterizationItemFilter::iAFoamCharacterizationItemFilter(iAFoamCharacterizationItemFilter* _pFilter)
	                                                                                        : iAFoamCharacterizationItem(_pFilter)
{
	m_eItemFilterType = _pFilter->itemFilterType();

	setName(_pFilter->name());

	m_dAnisotropicConductance = _pFilter->anisotropicConductance();
	m_iAnisotropicIteration = _pFilter->anisotropicIteration();
	m_dAnisotropicTimeStep = _pFilter->anisotropicTimeStep();

	m_dGaussVariance = _pFilter->gaussVariance();

	m_iMedianBoxRadius = _pFilter->medianBoxRadius();
}

double iAFoamCharacterizationItemFilter::anisotropicConductance() const
{
	return m_dAnisotropicConductance;
}

int iAFoamCharacterizationItemFilter::anisotropicIteration() const
{
	return m_iAnisotropicIteration;
}

double iAFoamCharacterizationItemFilter::anisotropicTimeStep() const
{
	return m_dAnisotropicTimeStep;
}

int iAFoamCharacterizationItemFilter::medianBoxRadius() const
{
	return m_iMedianBoxRadius;
}

void iAFoamCharacterizationItemFilter::dialog()
{
	QScopedPointer<iAFoamCharacterizationDialogFilter> pDialog(new iAFoamCharacterizationDialogFilter(this, qApp->focusWidget()));

	pDialog->exec();
}

void iAFoamCharacterizationItemFilter::execute()
{
	QTime t;
	t.start();

	switch (m_eItemFilterType)
	{
		case iftAnisotropic:
		executeAnisotropic();
		break;

		case iftGauss:
		executeGauss();
		break;

		default:
		executeMedian();
		break;
	}

	setTime(t.elapsed());
}

void iAFoamCharacterizationItemFilter::executeAnisotropic()
{
	typedef itk::GradientAnisotropicDiffusionImageFilter <itk::Image<unsigned short, 3>, itk::Image<float, 3>> itkFilter;

	itkFilter::Pointer pFilter(itkFilter::New());

	iAConnector connector1;
	connector1.SetImage(m_pImageData);

	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (connector1.GetITKImage()));
	pFilter->SetConductanceParameter(m_dAnisotropicConductance);
	pFilter->SetNumberOfIterations(m_iAnisotropicIteration);
	pFilter->SetTimeStep(m_dAnisotropicTimeStep);
	pFilter->Update();

	iAConnector connector2;
	connector2.SetImage(pFilter->GetOutput());

	m_pImageData->DeepCopy(connector2.GetVTKImage());
}

void iAFoamCharacterizationItemFilter::executeGauss()
{
	typedef itk::DiscreteGaussianImageFilter <itk::Image<unsigned short, 3>, itk::Image<unsigned short, 3>> itkFilter;

	itkFilter::Pointer pFilter(itkFilter::New());

	iAConnector connector1;
	connector1.SetImage(m_pImageData);

	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (connector1.GetITKImage()));
	pFilter->SetVariance(m_dGaussVariance);
	pFilter->Update();

	iAConnector connector2;
	connector2.SetImage(pFilter->GetOutput());

	m_pImageData->DeepCopy(connector2.GetVTKImage());
}

void iAFoamCharacterizationItemFilter::executeMedian()
{
	typedef itk::MedianImageFilter <itk::Image<unsigned short, 3>, itk::Image<unsigned short, 3>> itkFilter;

	itkFilter::Pointer pFilter(itkFilter::New());

	itkFilter::InputSizeType radius;
	radius.Fill(m_iMedianBoxRadius);
	pFilter->SetRadius(radius);

	iAConnector connector1;
	connector1.SetImage(m_pImageData);

	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (connector1.GetITKImage()));
	pFilter->Update();

	iAConnector connector2;
	connector2.SetImage(pFilter->GetOutput());

	m_pImageData->DeepCopy(connector2.GetVTKImage());
}

double iAFoamCharacterizationItemFilter::gaussVariance() const
{
	return m_dGaussVariance;
}

iAFoamCharacterizationItemFilter::EItemFilterType iAFoamCharacterizationItemFilter::itemFilterType() const
{
	return m_eItemFilterType;
}

QString iAFoamCharacterizationItemFilter::itemFilterTypeString() const
{
	switch (m_eItemFilterType)
	{
		case iftAnisotropic:
		return "A";
		break;

     	case iftGauss:
		return "G";
		break;

		default:
		return "M";
	}
}

void iAFoamCharacterizationItemFilter::open(QFile* _pFileOpen)
{
	iAFoamCharacterizationItem::open(_pFileOpen);

	_pFileOpen->read((char*) &m_eItemFilterType, sizeof(m_eItemFilterType));
	_pFileOpen->read((char*) &m_dAnisotropicConductance, sizeof(m_dAnisotropicConductance));
	_pFileOpen->read((char*) &m_dAnisotropicTimeStep, sizeof(m_dAnisotropicTimeStep));
	_pFileOpen->read((char*) &m_iAnisotropicIteration, sizeof(m_iAnisotropicIteration));
	_pFileOpen->read((char*) &m_dGaussVariance, sizeof(m_dGaussVariance));
	_pFileOpen->read((char*) &m_iMedianBoxRadius, sizeof(m_iMedianBoxRadius));

	setItemText();
}

void iAFoamCharacterizationItemFilter::save(QFile* _pFileSave)
{
	iAFoamCharacterizationItem::save(_pFileSave);

	_pFileSave->write((char*) &m_eItemFilterType, sizeof(m_eItemFilterType));
	_pFileSave->write((char*) &m_dAnisotropicConductance, sizeof(m_dAnisotropicConductance));
	_pFileSave->write((char*) &m_dAnisotropicTimeStep, sizeof(m_dAnisotropicTimeStep));
	_pFileSave->write((char*) &m_iAnisotropicIteration, sizeof(m_iAnisotropicIteration));
	_pFileSave->write((char*) &m_dGaussVariance, sizeof(m_dGaussVariance));
	_pFileSave->write((char*) &m_iMedianBoxRadius, sizeof(m_iMedianBoxRadius));
}

void iAFoamCharacterizationItemFilter::setAnisotropicConductance(const double& _dAnisotropicConductance)
{
	m_dAnisotropicConductance = _dAnisotropicConductance;
}

void iAFoamCharacterizationItemFilter::setAnisotropicIteration(const int& _iAnisotropicIteration)
{
	m_iAnisotropicIteration = _iAnisotropicIteration;
}

void iAFoamCharacterizationItemFilter::setAnisotropicTimeStep(const double& _dAnisotropicTimeStep)
{
	m_dAnisotropicTimeStep = _dAnisotropicTimeStep;
}

void iAFoamCharacterizationItemFilter::setGaussVariance(const double& _dGaussVariance)
{
	m_dGaussVariance = _dGaussVariance;
}

void iAFoamCharacterizationItemFilter::setItemFilterType(const iAFoamCharacterizationItemFilter::EItemFilterType& _eItemFilterType)
{
	m_eItemFilterType = _eItemFilterType;
}

void iAFoamCharacterizationItemFilter::setItemText()
{
	if (m_dExecute > 0.0)
	{
		setText(m_sName + QString(" [%1] (%2)").arg(itemFilterTypeString()).arg(executeTimeString()));
	}
	else
	{
		setText(m_sName + QString(" [%1]").arg(itemFilterTypeString()));
	}
}

void iAFoamCharacterizationItemFilter::setMedianBoxRadius(const int& _iMedianBoxRadius)
{
	m_iMedianBoxRadius = _iMedianBoxRadius;
}

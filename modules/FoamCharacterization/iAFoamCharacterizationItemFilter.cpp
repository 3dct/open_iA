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
#include <QThreadPool>
#include <QTime>

#include <QtMath>

#include <itkDiscreteGaussianImageFilter.h>
#include <itkGradientAnisotropicDiffusionImageFilter.h>
#include <itkMedianImageFilter.h>
#include <itkPatchBasedDenoisingImageFilter.h>

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
	m_uiAnisotropicIteration = _pFilter->anisotropicIteration();
	m_dAnisotropicTimeStep = _pFilter->anisotropicTimeStep();

	m_dGaussianVariance = _pFilter->gaussianVariance();

	m_uiMedianRadius = _pFilter->medianRadius();
	m_uiNonLocalMeansRadius = _pFilter->nonLocalMeansRadius();
}

double iAFoamCharacterizationItemFilter::anisotropicConductance() const
{
	return m_dAnisotropicConductance;
}

unsigned int iAFoamCharacterizationItemFilter::anisotropicIteration() const
{
	return m_uiAnisotropicIteration;
}

double iAFoamCharacterizationItemFilter::anisotropicTimeStep() const
{
	return m_dAnisotropicTimeStep;
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
		executeGaussian();
		break;

		case iftMedian:
		executeMedian();
		break;

		default:
		executeNonLocalMeans();
		break;
	}

	m_dExecuteTime = 0.001 * (double)t.elapsed();

	setItemText();
}

void iAFoamCharacterizationItemFilter::executeAnisotropic()
{
	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->SetImage(m_pImageData);

	typedef itk::GradientAnisotropicDiffusionImageFilter<itk::Image<unsigned short, 3>, itk::Image<float, 3>> itkFilter;
	itkFilter::Pointer pFilter(itkFilter::New());

	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->GetITKImage()));
	pFilter->SetConductanceParameter(m_dAnisotropicConductance);
	pFilter->SetNumberOfIterations(m_uiAnisotropicIteration);
	pFilter->SetTimeStep(m_dAnisotropicTimeStep);
	pFilter->Update();

	typedef itk::Image<typename itkFilter::OutputImagePixelType, 3> IntImageType;
	typedef itk::CastImageFilter<IntImageType, itk::Image<unsigned short, 3>> itkCaster;
	itkCaster::Pointer pCaster(itkCaster::New());
	pCaster->SetInput(0, pFilter->GetOutput());

	pConnector->SetImage(pCaster->GetOutput());

	m_pImageData->DeepCopy(pConnector->GetVTKImage());
	m_pImageData->CopyInformationFromPipeline(pConnector->GetVTKImage()->GetInformation());
}

void iAFoamCharacterizationItemFilter::executeGaussian()
{
	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->SetImage(m_pImageData);

	typedef itk::DiscreteGaussianImageFilter<itk::Image<unsigned short, 3>, itk::Image<unsigned short, 3>> itkFilter;
	itkFilter::Pointer pFilter(itkFilter::New());

	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->GetITKImage()));
	pFilter->SetVariance(m_dGaussianVariance);
	pFilter->Update();

	pConnector->SetImage(pFilter->GetOutput());

	m_pImageData->DeepCopy(pConnector->GetVTKImage());
	m_pImageData->CopyInformationFromPipeline(pConnector->GetVTKImage()->GetInformation());
}

void iAFoamCharacterizationItemFilter::executeMedian()
{
	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->SetImage(m_pImageData);

	typedef itk::MedianImageFilter<itk::Image<unsigned short, 3>, itk::Image<unsigned short, 3>> itkFilter;
	itkFilter::Pointer pFilter(itkFilter::New());

	itkFilter::InputSizeType radius;
	radius.Fill(m_uiMedianRadius);
	pFilter->SetRadius(radius);

	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->GetITKImage()));
	pFilter->Update();

	pConnector->SetImage(pFilter->GetOutput());

	m_pImageData->DeepCopy(pConnector->GetVTKImage());
	m_pImageData->CopyInformationFromPipeline(pConnector->GetVTKImage()->GetInformation());
}

void iAFoamCharacterizationItemFilter::executeMedianFX()
{
	unsigned short* pDataWrite((unsigned short*)m_pImageData->GetScalarPointer());

	const int* pDim(m_pImageData->GetDimensions());

	const unsigned int nk((unsigned int)pDim[2]);

	QThreadPool* pThreadPool(new QThreadPool());

	const unsigned int uiThread (QThread::idealThreadCount());

	QVector<QtRunnableMedian*> vRunnableMedian (uiThread);

	for (unsigned int ui(0), uii(1); ui < uiThread ; ++ui, ++uii)
	{
		vRunnableMedian[ui] = new QtRunnableMedian(this, nk * ui / uiThread, nk * uii / uiThread);

		pThreadPool->start(vRunnableMedian[ui]);
	}

	pThreadPool->waitForDone();

	for (auto& pRunnableMedian : vRunnableMedian)
	{
		delete pRunnableMedian;
	}

	delete pThreadPool;

	m_pImageData->Modified();
}

void iAFoamCharacterizationItemFilter::executeMedianFX(const unsigned int& _uiK1, const unsigned int& _uiK2)
{
	vtkSmartPointer<vtkImageData> pImageData1(vtkImageData::New());
	pImageData1->DeepCopy(m_pImageData);

	const int* pDim(m_pImageData->GetDimensions());

	const unsigned int ni((unsigned int)pDim[0]);
	const unsigned int nj((unsigned int)pDim[1]);
	const unsigned int nk((unsigned int)pDim[2]);

	const unsigned int uiStrideJ(ni);
	const unsigned int uiStrideK(ni * nj);

	unsigned short* pDataRead ((unsigned short*) pImageData1->GetScalarPointer());
    unsigned short* pDataWrite ((unsigned short*) m_pImageData->GetScalarPointer() + _uiK1 * uiStrideK);

	for (unsigned int k(_uiK1) ; k < _uiK2 ; ++k)
	{
		const int k1 (qMax(0u, k - m_uiMedianRadius));
		const int k2 (qMin(    k + m_uiMedianRadius, nk - 1u));

		const unsigned int uiBoxSizeK (k2 - k1 + 1);

		for (unsigned int j(0) ; j < nj ; ++j)
		{
			const unsigned int j1 (qMax(0u, j - m_uiMedianRadius));
			const unsigned int j2 (qMin(    j + m_uiMedianRadius, nj - 1u));

			const unsigned int uiBoxSizeJK ((j2 - j1 + 1) * uiBoxSizeK);

			for (unsigned int i(0) ; i < ni ; ++i)
			{
				const unsigned int i1 (qMax(0u, i - m_uiMedianRadius));
				const unsigned int i2 (qMin(    i + m_uiMedianRadius, ni - 1u));

				const unsigned int uiBoxSizeI (i2 - i1 + 1);

				*pDataWrite++ = executeMedianFXValue ( pDataRead, i1, i2, j1, j2, k1, k2, uiStrideJ, uiStrideK
													 , uiBoxSizeI * uiBoxSizeJK
													 );
			}
		}
	}
}

unsigned short iAFoamCharacterizationItemFilter::executeMedianFXValue	( unsigned short* _pDataRead
															, const unsigned int& _i1, const unsigned int& _i2
															, const unsigned int& _j1, const unsigned int& _j2
															, const unsigned int& _k1, const unsigned int& _k2
															, const unsigned int& _uiStrideJ
															, const unsigned int& _uiStrideK
															, const unsigned int& _uiBoxSize
															)
{
	std::vector<unsigned short> vValue (_uiBoxSize);

	unsigned int l (0);

	unsigned short* pDataReadK (_pDataRead + _k1 * _uiStrideK);

	for (unsigned int k(_k1); k <= _k2 ; ++k, pDataReadK += _uiStrideK)
	{
		unsigned short* pDataReadJ(pDataReadK + _j1 * _uiStrideJ);

		for (unsigned int j (_j1); j <= _j2 ; ++j, pDataReadJ += _uiStrideJ)
		{
			for (unsigned int i (_i1); i <= _i2 ; ++i)
			{
				vValue[l++] = pDataReadJ[i];
			}
		}
	}

	const unsigned int uiBoxSize2(_uiBoxSize / 2);

	std::nth_element(vValue.begin(), vValue.begin() + uiBoxSize2, vValue.end());

	return vValue[uiBoxSize2];
}

void iAFoamCharacterizationItemFilter::executeNonLocalMeans()
{
	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->SetImage(m_pImageData);

	typedef itk::PatchBasedDenoisingImageFilter<itk::Image<unsigned short, 3>, itk::Image<unsigned short, 3>> itkFilter;
	itkFilter::Pointer pFilter(itkFilter::New());

	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->GetITKImage()));
	pFilter->SetNumberOfIterations(m_uiNonLocalMeansIteration);
	pFilter->SetPatchRadius(m_uiNonLocalMeansRadius);
	pFilter->Update();

	pConnector->SetImage(pFilter->GetOutput());

	m_pImageData->DeepCopy(pConnector->GetVTKImage());
	m_pImageData->CopyInformationFromPipeline(pConnector->GetVTKImage()->GetInformation());
}

double iAFoamCharacterizationItemFilter::gaussianVariance() const
{
	return m_dGaussianVariance;
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

		case iftMedian:
		return "M";
		break;

		default:
		return "N";
		break;
	}
}

unsigned int iAFoamCharacterizationItemFilter::medianRadius() const
{
	return m_uiMedianRadius;
}

unsigned int iAFoamCharacterizationItemFilter::nonLocalMeansIteration() const
{
	return m_uiNonLocalMeansIteration;
}

unsigned int iAFoamCharacterizationItemFilter::nonLocalMeansRadius() const
{
	return m_uiNonLocalMeansRadius;
}

void iAFoamCharacterizationItemFilter::open(QFile* _pFileOpen)
{
	iAFoamCharacterizationItem::open(_pFileOpen);

	_pFileOpen->read((char*) &m_eItemFilterType, sizeof(m_eItemFilterType));
	_pFileOpen->read((char*) &m_dAnisotropicConductance, sizeof(m_dAnisotropicConductance));
	_pFileOpen->read((char*) &m_dAnisotropicTimeStep, sizeof(m_dAnisotropicTimeStep));
	_pFileOpen->read((char*) &m_uiAnisotropicIteration, sizeof(m_uiAnisotropicIteration));
	_pFileOpen->read((char*) &m_dGaussianVariance, sizeof(m_dGaussianVariance));
	_pFileOpen->read((char*)&m_uiMedianRadius, sizeof(m_uiMedianRadius));
	_pFileOpen->read((char*)&m_uiNonLocalMeansIteration, sizeof(m_uiNonLocalMeansIteration));
	_pFileOpen->read((char*)&m_uiNonLocalMeansRadius, sizeof(m_uiNonLocalMeansRadius));

	setItemText();
}

void iAFoamCharacterizationItemFilter::save(QFile* _pFileSave)
{
	iAFoamCharacterizationItem::save(_pFileSave);

	_pFileSave->write((char*) &m_eItemFilterType, sizeof(m_eItemFilterType));
	_pFileSave->write((char*) &m_dAnisotropicConductance, sizeof(m_dAnisotropicConductance));
	_pFileSave->write((char*) &m_dAnisotropicTimeStep, sizeof(m_dAnisotropicTimeStep));
	_pFileSave->write((char*) &m_uiAnisotropicIteration, sizeof(m_uiAnisotropicIteration));
	_pFileSave->write((char*) &m_dGaussianVariance, sizeof(m_dGaussianVariance));
	_pFileSave->write((char*) &m_uiMedianRadius, sizeof(m_uiMedianRadius));
	_pFileSave->write((char*)&m_uiNonLocalMeansIteration, sizeof(m_uiNonLocalMeansIteration));
	_pFileSave->write((char*)&m_uiNonLocalMeansRadius, sizeof(m_uiNonLocalMeansRadius));
}

void iAFoamCharacterizationItemFilter::setAnisotropicConductance(const double& _dAnisotropicConductance)
{
	m_dAnisotropicConductance = _dAnisotropicConductance;
}

void iAFoamCharacterizationItemFilter::setAnisotropicIteration(const unsigned int& _uiAnisotropicIteration)
{
	m_uiAnisotropicIteration = _uiAnisotropicIteration;
}

void iAFoamCharacterizationItemFilter::setAnisotropicTimeStep(const double& _dAnisotropicTimeStep)
{
	m_dAnisotropicTimeStep = _dAnisotropicTimeStep;
}

void iAFoamCharacterizationItemFilter::setGaussianVariance(const double& _dGaussianVariance)
{
	m_dGaussianVariance = _dGaussianVariance;
}

void iAFoamCharacterizationItemFilter::setItemFilterType(const iAFoamCharacterizationItemFilter::EItemFilterType& _eItemFilterType)
{
	m_eItemFilterType = _eItemFilterType;
}

void iAFoamCharacterizationItemFilter::setItemText()
{
	setText(m_sName + QString(" [%1]").arg(itemFilterTypeString()));
}

void iAFoamCharacterizationItemFilter::setMedianRadius(const unsigned int& _uiMedianRadius)
{
	m_uiMedianRadius = _uiMedianRadius;
}

void iAFoamCharacterizationItemFilter::setNonLocalMeansIteration(const unsigned int& _uiNonLocalMeansIteration)
{
	m_uiNonLocalMeansIteration = _uiNonLocalMeansIteration;
}

void iAFoamCharacterizationItemFilter::setNonLocalMeansRadius(const unsigned int& _uiNonLocalMeansRadius)
{
	m_uiNonLocalMeansRadius = _uiNonLocalMeansRadius;
}

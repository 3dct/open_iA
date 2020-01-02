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
#include "iAFoamCharacterizationItemFilter.h"

#include "iAFoamCharacterizationDialogFilter.h"
#include "iAFoamCharacterizationTable.h"

#include "iAConnector.h"
#include "iAProgress.h"

#include <itkDiscreteGaussianImageFilter.h>
#include <itkGradientAnisotropicDiffusionImageFilter.h>
#include <itkMedianImageFilter.h>
#include <itkPatchBasedDenoisingImageFilter.h>

#include <vtkImageData.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QThreadPool>
#include <QtMath>

iAFoamCharacterizationItemFilter::iAFoamCharacterizationItemFilter
																 (iAFoamCharacterizationTable* _pTable, vtkImageData* _pImageData)
										  : iAFoamCharacterizationItem(_pTable, _pImageData, iAFoamCharacterizationItem::itFilter)
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

	m_bGaussianImageSpacing = _pFilter->gaussianImageSpacing();
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
	setExecuting(true);

	QElapsedTimer t;
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

	m_dExecuteTime = 0.001 * (double) t.elapsed();

	setExecuting(false);
}

void iAFoamCharacterizationItemFilter::executeAnisotropic()
{
	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->setImage(m_pImageData);

	typedef itk::GradientAnisotropicDiffusionImageFilter<itk::Image<unsigned short, 3>, itk::Image<float, 3>> itkFilter;
	itkFilter::Pointer pFilter(itkFilter::New());
	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->itkImage()));
	pFilter->SetConductanceParameter(m_dAnisotropicConductance);
	pFilter->SetNumberOfIterations(m_uiAnisotropicIteration);
	pFilter->SetTimeStep(m_dAnisotropicTimeStep);
	
	QScopedPointer<iAProgress> pObserver(new iAProgress());
	pObserver->observe(pFilter);
	connect(pObserver.data(), SIGNAL(progress(const int&)), this, SLOT(slotObserver(const int&)));
	
	pFilter->Update();

	typedef itk::Image<typename itkFilter::OutputImagePixelType, 3> IntImageType;
	typedef itk::CastImageFilter<IntImageType, itk::Image<unsigned short, 3>> itkCaster;
	itkCaster::Pointer pCaster(itkCaster::New());
	pCaster->SetInput(0, pFilter->GetOutput());

	pConnector->setImage(pCaster->GetOutput());

	m_pImageData->DeepCopy(pConnector->vtkImage());
	m_pImageData->CopyInformationFromPipeline(pConnector->vtkImage()->GetInformation());
}

void iAFoamCharacterizationItemFilter::executeGaussian()
{
	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->setImage(m_pImageData);

	typedef itk::DiscreteGaussianImageFilter<itk::Image<unsigned short, 3>, itk::Image<unsigned short, 3>> itkFilter;
	itkFilter::Pointer pFilter(itkFilter::New());
	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->itkImage()));
	pFilter->SetVariance(m_dGaussianVariance);
	pFilter->SetUseImageSpacing(m_bGaussianImageSpacing);

	QScopedPointer<iAProgress> pObserver(new iAProgress());
	pObserver->observe(pFilter);
	connect(pObserver.data(), SIGNAL(progress(const int&)), this, SLOT(slotObserver(const int&)));

	pFilter->Update();

	pConnector->setImage(pFilter->GetOutput());

	m_pImageData->DeepCopy(pConnector->vtkImage());
	m_pImageData->CopyInformationFromPipeline(pConnector->vtkImage()->GetInformation());
}

void iAFoamCharacterizationItemFilter::executeMedian()
{
	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->setImage(m_pImageData);

	typedef itk::MedianImageFilter<itk::Image<unsigned short, 3>, itk::Image<unsigned short, 3>> itkFilter;
	itkFilter::Pointer pFilter(itkFilter::New());
	itkFilter::InputSizeType radius;
	radius.Fill(m_uiMedianRadius);
	pFilter->SetRadius(radius);
	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->itkImage()));
	
	QScopedPointer<iAProgress> pObserver(new iAProgress());
	pObserver->observe(pFilter);
	connect(pObserver.data(), SIGNAL(progress(const int&)), this, SLOT(slotObserver(const int&)));

	pFilter->Update();

	pConnector->setImage(pFilter->GetOutput());

	m_pImageData->DeepCopy(pConnector->vtkImage());
	m_pImageData->CopyInformationFromPipeline(pConnector->vtkImage()->GetInformation());
}

void iAFoamCharacterizationItemFilter::executeMedianFX()
{
	m_uiMedianFXSlice = 0;

	vtkSmartPointer<vtkImageData> pImageDataRead(vtkImageData::New());
	pImageDataRead->DeepCopy(m_pImageData);

	const int* pDim(m_pImageData->GetDimensions());

	const unsigned int ni((unsigned int) pDim[0]);
	const unsigned int nj((unsigned int) pDim[1]);
	const unsigned int nk((unsigned int) pDim[2]);

	const unsigned int uiStrideJ (ni);
	const unsigned int uiStrideK (ni * nj);

	QScopedPointer<QThreadPool> pThreadPool (new QThreadPool(qApp));

	const unsigned int uiThread(QThread::idealThreadCount());
	const unsigned int uiThread_1 (uiThread - 1);

	QVector<QSharedPointer<QtRunnableMedian>> vRunnableMedian (uiThread);

	unsigned short* pDataRead ((unsigned short*) pImageDataRead->GetScalarPointer());
	unsigned short* pDataWrite ((unsigned short*) m_pImageData->GetScalarPointer());

	for (unsigned int ui(0), uii(1); ui < uiThread_1; ++ui, ++uii)
	{
		vRunnableMedian[ui].reset ( new QtRunnableMedian ( this, pDataRead, pDataWrite
														 , ni, nj, nk, uiStrideJ, uiStrideK
														 , nk * ui / uiThread, nk * uii / uiThread
														 )
								  );

		pThreadPool->start(vRunnableMedian[ui].data());
	}

	executeMedianFX1(pDataRead, pDataWrite, ni, nj, nk, uiStrideJ, uiStrideK, nk * uiThread_1 / uiThread, nk);

	pThreadPool->waitForDone();

	m_pImageData->Modified();
}

void iAFoamCharacterizationItemFilter::executeMedianFX(unsigned short* _pDataRead, unsigned short* _pDataWrite
	, const unsigned int& _uiNi, const unsigned int& _uiNj, const unsigned int& _uiNk
	, const unsigned int& _uiStrideJ, const unsigned int& _uiStrideK
	, const unsigned int& _uiK1, const unsigned int& _uiK2
)
{
	for (unsigned int uiK(_uiK1); uiK < _uiK2; ++uiK, ++m_uiMedianFXSlice)
	{
		executeMedianFXSlice(_pDataRead, _pDataWrite + uiK * _uiStrideK, _uiNi, _uiNj, _uiNk, _uiStrideJ, _uiStrideK, uiK);
	}
}

void iAFoamCharacterizationItemFilter::executeMedianFX1(unsigned short* _pDataRead, unsigned short* _pDataWrite
	, const unsigned int& _uiNi, const unsigned int& _uiNj, const unsigned int& _uiNk
	, const unsigned int& _uiStrideJ, const unsigned int& _uiStrideK
	, const unsigned int& _uiK1, const unsigned int& _uiK2
)
{
	for (unsigned int uiK (_uiK1); uiK < _uiK2; ++uiK)
	{
		executeMedianFXSlice(_pDataRead, _pDataWrite + uiK * _uiStrideK, _uiNi, _uiNj, _uiNk, _uiStrideJ, _uiStrideK, uiK);

		setProgress(100u * m_uiMedianFXSlice++ / _uiNk);
	}
}

void iAFoamCharacterizationItemFilter::executeMedianFXSlice(unsigned short* _pDataRead, unsigned short* _pDataWrite
	, const unsigned int& _uiNi, const unsigned int& _uiNj, const unsigned int& _uiNk
	, const unsigned int& _uiStrideJ, const unsigned int& _uiStrideK
	, const unsigned int& _uiK
)
{
	const unsigned int iuNi_MedianRadius(_uiNi - m_uiMedianRadius);
	const unsigned int iuNj_MedianRadius(_uiNj - m_uiMedianRadius);

	const unsigned int k1(_uiK - qMin(_uiK, m_uiMedianRadius));
	const unsigned int k2(qMin(_uiK + m_uiMedianRadius, _uiNk - 1));

	const unsigned int uiBoxSizeK(k2 - k1 + 1);

	unsigned int j(0);

	for (; j < m_uiMedianRadius; ++j)
	{
		const unsigned int j1(0);
		const unsigned int j2(qMin(j + m_uiMedianRadius, _uiNj - 1));

		const unsigned int uiBoxSizeJK((j2 - j1 + 1) * uiBoxSizeK);

		unsigned int i(0);

		for (; i < m_uiMedianRadius; ++i)
		{
			const unsigned int i1(0);
			const unsigned int i2(qMin(i + m_uiMedianRadius, _uiNi - 1));

			*_pDataWrite++ = executeMedianFXValue(_pDataRead, i1, i2, j1, j2
				, k1, k2, _uiStrideJ, _uiStrideK, (i2 - i1 + 1) * uiBoxSizeJK
			);
		}

		for (; i < iuNi_MedianRadius; ++i)
		{
			const unsigned int i1(i - m_uiMedianRadius);
			const unsigned int i2(i + m_uiMedianRadius);

			*_pDataWrite++ = executeMedianFXValue(_pDataRead, i1, i2, j1, j2, k1, k2
				, _uiStrideJ, _uiStrideK, (2 * m_uiMedianRadius + 1) * uiBoxSizeJK
			);
		}

		for (; i < _uiNi; ++i)
		{
			const unsigned int i1(i - qMin(i, m_uiMedianRadius));
			const unsigned int i2(_uiNi - 1);

			*_pDataWrite++ = executeMedianFXValue(_pDataRead, i1, i2, j1, j2, k1, k2
				, _uiStrideJ, _uiStrideK, (i2 - i1 + 1) * uiBoxSizeJK
			);
		}
	}

	for (; j < iuNj_MedianRadius; ++j)
	{
		const unsigned int j1(j - m_uiMedianRadius);
		const unsigned int j2(j + m_uiMedianRadius);

		const unsigned int uiBoxSizeJK((2 * m_uiMedianRadius + 1) * uiBoxSizeK);

		unsigned int i(0);

		for (; i < m_uiMedianRadius; ++i)
		{
			const unsigned int i1(0);
			const unsigned int i2(qMin(i + m_uiMedianRadius, _uiNi - 1));

			*_pDataWrite++ = executeMedianFXValue(_pDataRead, i1, i2, j1, j2
				, k1, k2, _uiStrideJ, _uiStrideK, (i2 - i1 + 1) * uiBoxSizeJK
			);
		}

		for (; i < iuNi_MedianRadius; ++i)
		{
			const unsigned int i1(i - m_uiMedianRadius);
			const unsigned int i2(i + m_uiMedianRadius);

			*_pDataWrite++ = executeMedianFXValue(_pDataRead, i1, i2, j1, j2, k1, k2
				, _uiStrideJ, _uiStrideK, (2 * m_uiMedianRadius + 1) * uiBoxSizeJK
			);
		}

		for (; i < _uiNi; ++i)
		{
			const unsigned int i1(i - qMin(i, m_uiMedianRadius));
			const unsigned int i2(_uiNi - 1);

			*_pDataWrite++ = executeMedianFXValue(_pDataRead, i1, i2, j1, j2, k1, k2
				, _uiStrideJ, _uiStrideK, (i2 - i1 + 1) * uiBoxSizeJK
			);
		}
	}

	for (; j < _uiNj; ++j)
	{
		const unsigned int j1(j - qMin(j, m_uiMedianRadius));
		const unsigned int j2(_uiNj - 1);

		const unsigned int uiBoxSizeJK((j2 - j1 + 1) * uiBoxSizeK);

		unsigned int i(0);

		for (; i < m_uiMedianRadius; ++i)
		{
			const unsigned int i1(0);
			const unsigned int i2(qMin(i + m_uiMedianRadius, _uiNi - 1));

			*_pDataWrite++ = executeMedianFXValue(_pDataRead, i1, i2, j1, j2
				, k1, k2, _uiStrideJ, _uiStrideK, (i2 - i1 + 1) * uiBoxSizeJK
			);
		}

		for (; i < iuNi_MedianRadius; ++i)
		{
			const unsigned int i1(i - m_uiMedianRadius);
			const unsigned int i2(i + m_uiMedianRadius);

			*_pDataWrite++ = executeMedianFXValue(_pDataRead, i1, i2, j1, j2, k1, k2
				, _uiStrideJ, _uiStrideK, (2 * m_uiMedianRadius + 1) * uiBoxSizeJK
			);
		}

		for (; i < _uiNi; ++i)
		{
			const unsigned int i1(i - qMin(i, m_uiMedianRadius));
			const unsigned int i2(_uiNi - 1);

			*_pDataWrite++ = executeMedianFXValue(_pDataRead, i1, i2, j1, j2, k1, k2
				, _uiStrideJ, _uiStrideK, (i2 - i1 + 1) * uiBoxSizeJK
			);
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
	const unsigned int uiMemCpySize (_i2 - _i1 + 1);
	const unsigned int uiMemCpySizeChar (uiMemCpySize * sizeof(unsigned short));

	std::vector<unsigned short> vValue (_uiBoxSize);

	unsigned int l (0);

	unsigned short* pDataReadK (_pDataRead + _k1 * _uiStrideK);

	for (unsigned int k(_k1); k <= _k2 ; ++k, pDataReadK += _uiStrideK)
	{
		unsigned short* pDataReadJ (pDataReadK + _j1 * _uiStrideJ);

		for (unsigned int j (_j1); j <= _j2 ; ++j, pDataReadJ += _uiStrideJ)
		{
			memcpy(&vValue[0] + l++ * uiMemCpySize, pDataReadJ + _i1, uiMemCpySizeChar);
		}
	}

	const unsigned int uiBoxSize2 (_uiBoxSize / 2);

	std::nth_element(vValue.begin(), vValue.begin() + uiBoxSize2, vValue.end());

	return vValue[uiBoxSize2];
}

void iAFoamCharacterizationItemFilter::executeNonLocalMeans()
{
	QScopedPointer<iAConnector> pConnector(new iAConnector());
	pConnector->setImage(m_pImageData);

	typedef itk::PatchBasedDenoisingImageFilter<itk::Image<unsigned short, 3>, itk::Image<unsigned short, 3>> itkFilter;
	itkFilter::Pointer pFilter(itkFilter::New());
	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (pConnector->itkImage()));
	pFilter->SetNumberOfIterations(m_uiNonLocalMeansIteration);
	pFilter->SetKernelBandwidthEstimation(false);
	pFilter->SetPatchRadius(m_uiNonLocalMeansRadius);

	QScopedPointer<iAProgress> pObserver(new iAProgress());
	pObserver->observe(pFilter);
	connect(pObserver.data(), SIGNAL(progress(const int&)), this, SLOT(slotObserver(const int&)));

	pFilter->Update();

	pConnector->setImage(pFilter->GetOutput());

	m_pImageData->DeepCopy(pConnector->vtkImage());
	m_pImageData->CopyInformationFromPipeline(pConnector->vtkImage()->GetInformation());
}

bool iAFoamCharacterizationItemFilter::gaussianImageSpacing() const
{
	return m_bGaussianImageSpacing;
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
	_pFileOpen->read((char*) &m_bGaussianImageSpacing, sizeof(m_dGaussianVariance));
	_pFileOpen->read((char*) &m_dGaussianVariance, sizeof(m_dGaussianVariance));
	_pFileOpen->read((char*) &m_uiMedianRadius, sizeof(m_uiMedianRadius));
	_pFileOpen->read((char*) &m_uiNonLocalMeansIteration, sizeof(m_uiNonLocalMeansIteration));
	_pFileOpen->read((char*) &m_uiNonLocalMeansRadius, sizeof(m_uiNonLocalMeansRadius));

	setItemText();
}

void iAFoamCharacterizationItemFilter::save(QFile* _pFileSave)
{
	iAFoamCharacterizationItem::save(_pFileSave);

	_pFileSave->write((char*) &m_eItemFilterType, sizeof(m_eItemFilterType));
	_pFileSave->write((char*) &m_dAnisotropicConductance, sizeof(m_dAnisotropicConductance));
	_pFileSave->write((char*) &m_dAnisotropicTimeStep, sizeof(m_dAnisotropicTimeStep));
	_pFileSave->write((char*) &m_uiAnisotropicIteration, sizeof(m_uiAnisotropicIteration));
	_pFileSave->write((char*) &m_bGaussianImageSpacing, sizeof(m_dGaussianVariance));
	_pFileSave->write((char*) &m_dGaussianVariance, sizeof(m_dGaussianVariance));
	_pFileSave->write((char*) &m_uiMedianRadius, sizeof(m_uiMedianRadius));
	_pFileSave->write((char*) &m_uiNonLocalMeansIteration, sizeof(m_uiNonLocalMeansIteration));
	_pFileSave->write((char*) &m_uiNonLocalMeansRadius, sizeof(m_uiNonLocalMeansRadius));
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

void iAFoamCharacterizationItemFilter::setGaussianImageSpacing(const bool& _bGaussianImageSpacing)
{
	m_bGaussianImageSpacing = _bGaussianImageSpacing;
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

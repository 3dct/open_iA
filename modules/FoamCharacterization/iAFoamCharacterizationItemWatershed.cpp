// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFoamCharacterizationItemWatershed.h"

#include "iAFoamCharacterizationDialogWatershed.h"

#include <iAImageData.h>
#include <iAProgress.h>
#include <iAToolsITK.h>    // for itkScalarType

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkCastImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#include <itkWatershedImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <vtkImageData.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QFile>

iAFoamCharacterizationItemWatershed::iAFoamCharacterizationItemWatershed(iAFoamCharacterizationTable* _pTable) :
	iAFoamCharacterizationItem(_pTable, iAFoamCharacterizationItem::itWatershed)
{
}

iAFoamCharacterizationItemWatershed::iAFoamCharacterizationItemWatershed(
	iAFoamCharacterizationItemWatershed* _pWatershed) :
	iAFoamCharacterizationItem(_pWatershed)
{
	setName(_pWatershed->name());

	m_dLevel = _pWatershed->level();
	m_dThreshold = _pWatershed->threshold();

	m_iItemMask = _pWatershed->itemMask();
}

void iAFoamCharacterizationItemWatershed::dialog()
{
	QScopedPointer<iAFoamCharacterizationDialogWatershed> pDialog(
		new iAFoamCharacterizationDialogWatershed(this, QApplication::focusWidget()));
	pDialog->exec();
}

std::shared_ptr<iADataSet> iAFoamCharacterizationItemWatershed::execute(std::shared_ptr<iADataSet> dataSet)
{
	setExecuting(true);

	QElapsedTimer t;
	t.start();
	auto itkImg = dynamic_cast<iAImageData*>(dataSet.get())->itkImage();
	auto result = (itkScalarType(itkImg)  == iAITKIO::ScalarType::FLOAT) ?
		executeFloat(itkImg) : executeUnsignedShort(itkImg);

	if (m_iItemMask > -1)
	{
		//if (m_pItemMask->isMask())
		{

		}
	}
	m_dExecuteTime = 0.001 * (double) t.elapsed();

	setExecuting(false);
	return result;
}

std::shared_ptr<iADataSet> iAFoamCharacterizationItemWatershed::executeFloat(itk::ImageBase<3>* img)
{
	typedef itk::WatershedImageFilter<itk::Image<float, 3>> itkWatershed;
	itkWatershed::Pointer pFilter(itkWatershed::New());
	pFilter->SetInput(dynamic_cast<itk::Image<float, 3>*>(img));
	pFilter->SetLevel(m_dLevel);
	pFilter->SetThreshold(m_dThreshold);

	QScopedPointer<iAProgress> pObserver(new iAProgress());
	pObserver->observe(pFilter);
	connect(pObserver.data(), &iAProgress::progress, this, &iAFoamCharacterizationItemWatershed::slotObserver);

	pFilter->Update();

	typedef itk::Image<itkWatershed::OutputImagePixelType, 3> IntImageType;
	typedef itk::CastImageFilter<IntImageType, itk::Image<unsigned short, 3>> itkCaster;
	itkCaster::Pointer pCaster(itkCaster::New());
	pCaster->SetInput(0, pFilter->GetOutput());

	return std::make_shared<iAImageData>(pCaster->GetOutput());
}

std::shared_ptr<iADataSet> iAFoamCharacterizationItemWatershed::executeUnsignedShort(itk::ImageBase<3>* img)
{
	typedef itk::WatershedImageFilter<itk::Image<unsigned short, 3>> itkWatershed;
	itkWatershed::Pointer pFilter(itkWatershed::New());
	pFilter->SetInput(dynamic_cast<itk::Image<unsigned short, 3>*> (img));
	pFilter->SetLevel(m_dLevel);
	pFilter->SetThreshold(m_dThreshold);

	QScopedPointer<iAProgress> pObserver(new iAProgress());
	pObserver->observe(pFilter);
	connect(pObserver.data(), &iAProgress::progress, this, &iAFoamCharacterizationItemWatershed::slotObserver);

	pFilter->Update();

	return std::make_shared<iAImageData>(pFilter->GetOutput());
}

int iAFoamCharacterizationItemWatershed::itemMask() const
{
	return m_iItemMask;
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
	_pFileOpen->read((char*)&m_iItemMask, sizeof(m_iItemMask));

	setItemText();
}

void iAFoamCharacterizationItemWatershed::save(QFile* _pFileSave)
{
	iAFoamCharacterizationItem::save(_pFileSave);

	_pFileSave->write((char*)&m_dLevel, sizeof(m_dLevel));
	_pFileSave->write((char*)&m_dThreshold, sizeof(m_dThreshold));
	_pFileSave->write((char*)&m_iItemMask, sizeof(m_iItemMask));
}

void iAFoamCharacterizationItemWatershed::setItemMask(const int& _iItemMask)
{
	m_iItemMask = _iItemMask;
}

void iAFoamCharacterizationItemWatershed::setLevel(const double& _dLevel)
{
	m_dLevel = _dLevel;
}

void iAFoamCharacterizationItemWatershed::setThreshold(const double& _dThreshold)
{
	m_dThreshold = _dThreshold;
}

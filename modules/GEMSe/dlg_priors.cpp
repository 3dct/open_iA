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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "dlg_priors.h"

#include "dlg_commoninput.h"
#include "dlg_labels.h"
#include "dlg_modalities.h"
#include "extension2id.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAImageCoordinate.h"
#include "iAImageTypes.h"
#include "iAIO.h"
#include "iASpectrumType.h"
#include "iATypedCallHelper.h"
#include "mdichild.h"
#include "SVMImageFilter.h"

#include <itkBinaryThresholdImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkImageFileWriter.h>

#include <QFileDialog>
#include <QMessageBox>

dlg_priors::dlg_priors(dlg_modalities * dlgModalities, dlg_labels const * dlgLabels):
	m_dlgLabels(dlgLabels),
	m_dlgModalities(dlgModalities)
{
	connect(pbLoad, SIGNAL(clicked()), this, SLOT(Load()));
	connect(pbThresh, SIGNAL(clicked()), this, SLOT(CreateThresh()));
	connect(pbSVM, SIGNAL(clicked()), this, SLOT(CreateSVM()));
}

#include "iAModality.h"

bool dlg_priors::Load(QString const & filename)
{
	ProbabilityImagesType prior;
	iAIO ioThread(0, 0, &prior);
	QString extension = QFileInfo( filename ).suffix();
	extension = extension.toUpper();
	IOType id = extensionToIdStack.find( extension ).value();
	if(!ioThread.setupIO( id, filename ) )
	{
		DEBUG_LOG("Loading Priors failed!\n");
		return false;
	}
	ioThread.start();
	ioThread.wait();

	if (prior.empty())
	{
		DEBUG_LOG("Loading Priors failed (no files found)!\n");
		return false;
	}
	for (int i = 0; i < prior.size(); ++i)
	{
		AddPriorModality(prior[i], i);
	}	
	return true;
}

void dlg_priors::Load()
{
	QString fileName = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		tr("Open Input File"),
		QString() // TODO get directory of current file
		,
		tr("Prior collection file (*.volstack);;All files (*.*)" ) );
	if (fileName.isEmpty())
	{
		return;
	}
	if (!Load(fileName))
	{
		QMessageBox::warning(this, "Segmentation Explorer", "Prior file loading failed!");
	}
}

typedef itk::ImageIOBase::IOComponentType ScalarPixelType;

template <typename T>
void threshold_prior(iAConnector::ImageBaseType* imgBase, dlg_priors::ProbabilityImagesType& priors, double lower, double upper, double outside, double inside)
{
	typedef itk::Image< T, 3>  InputImageType;
	typedef itk::Image< T, 3>  OutputImageType;

	typedef itk::BinaryThresholdImageFilter<InputImageType, OutputImageType> BTIFType;
	typename BTIFType::Pointer filter = BTIFType::New();
	filter->SetLowerThreshold( T(lower) );
	filter->SetUpperThreshold( T(upper) );
	filter->SetOutsideValue( T(outside) );
	filter->SetInsideValue( T(inside) );
	filter->SetInput( dynamic_cast< InputImageType * >( imgBase ) );
	filter->Update();

	// get inverted
	typedef itk::InvertIntensityImageFilter<InputImageType> InvertType;
	typename InvertType::Pointer invert = InvertType::New();
	invert->SetInput(filter->GetOutput());
	invert->SetMaximum(1.0);
	invert->Update();

	typedef itk::CastImageFilter<OutputImageType, itk::Image<double, 3> > CastType;
	typename CastType::Pointer caster1 = CastType::New();
	typename CastType::Pointer caster2 = CastType::New();
	caster1->SetInput(filter->GetOutput());
	caster2->SetInput(invert->GetOutput());


	// add images to list
	iAConnector con1, con2;
	con1.SetImage(caster1->GetOutput());
	con2.SetImage(caster2->GetOutput());

	vtkSmartPointer<vtkImageData> img1 = vtkSmartPointer<vtkImageData>::New();
	img1->DeepCopy(con1.GetVTKImage());
	priors.push_back(img1);
	vtkSmartPointer<vtkImageData> img2 = vtkSmartPointer<vtkImageData>::New();
	img2->DeepCopy(con2.GetVTKImage());
	priors.push_back(img2);
}


void dlg_priors::CreateThresh()
{
	double btlower = 0,
		btupper = 512,
		btoutside = 0,
		btinside = 1;
	QStringList inList = (QStringList() << tr( "#Lower Threshold" ) << tr( "#Upper Threshold" ) << tr( "#Outside Value" ) << tr( "#Inside Value" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( btlower ) << tr( "%1" ).arg( btupper ) << tr( "%1" ).arg( btoutside ) << tr( "%1" ).arg( btinside );
	dlg_commoninput dlg( this, "Binary Threshold", 4, inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;
	btlower = dlg.getValues()[0];
	btupper = dlg.getValues()[1];
	btoutside = dlg.getValues()[2];
	btinside = dlg.getValues()[3];
	//prepare
	QString filterName = tr( "Binary threshold filter" );
	
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	iAConnector con;
	con.SetImage(mdiChild->getImagePointer());
	ProbabilityImagesType prior;
	ITK_TYPED_CALL(threshold_prior, con.GetITKScalarPixelType(),
		con.GetITKImage(), prior, btlower, btupper, btoutside, btinside );

	for (int i=0; i<prior.size(); ++i)
	{
		AddPriorModality(prior[i], i);
	}
}

void dlg_priors::CreateSVM()
{
	// collect parameters:
	QSharedPointer<iAModalityList const> modalities = m_dlgModalities->GetModalities();
	bool ok1 = true, ok2 = true, ok3 = true;
	double param_C = leSVM_C->text().toDouble(&ok2);
	double param_gamma = leSVM_gamma->text().toDouble(&ok1);
	int param_channelCount = leChannelNumber->text().toInt(&ok3);
	if (!ok1 || !ok2 || !ok3)
	{
		QMessageBox::warning(this, "Segmentation Explorer", "Gamma and C parameter need to be valid double values, channelCount needs to be an integer number. Please correct and try again!");
		return;
	}

	SVMImageFilter svm(param_C, param_gamma, modalities, m_dlgLabels->GetSeeds(), param_channelCount);
	svm.Run();

	SVMImageFilter::ProbabilityImagesPointer probabilities = svm.GetResult();
	for (int i=0; i<probabilities->size(); ++i)
	{
		AddPriorModality((*probabilities)[i], i);
	}
}

void dlg_priors::AddPriorModality(vtkSmartPointer<vtkImageData> imgData, int priorNumber)
{
	QSharedPointer<iAModality> mod(new iAModality(QString("Prior ").arg(priorNumber), "", imgData, iAModality::NoRenderer));
	m_dlgModalities->GetModalities()->Add(mod);
}
/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAFuzzyCMeans.h"

#include "iAConnector.h"
#include "iAConsole.h"
#include "iATypedCallHelper.h"

#include <itkFCMClassifierInitializationImageFilter.h>
#include <itkFuzzyClassifierImageFilter.h>
#include <itkVectorImage.h>
#include <itkVectorIndexSelectionCastImageFilter.h>

#include <vtkImageData.h>

namespace
{
	const unsigned int ImageDimension = 3;
}

template <typename InputPixelType>
void fuzzycmeans_template(iAConnector * img, unsigned int maxIter, double maxError, double m, unsigned int numOfThreads, unsigned int numOfClasses,
	QVector<double> const & centroids, bool ignoreBackgroundPixels, double backgroundPixel, QVector<vtkSmartPointer<vtkImageData> > & probOut)
{
	typedef unsigned int LabelPixelType;
	typedef itk::Image<InputPixelType, ImageDimension> InputImageType;
	typedef itk::FuzzyClassifierInitializationImageFilter<InputImageType> TFuzzyClassifier;
	typedef TFuzzyClassifier::MembershipValueType ProbabilityPixelType;
	typedef itk::FCMClassifierInitializationImageFilter<InputImageType> TClassifierFCM;
	typedef itk::VectorImage<ProbabilityPixelType, ImageDimension> VectorImageType;
	typedef itk::Image<ProbabilityPixelType, ImageDimension> ScalarProbabilityImageType;
	typedef itk::VectorIndexSelectionCastImageFilter<VectorImageType, ScalarProbabilityImageType> IndexSelectionType;
	typedef itk::FuzzyClassifierImageFilter<TClassifierFCM::OutputImageType, LabelPixelType> TLabelClassifier;

	auto classifier = TClassifierFCM::New();
	classifier->SetMaximumNumberOfIterations(maxIter);
	classifier->SetMaximumError(maxError);
	classifier->SetM(m);
	classifier->SetNumberOfThreads(numOfThreads);
	classifier->SetNumberOfClasses(numOfClasses);
	TFuzzyClassifier::CentroidArrayType centroidsArray;
	for (int i = 0; i < numOfClasses; i++)
	{
		centroidsArray.push_back(centroids[i]);
	}
	classifier->SetCentroids(centroidsArray);
	classifier->SetIgnoreBackgroundPixels(ignoreBackgroundPixels);
	classifier->SetBackgroundPixel(backgroundPixel);
	classifier->SetInput(dynamic_cast<InputImageType *>(img->GetITKImage()));
	classifier->Update();
	auto probs = classifier->GetOutput();

	for (int p = 0; p < probs->GetVectorLength(); ++p)
	{
		auto indexSelectionFilter = IndexSelectionType::New();
		indexSelectionFilter->SetIndex(p);
		indexSelectionFilter->SetInput(probs);
		indexSelectionFilter->Update();
		iAConnector con;
		con.SetImage(indexSelectionFilter->GetOutput());
		auto vtkImg = vtkSmartPointer<vtkImageData>::New();
		vtkImg->DeepCopy(con.GetVTKImage());
		probOut.push_back(vtkImg);
	}
	auto labelClass = TLabelClassifier::New();
	labelClass->SetInput(probs);
	img->SetImage(labelClass->GetOutput());
}

iAFuzzyCMeans::iAFuzzyCMeans(QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent)
	: iAAlgorithm(fn, i, p, logger, parent)
{}

void iAFuzzyCMeans::setParameters(unsigned int maxIter, double maxError, double m, unsigned int numOfThreads, unsigned int numOfClasses,
	QVector<double> centroids, bool ignoreBg, double bgPixel)
{
	m_maxIter = maxIter;
	m_maxError = maxError;
	m_m = m;
	m_numOfThreads = numOfThreads;
	m_numOfClasses = numOfClasses;
	m_centroids = centroids;
	m_ignoreBg = ignoreBg;
	m_bgPixel = bgPixel;
}

void iAFuzzyCMeans::performWork()
{
	iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
	ITK_TYPED_CALL(fuzzycmeans_template, itkType, getConnector(), m_maxIter, m_maxError, m_m,
		m_numOfThreads, m_numOfClasses, m_centroids, m_ignoreBg, m_bgPixel, m_probOut);
}

QVector<vtkSmartPointer<vtkImageData> > & iAFuzzyCMeans::Probabilities()
{
	return m_probOut;
}




// KFCM


QSharedPointer<iAKFCMFilter> iAKFCMFilter::Create()
{
	return QSharedPointer<iAKFCMFilter>(new iAKFCMFilter());
}

iAKFCMFilter::iAKFCMFilter() :
	iAFilter("Kernelized FCM", "Segmentation",
		"Fuzzy C-Means with spatial constraints based on kernel-induced distance")
{
	m_parameters.push_back(ParamDesc::CreateParam("Maximum Iterations", Discrete  , 500   , 1));
	m_parameters.push_back(ParamDesc::CreateParam("Maximum Error"     , Continuous, 0.0001));
	m_parameters.push_back(ParamDesc::CreateParam("M"                 , Continuous, 2     ));
	m_parameters.push_back(ParamDesc::CreateParam("Alpha"             , Continuous, 1     ));
	m_parameters.push_back(ParamDesc::CreateParam("Number of Classes" , Discrete  , 2     , 1));
	m_parameters.push_back(ParamDesc::CreateParam("Centroids"         , String    , "0 1" ));
	m_parameters.push_back(ParamDesc::CreateParam("Sigma"             , Continuous, 1     ));
	m_parameters.push_back(ParamDesc::CreateParam("StructRadius X"    , Discrete  , 1     , 1));
	m_parameters.push_back(ParamDesc::CreateParam("StructRadius Y"    , Discrete  , 1     , 1));
	m_parameters.push_back(ParamDesc::CreateParam("StructRadius Z"    , Discrete  , 1     , 1));	// (Vector Type ? )
	m_parameters.push_back(ParamDesc::CreateParam("Ignore Background" , Boolean   , false ));
	m_parameters.push_back(ParamDesc::CreateParam("Background Value"  , Continuous, 0     ));
	m_parameters.push_back(ParamDesc::CreateParam("Number of Threads", Discrete, 4, 1));
}

template <typename InputPixelType>
void kfcm_template(iAConnector & inCon, unsigned int maxIter, double maxError,
	double m, double alpha, unsigned int numOfThreads, unsigned int numOfClasses,
	QVector<double> const & centroids, double sigma,
	unsigned int seRadius[3], bool ignoreBackgroundPixels,
	double backgroundPixel,
	vtkSmartPointer<vtkImageData> & out,
	QVector<vtkSmartPointer<vtkImageData> > & probOut)
{
	typedef unsigned int LabelPixelType;
	typedef itk::Image<InputPixelType, ImageDimension> InputImageType;
	//typedef itk::Image<LabelPixelType, ImageDimension> LabelImageType;
	typedef itk::FuzzyClassifierInitializationImageFilter<InputImageType> TFuzzyClassifier;
	typedef itk::KFCMSClassifierInitializationImageFilter<InputImageType> TClassifierKFCMS;
	typedef TClassifierKFCMS::KernelDistanceMetricPointer KernelDistMetricPtr;
	typedef TFuzzyClassifier::CentroidType TCentroid;
	typedef itk::Statistics::RBFKernelInducedDistanceMetric<TCentroid> RBFKernelType;
	typedef itk::FuzzyClassifierImageFilter<TClassifierKFCMS::OutputImageType> TLabelClassifier;
	typedef itk::FlatStructuringElement<ImageDimension> StructuringElementType;

	typedef TFuzzyClassifier::MembershipValueType ProbabilityPixelType;
	typedef itk::VectorImage<ProbabilityPixelType, ImageDimension> VectorImageType;
	typedef itk::Image<ProbabilityPixelType, ImageDimension> ScalarProbabilityImageType;
	typedef itk::VectorIndexSelectionCastImageFilter<VectorImageType, ScalarProbabilityImageType> IndexSelectionType;

	TClassifierKFCMS::Pointer classifier = TClassifierKFCMS::New();
	itk::SimpleFilterWatcher watcher(classifier, "KFCMS classifier");

	// set parameters:
	classifier->SetMaximumNumberOfIterations(maxIter);
	classifier->SetMaximumError(maxError);
	classifier->SetM(m);
	classifier->SetAlpha(alpha);
	classifier->SetNumberOfThreads(numOfThreads);
	classifier->SetNumberOfClasses(numOfClasses);
	TFuzzyClassifier::CentroidArrayType centroidsArray;
	for (int i = 0; i < numOfClasses; i++)
	{
		centroidsArray.push_back(centroids[i]);
	}
	classifier->SetCentroids(centroidsArray);
	RBFKernelType::Pointer kernelDistancePtr = RBFKernelType::New();
	kernelDistancePtr->SetA(2.0);		// make a parameter?
	kernelDistancePtr->SetB(1.0);		// make a parameter?
	kernelDistancePtr->SetSigma(sigma);
	classifier->SetKernelDistanceMetric(static_cast<KernelDistMetricPtr>(kernelDistancePtr));

	StructuringElementType::RadiusType elementRadius;
	for (int i = 0; i < ImageDimension; i++)
	{
		elementRadius[i] = seRadius[i];
	}
	auto structuringElement = StructuringElementType::Box(elementRadius);
	classifier->SetStructuringElement(structuringElement);
	classifier->SetIgnoreBackgroundPixels(ignoreBackgroundPixels);
	classifier->SetBackgroundPixel(backgroundPixel);
	classifier->SetInput(dynamic_cast<InputImageType *>(inCon.GetITKImage()));
	classifier->Update();
	auto probs = classifier->GetOutput();
	for (int p = 0; p < probs->GetVectorLength(); ++p)
	{
		auto indexSelectionFilter = IndexSelectionType::New();
		indexSelectionFilter->SetIndex(p);
		indexSelectionFilter->SetInput(probs);
		indexSelectionFilter->Update();
		iAConnector con;
		con.SetImage(indexSelectionFilter->GetOutput());
		auto vtkImg = vtkSmartPointer<vtkImageData>::New();
		vtkImg->DeepCopy(con.GetVTKImage());
		probOut.push_back(vtkImg);
	}
	TLabelClassifier::Pointer labelClass = TLabelClassifier::New();
	labelClass->SetInput(classifier->GetOutput());
	labelClass->Update();
	iAConnector outCon;
	outCon.SetImage(labelClass->GetOutput());
	out->DeepCopy(outCon.GetVTKImage());
}

void iAKFCMFilter::Run(QMap<QString, QVariant> parameters)
{
	iAConnector con;
	con.SetImage(m_inImg);
	iAConnector::ITKScalarPixelType itkType = con.GetITKScalarPixelType();
	unsigned int numberOfClasses = parameters["Number of Classes"].toUInt();
	QString centroidString = parameters["Centroids"].toString();
	auto centroidStringList = centroidString.split(" ");
	if (centroidStringList.size() != numberOfClasses)
	{
		DEBUG_LOG("Number of classes doesn't match the count of centroids specified!");
		return;
	}
	QVector<double> centroids;
	for (auto c : centroidStringList)
	{
		bool ok;
		double centroid = c.toDouble(&ok);
		if (!ok)
		{
			DEBUG_LOG(QString("Could not convert string in centroid list to double: '%1' !").arg(c));
			return;
		}
		centroids.push_back(centroid);
	}
	unsigned int seRadius[3] = {
		parameters["StructRadius X"].toUInt(),
		parameters["StructRadius Y"].toUInt(),
		parameters["StructRadius Z"].toUInt()
	};
	ITK_TYPED_CALL(kfcm_template, itkType,
		con,
		parameters["Maximum Iterations"].toUInt(),
		parameters["Maximum Error"].toDouble(),
		parameters["M"].toDouble(),
		parameters["Alpha"].toDouble(),
		parameters["Number of Threads"].toUInt(),
		numberOfClasses,
		centroids,
		parameters["Sigma"].toDouble(),
		seRadius,
		parameters["Ignore Background"].toBool(),
		parameters["Background Value"].toDouble(),
		m_outImg,
		m_probOut
	);
}


QVector<vtkSmartPointer<vtkImageData> > & iAKFCMFilter::Probabilities()
{
	return m_probOut;
}

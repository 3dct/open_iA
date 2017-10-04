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
#include <itkKFCMSClassifierInitializationImageFilter.h>
#include <itkMSKFCMClassifierInitializationImageFilter.h>
#include <itkVectorImage.h>
#include <itkVectorIndexSelectionCastImageFilter.h>

#include <vtkImageData.h>

typedef unsigned int LabelPixelType;
typedef itk::Image<ProbabilityPixelType, ImageDimension> ScalarProbabilityImageType;
typedef itk::VectorIndexSelectionCastImageFilter<VectorImageType, ScalarProbabilityImageType> IndexSelectionType;
typedef itk::FuzzyClassifierImageFilter<VectorImageType, LabelPixelType> TLabelClassifier;



QVector<vtkSmartPointer<vtkImageData> > & iAProbabilitySource::Probabilities()
{
	return m_probOut;
}



void iAProbabilitySource::SetProbabilities(VectorImageType::Pointer vectorImg)
{
	for (int p = 0; p < vectorImg->GetVectorLength(); ++p)
	{
		auto indexSelectionFilter = IndexSelectionType::New();
		indexSelectionFilter->SetIndex(p);
		indexSelectionFilter->SetInput(vectorImg);
		indexSelectionFilter->Update();
		iAConnector con;
		con.SetImage(indexSelectionFilter->GetOutput());
		auto vtkImg = vtkSmartPointer<vtkImageData>::New();
		vtkImg->DeepCopy(con.GetVTKImage());
		m_probOut.push_back(vtkImg);
	}
}


// FCM

template <typename InputPixelType>
void fcm_template(iAConnector & img, unsigned int maxIter, double maxError, double m, unsigned int numOfThreads, unsigned int numOfClasses,
	QVector<double> const & centroids, bool ignoreBackgroundPixels, double backgroundPixel,
	vtkSmartPointer<vtkImageData> & outImg,
	iAProbabilitySource & probSource)
{
	typedef itk::Image<InputPixelType, ImageDimension> InputImageType;
	typedef itk::FuzzyClassifierInitializationImageFilter<InputImageType> TFuzzyClassifier;
	typedef itk::FCMClassifierInitializationImageFilter<InputImageType> TClassifierFCM;

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
	classifier->SetInput(dynamic_cast<InputImageType *>(img.GetITKImage()));
	classifier->Update();
	auto probs = classifier->GetOutput();
	probSource.SetProbabilities(probs);
	auto labelClass = TLabelClassifier::New();
	labelClass->SetInput(probs);
	iAConnector outCon;
	outCon.SetImage(labelClass->GetOutput());
	outImg->DeepCopy(outCon.GetVTKImage());
}

namespace
{
	void AddFCMParameters(QVector<QSharedPointer<iAAttributeDescriptor> > & params)
	{
		params.push_back(ParamDesc::CreateParam("Maximum Iterations", Discrete, 500, 1));
		params.push_back(ParamDesc::CreateParam("Maximum Error", Continuous, 0.0001));
		params.push_back(ParamDesc::CreateParam("M", Continuous, 2));
		params.push_back(ParamDesc::CreateParam("Number of Classes", Discrete, 2, 1));
		params.push_back(ParamDesc::CreateParam("Centroids", String, "0 1"));
		params.push_back(ParamDesc::CreateParam("Ignore Background", Boolean, false));
		params.push_back(ParamDesc::CreateParam("Background Value", Continuous, 0));
		params.push_back(ParamDesc::CreateParam("Number of Threads", Discrete, 4, 1));
	}

	void AddKFCMParameters(QVector<QSharedPointer<iAAttributeDescriptor> > & params)
	{
		AddFCMParameters(params);
		params.push_back(ParamDesc::CreateParam("Alpha", Continuous, 1));
		params.push_back(ParamDesc::CreateParam("Sigma", Continuous, 1));
		params.push_back(ParamDesc::CreateParam("StructRadius X", Discrete, 1, 1));
		params.push_back(ParamDesc::CreateParam("StructRadius Y", Discrete, 1, 1));
		params.push_back(ParamDesc::CreateParam("StructRadius Z", Discrete, 1, 1));	// (Vector Type ? )
	}
	
	bool ConvertStringToCentroids(QString centroidString, unsigned int numberOfClasses, QVector<double> & centroids)
	{
		auto centroidStringList = centroidString.split(" ");
		if (centroidStringList.size() != numberOfClasses)
		{
			DEBUG_LOG("Number of classes doesn't match the count of centroids specified!");
			return false;
		}
		for (auto c : centroidStringList)
		{
			bool ok;
			double centroid = c.toDouble(&ok);
			if (!ok)
			{
				DEBUG_LOG(QString("Could not convert string in centroid list to double: '%1' !").arg(c));
				return false;
			}
			centroids.push_back(centroid);
		}
		return true;
	}
}

QSharedPointer<iAFCMFilter> iAFCMFilter::Create()
{
	return QSharedPointer<iAFCMFilter>(new iAFCMFilter());
}

iAFCMFilter::iAFCMFilter() :
	iAFilter("FCM", "Segmentation",
		"Fuzzy C-Means (FCM) Classification<br/><br/>"
		"This implementation is based on Bezdek et al.'s paper \"FCM: The fuzzy"
		"c-means clustering algorithm\" (Computers & Geosciences, 10 (2), 191-203.,"
		"1984).")
{
	AddFCMParameters(m_parameters);
}

void iAFCMFilter::Run(QMap<QString, QVariant> parameters)
{
	unsigned int numberOfClasses = parameters["Number of Classes"].toUInt();
	QVector<double> centroids;
	if (!ConvertStringToCentroids(parameters["Centroids"].toString(), numberOfClasses, centroids))
	{
		return;
	}
	m_outImg = vtkSmartPointer<vtkImageData>::New();
	iAConnector con;
	con.SetImage(m_inImg);
	iAConnector::ITKScalarPixelType itkType = con.GetITKScalarPixelType();
	ITK_TYPED_CALL(fcm_template, itkType,
		con,
		parameters["Maximum Iterations"].toUInt(),
		parameters["Maximum Error"].toDouble(),
		parameters["M"].toDouble(),
		parameters["Number of Threads"].toUInt(),
		numberOfClasses,
		centroids,
		parameters["Ignore Background"].toBool(),
		parameters["Background Value"].toDouble(),
		m_outImg,
		*this
	);
}


// KFCM

QSharedPointer<iAKFCMFilter> iAKFCMFilter::Create()
{
	return QSharedPointer<iAKFCMFilter>(new iAKFCMFilter());
}

iAKFCMFilter::iAKFCMFilter() :
	iAFilter("Kernelized FCM", "Segmentation",
		"Spatially Constrained Fuzzy C-Means based on kernel-induced distance (KFCMS)<br/><br/>"
		"This implementation is based on S.C.Chen. and D.Q.Zhang, \"Robust image segmentation using"
		"FCM with spatial constraints based on new kernel - induced distance measure\". Systems, Man, and"
		"Cybernetics, Part B : Cybernetics, IEEE Transactions on, 34(4) : 1907–1916, 2004. 1, 2.2")
{
	AddKFCMParameters(m_parameters);
}

template <typename InputPixelType>
void kfcm_template(iAConnector & inCon, unsigned int maxIter, double maxError,
	double m, double alpha, unsigned int numOfThreads, unsigned int numOfClasses,
	QVector<double> const & centroids, double sigma,
	unsigned int seRadius[3], bool ignoreBackgroundPixels,
	double backgroundPixel,
	vtkSmartPointer<vtkImageData> & out,
	iAProbabilitySource & probSource)
{
	typedef itk::Image<InputPixelType, ImageDimension> InputImageType;
	typedef itk::FuzzyClassifierInitializationImageFilter<InputImageType> TFuzzyClassifier;
	typedef itk::KFCMSClassifierInitializationImageFilter<InputImageType> TClassifierKFCMS;
	typedef TClassifierKFCMS::KernelDistanceMetricPointer KernelDistMetricPtr;
	typedef TFuzzyClassifier::CentroidType TCentroid;
	typedef itk::Statistics::RBFKernelInducedDistanceMetric<TCentroid> RBFKernelType;
	typedef itk::FlatStructuringElement<ImageDimension> StructuringElementType;

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
	probSource.SetProbabilities(probs);
	TLabelClassifier::Pointer labelClass = TLabelClassifier::New();
	labelClass->SetInput(probs);
	labelClass->Update();
	iAConnector outCon;
	auto itkImg = labelClass->GetOutput();
	outCon.SetImage(itkImg);
	out->DeepCopy(outCon.GetVTKImage());
}

void iAKFCMFilter::Run(QMap<QString, QVariant> parameters)
{
	unsigned int numberOfClasses = parameters["Number of Classes"].toUInt();
	QVector<double> centroids;
	if (!ConvertStringToCentroids(parameters["Centroids"].toString(), numberOfClasses, centroids))
	{
		return;
	}
	unsigned int seRadius[3] = {
		parameters["StructRadius X"].toUInt(),
		parameters["StructRadius Y"].toUInt(),
		parameters["StructRadius Z"].toUInt()
	};
	m_outImg = vtkSmartPointer<vtkImageData>::New();
	iAConnector con;
	con.SetImage(m_inImg);
	iAConnector::ITKScalarPixelType itkType = con.GetITKScalarPixelType();
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
		*this
	);
}


// MSKFCM

QSharedPointer<iAMSKFCMFilter> iAMSKFCMFilter::Create()
{
	return QSharedPointer<iAMSKFCMFilter>(new iAMSKFCMFilter());
}

iAMSKFCMFilter::iAMSKFCMFilter() :
	iAFilter("MSKFCM", "Segmentation",
		"Modified Spatial Kernelized Fuzzy C-Means.<br/><br/>"
		"This implementation is a modified version of the algorithm MSFKCM proposed"
		"by Castro et al.in the paper \"Comparison of various fuzzy clustering"
		"algorithms in the detection of ROI in lung CT and a modified"
		"kernelized - spatial fuzzy c-means algorithm\" (Proc. of 10th IEEE Int. Conf."
		"On Inf.Tech. and Appl.in Biom., Corfu, Greece, 2010).")
{
	AddKFCMParameters(m_parameters);
	m_parameters.push_back(ParamDesc::CreateParam("P", Continuous, 2));
	m_parameters.push_back(ParamDesc::CreateParam("Q", Continuous, 1));
}

template <typename InputPixelType>
void mskfcm_template(iAConnector & inCon, unsigned int maxIter, double maxError,
	double m, double alpha, unsigned int numOfThreads, unsigned int numOfClasses,
	QVector<double> const & centroids, double sigma,
	unsigned int seRadius[3], bool ignoreBackgroundPixels,
	double backgroundPixel, double p, double q,
	vtkSmartPointer<vtkImageData> & out,
	iAProbabilitySource & probSource)
{
	typedef itk::Image<InputPixelType, ImageDimension> InputImageType;
	typedef itk::FuzzyClassifierInitializationImageFilter<InputImageType> TFuzzyClassifier;
	typedef itk::MSKFCMClassifierInitializationImageFilter<InputImageType> TClassifierMSKFCM;
	typedef TClassifierMSKFCM::KernelDistanceMetricPointer KernelDistMetricPtr;

	TClassifierMSKFCM::Pointer classifier = TClassifierMSKFCM::New();
	classifier->SetMaximumNumberOfIterations(maxIter);
	classifier->SetMaximumError(maxError);
	classifier->SetM(m);
	classifier->SetP(p);
	classifier->SetQ(q);
	classifier->SetNumberOfThreads(numOfThreads);
	classifier->SetNumberOfClasses(numOfClasses);
	TFuzzyClassifier::CentroidArrayType centroidsArray;
	for (int i = 0; i < numOfClasses; i++)
	{
		centroidsArray.push_back(centroids[i]);
	}
	classifier->SetCentroids(centroidsArray);
	itk::SimpleFilterWatcher watcher(classifier, "MSKFCM classifier");

	typedef TFuzzyClassifier::CentroidType TCentroid;
	typedef itk::Statistics::RBFKernelInducedDistanceMetric<TCentroid>
		RBFKernelType;
	RBFKernelType::Pointer kernelDistancePtr = RBFKernelType::New();
	kernelDistancePtr->SetA(2.0);		// make a parameter?
	kernelDistancePtr->SetB(1.0);		// make a parameter?
	kernelDistancePtr->SetSigma(sigma);
	classifier->SetKernelDistanceMetric(static_cast<KernelDistMetricPtr>(kernelDistancePtr));
	typedef itk::FlatStructuringElement<ImageDimension> StructuringElementType;
	StructuringElementType::RadiusType elementRadius;
	for (int i = 0; i < ImageDimension; i++)
	{
		elementRadius[i] = seRadius[i];
	}
	auto structuringElement = StructuringElementType::Ball(elementRadius);
	classifier->SetStructuringElement(structuringElement);
	classifier->SetIgnoreBackgroundPixels(ignoreBackgroundPixels);
	classifier->SetBackgroundPixel(backgroundPixel);
	classifier->SetInput(dynamic_cast<InputImageType *>(inCon.GetITKImage()));
	classifier->Update();
	auto probs = classifier->GetOutput();
	probSource.SetProbabilities(probs);
	TLabelClassifier::Pointer labelClass = TLabelClassifier::New();
	labelClass->SetInput(probs);
	labelClass->Update();
	iAConnector outCon;
	auto itkImg = labelClass->GetOutput();
	outCon.SetImage(itkImg);
	out->DeepCopy(outCon.GetVTKImage());
}

void iAMSKFCMFilter::Run(QMap<QString, QVariant> parameters)
{
	unsigned int numberOfClasses = parameters["Number of Classes"].toUInt();
	QVector<double> centroids;
	if (!ConvertStringToCentroids(parameters["Centroids"].toString(), numberOfClasses, centroids))
	{
		return;
	}
	unsigned int seRadius[3] = {
		parameters["StructRadius X"].toUInt(),
		parameters["StructRadius Y"].toUInt(),
		parameters["StructRadius Z"].toUInt()
	};
	m_outImg = vtkSmartPointer<vtkImageData>::New();
	iAConnector con;
	con.SetImage(m_inImg);
	iAConnector::ITKScalarPixelType itkType = con.GetITKScalarPixelType();
	ITK_TYPED_CALL(mskfcm_template, itkType,
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
		parameters["P"].toDouble(),
		parameters["Q"].toDouble(),
		m_outImg,
		*this
	);
}

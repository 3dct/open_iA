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
#include "iAProgress.h"
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

namespace
{
	void AddFCMParameters(iAFilter & filter)
	{
		filter.AddParameter("Maximum Iterations", Discrete, 500, 1);
		filter.AddParameter("Maximum Error", Continuous, 0.0001);
		filter.AddParameter("M", Continuous, 2);
		filter.AddParameter("Number of Classes", Discrete, 2, 1);
		filter.AddParameter("Centroids", String, "0 1");
		filter.AddParameter("Ignore Background", Boolean, false);
		filter.AddParameter("Background Value", Continuous, 0);
		filter.AddParameter("Number of Threads", Discrete, 4, 1);
	}

	void AddKFCMParameters(iAFilter & filter)
	{
		AddFCMParameters(filter);
		filter.AddParameter("Alpha", Continuous, 1);
		filter.AddParameter("Sigma", Continuous, 1);
		filter.AddParameter("StructRadius X", Discrete, 1, 1);
		filter.AddParameter("StructRadius Y", Discrete, 1, 1);
		filter.AddParameter("StructRadius Z", Discrete, 1, 1);	// (Vector Type ? )
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

	bool CheckFCMParameters(QMap<QString, QVariant> parameters)
	{
		unsigned int numberOfClasses = parameters["Number of Classes"].toUInt();
		QVector<double> centroids;
		return ConvertStringToCentroids(parameters["Centroids"].toString(), numberOfClasses, centroids);
	}
}


// FCM

template <typename InputPixelType>
void fcm_template(iAConnector * con, unsigned int maxIter, double maxError, double m,
	unsigned int numOfThreads, unsigned int numOfClasses, QVector<double> const & centroids,
	bool ignoreBackgroundPixels, double backgroundPixel,
	iAProbabilitySource & probSource, iAProgress* p)
{
	typedef itk::Image<InputPixelType, ImageDimension> InputImageType;
	typedef itk::FuzzyClassifierInitializationImageFilter<InputImageType> TFuzzyClassifier;
	typedef itk::FCMClassifierInitializationImageFilter<InputImageType> TClassifierFCM;

	auto classifier = TClassifierFCM::New();
	p->Observe(classifier);
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
	classifier->SetInput(dynamic_cast<InputImageType *>(con->GetITKImage()));
	classifier->Update();
	auto probs = classifier->GetOutput();
	probSource.SetProbabilities(probs);
	auto labelClass = TLabelClassifier::New();
	labelClass->SetInput(probs);
	con->SetImage(labelClass->GetOutput());
}

IAFILTER_CREATE(iAFCMFilter)

iAFCMFilter::iAFCMFilter() :
	iAFilter("FCM", "Segmentation/Fuzzy C-Means",
		"Fuzzy C-Means (FCM) Classification<br/>"
		"This implementation is based on Bezdek et al.'s paper \"FCM: The fuzzy"
		"c-means clustering algorithm\" (Computers & Geosciences, 10 (2), 191-203.,"
		"1984).")
{
	AddFCMParameters(*this);
}

bool iAFCMFilter::CheckParameters(QMap<QString, QVariant> parameters)
{
	return iAFilter::CheckParameters(parameters) && CheckFCMParameters(parameters);
}

void iAFCMFilter::Run(QMap<QString, QVariant> parameters)
{
	unsigned int numberOfClasses = parameters["Number of Classes"].toUInt();
	QVector<double> centroids;
	ConvertStringToCentroids(parameters["Centroids"].toString(), numberOfClasses, centroids);
	iAConnector::ITKScalarPixelType itkType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(fcm_template, itkType,
		m_con,
		parameters["Maximum Iterations"].toUInt(),
		parameters["Maximum Error"].toDouble(),
		parameters["M"].toDouble(),
		parameters["Number of Threads"].toUInt(),
		numberOfClasses,
		centroids,
		parameters["Ignore Background"].toBool(),
		parameters["Background Value"].toDouble(),
		*this, m_progress
	);
}


// KFCM

IAFILTER_CREATE(iAKFCMFilter)

iAKFCMFilter::iAKFCMFilter() :
	iAFilter("Kernelized FCM", "Segmentation/Fuzzy C-Means",
		"Spatially Constrained Fuzzy C-Means based on kernel-induced distance (KFCMS)<br/>"
		"This implementation is based on S.C.Chen. and D.Q.Zhang, \"Robust image segmentation using"
		"FCM with spatial constraints based on new kernel - induced distance measure\". Systems, Man, and"
		"Cybernetics, Part B : Cybernetics, IEEE Transactions on, 34(4) : 1907–1916, 2004. 1, 2.2")
{
	AddKFCMParameters(*this);
}

bool iAKFCMFilter::CheckParameters(QMap<QString, QVariant> parameters)
{
	return iAFilter::CheckParameters(parameters) && CheckFCMParameters(parameters);
}

template <typename InputPixelType>
void kfcm_template(iAConnector * con, unsigned int maxIter, double maxError, double m,
	unsigned int numOfThreads, unsigned int numOfClasses, QVector<double> const & centroids,
	bool ignoreBackgroundPixels, double backgroundPixel, double alpha, double sigma,
	unsigned int seRadius[3], iAProbabilitySource & probSource, iAProgress* p)
{
	typedef itk::Image<InputPixelType, ImageDimension> InputImageType;
	typedef itk::FuzzyClassifierInitializationImageFilter<InputImageType> TFuzzyClassifier;
	typedef itk::KFCMSClassifierInitializationImageFilter<InputImageType> TClassifierKFCMS;
	typedef TClassifierKFCMS::KernelDistanceMetricPointer KernelDistMetricPtr;
	typedef TFuzzyClassifier::CentroidType TCentroid;
	typedef itk::Statistics::RBFKernelInducedDistanceMetric<TCentroid> RBFKernelType;
	typedef itk::FlatStructuringElement<ImageDimension> StructuringElementType;

	TClassifierKFCMS::Pointer classifier = TClassifierKFCMS::New();
	p->Observe(classifier);
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
	classifier->SetInput(dynamic_cast<InputImageType *>(con->GetITKImage()));
	classifier->Update();
	auto probs = classifier->GetOutput();
	probSource.SetProbabilities(probs);
	TLabelClassifier::Pointer labelClass = TLabelClassifier::New();
	labelClass->SetInput(probs);
	labelClass->Update();
	con->SetImage(labelClass->GetOutput());
}

void iAKFCMFilter::Run(QMap<QString, QVariant> parameters)
{
	unsigned int numberOfClasses = parameters["Number of Classes"].toUInt();
	QVector<double> centroids;
	ConvertStringToCentroids(parameters["Centroids"].toString(), numberOfClasses, centroids);
	unsigned int seRadius[3] = {
		parameters["StructRadius X"].toUInt(),
		parameters["StructRadius Y"].toUInt(),
		parameters["StructRadius Z"].toUInt()
	};
	iAConnector::ITKScalarPixelType itkType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(kfcm_template, itkType,
		m_con,
		parameters["Maximum Iterations"].toUInt(),
		parameters["Maximum Error"].toDouble(),
		parameters["M"].toDouble(),
		parameters["Number of Threads"].toUInt(),
		numberOfClasses,
		centroids,
		parameters["Ignore Background"].toBool(),
		parameters["Background Value"].toDouble(),
		parameters["Alpha"].toDouble(),
		parameters["Sigma"].toDouble(),
		seRadius,
		*this, m_progress
	);
}


// MSKFCM

IAFILTER_CREATE(iAMSKFCMFilter)

iAMSKFCMFilter::iAMSKFCMFilter() :
	iAFilter("MSKFCM", "Segmentation/Fuzzy C-Means",
		"Modified Spatial Kernelized Fuzzy C-Means.<br/>"
		"This implementation is a modified version of the algorithm MSFKCM proposed"
		"by Castro et al.in the paper \"Comparison of various fuzzy clustering"
		"algorithms in the detection of ROI in lung CT and a modified"
		"kernelized - spatial fuzzy c-means algorithm\" (Proc. of 10th IEEE Int. Conf."
		"On Inf.Tech. and Appl.in Biom., Corfu, Greece, 2010).")
{
	AddKFCMParameters(*this);
	AddParameter("P", Continuous, 2);
	AddParameter("Q", Continuous, 1);
}

bool iAMSKFCMFilter::CheckParameters(QMap<QString, QVariant> parameters)
{
	return iAFilter::CheckParameters(parameters) && CheckFCMParameters(parameters);
}

template <typename InputPixelType>
void mskfcm_template(iAConnector * con, unsigned int maxIter, double maxError, double m,
	unsigned int numOfThreads, unsigned int numOfClasses, QVector<double> const & centroids,
	bool ignoreBackgroundPixels, double backgroundPixel, double alpha, double sigma,
	unsigned int seRadius[3], double p, double q,
	iAProbabilitySource & probSource, iAProgress* progress)
{
	typedef itk::Image<InputPixelType, ImageDimension> InputImageType;
	typedef itk::FuzzyClassifierInitializationImageFilter<InputImageType> TFuzzyClassifier;
	typedef itk::MSKFCMClassifierInitializationImageFilter<InputImageType> TClassifierMSKFCM;
	typedef TClassifierMSKFCM::KernelDistanceMetricPointer KernelDistMetricPtr;

	TClassifierMSKFCM::Pointer classifier = TClassifierMSKFCM::New();
	progress->Observe(classifier);
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
	classifier->SetInput(dynamic_cast<InputImageType *>(con->GetITKImage()));
	classifier->Update();
	auto probs = classifier->GetOutput();
	probSource.SetProbabilities(probs);
	TLabelClassifier::Pointer labelClass = TLabelClassifier::New();
	labelClass->SetInput(probs);
	labelClass->Update();
	con->SetImage(labelClass->GetOutput());
}

void iAMSKFCMFilter::Run(QMap<QString, QVariant> parameters)
{
	unsigned int numberOfClasses = parameters["Number of Classes"].toUInt();
	QVector<double> centroids;
	ConvertStringToCentroids(parameters["Centroids"].toString(), numberOfClasses, centroids);
	unsigned int seRadius[3] = {
		parameters["StructRadius X"].toUInt(),
		parameters["StructRadius Y"].toUInt(),
		parameters["StructRadius Z"].toUInt()
	};
	iAConnector::ITKScalarPixelType itkType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(mskfcm_template, itkType,
		m_con,
		parameters["Maximum Iterations"].toUInt(),
		parameters["Maximum Error"].toDouble(),
		parameters["M"].toDouble(),
		parameters["Number of Threads"].toUInt(),
		numberOfClasses,
		centroids,
		parameters["Ignore Background"].toBool(),
		parameters["Background Value"].toDouble(),
		parameters["Alpha"].toDouble(),
		parameters["Sigma"].toDouble(),
		seRadius,
		parameters["P"].toDouble(),
		parameters["Q"].toDouble(),
		*this, m_progress
	);
}

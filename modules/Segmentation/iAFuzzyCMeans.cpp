/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include <defines.h>    // for DIM
#include <iAAutoRegistration.h>
#include <iADataSet.h>
#include <iAFilter.h>
#include <iAFilterRegistry.h>
//#include <iAITKIO.h>
#include <iALog.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#include <itkConfigure.h>    // for ITK_VERSION...
#include <itkFCMClassifierInitializationImageFilter.h>
#include <itkFuzzyClassifierImageFilter.h>
#include <itkKFCMSClassifierInitializationImageFilter.h>
#if ITK_VERSION_MAJOR < 5
// MSKFCM filter is implemented with the help of itk Barriers, which got removed with ITK 5
#include <itkMSKFCMClassifierInitializationImageFilter.h>
#endif
#include <itkVectorImage.h>
#include <itkVectorIndexSelectionCastImageFilter.h>

//#include <vtkImageData.h>

#include <itkConfigure.h>    // for ITK_VERSION_MAJOR

typedef iAAttributeDescriptor ParamDesc;

class iAFCMFilter : public iAFilter, private iAAutoRegistration<iAFilter, iAFCMFilter, iAFilterRegistry>
{
public:
	iAFCMFilter();
	bool checkParameters(QVariantMap const & parameters) override;
	void performWork(QVariantMap const & parameters) override;
};

class iAKFCMFilter : public iAFilter, private iAAutoRegistration<iAFilter, iAKFCMFilter, iAFilterRegistry>
{
public:
	iAKFCMFilter();
	bool checkParameters(QVariantMap const & parameters) override;
	void performWork(QVariantMap const & parameters) override;
};

#if ITK_VERSION_MAJOR < 5
class iAMSKFCMFilter : public iAFilter, private iAAutoRegistration<iAFilter, iAMSKFCMFilter, iAFilterRegistry>
{
public:
	iAMSKFCMFilter();
	bool checkParameters(QVariantMap const & parameters) override;
	void performWork(QVariantMap const & parameters) override;
};
#endif

typedef double ProbabilityPixelType;
typedef itk::VectorImage<ProbabilityPixelType, DIM> VectorImageType;
typedef unsigned int LabelPixelType;
typedef itk::Image<ProbabilityPixelType, DIM> ScalarProbabilityImageType;
typedef itk::VectorIndexSelectionCastImageFilter<VectorImageType, ScalarProbabilityImageType> IndexSelectionType;
typedef itk::FuzzyClassifierImageFilter<VectorImageType, LabelPixelType> TLabelClassifier;

namespace
{
	void setProbabilities(VectorImageType::Pointer vectorImg, iAFilter* filter)
	{
		for (unsigned int p = 0; p < vectorImg->GetVectorLength(); ++p)
		{
			auto indexSelectionFilter = IndexSelectionType::New();
			indexSelectionFilter->SetIndex(p);
			indexSelectionFilter->SetInput(vectorImg);
			indexSelectionFilter->Update();
			filter->addOutput(indexSelectionFilter->GetOutput());
		}
	}

	void addFCMParameters(iAFilter & filter)
	{
		filter.addParameter("Maximum Iterations", iAValueType::Discrete, 500, 1);
		filter.addParameter("Maximum Error", iAValueType::Continuous, 0.0001);
		filter.addParameter("M", iAValueType::Continuous, 2, 1.0+std::numeric_limits<double>::epsilon());	// must be larger than 1.0, 1.0 would cause division by zero
		filter.addParameter("Number of Classes", iAValueType::Discrete, 2, 1);
		filter.addParameter("Centroids", iAValueType::String, "0 1");
		filter.addParameter("Ignore Background", iAValueType::Boolean, false);
		filter.addParameter("Background Value", iAValueType::Continuous, 0);
		filter.addParameter("Number of Threads", iAValueType::Discrete, 4, 1);
	}

	void addKFCMParameters(iAFilter & filter)
	{
		addFCMParameters(filter);
		filter.addParameter("Alpha", iAValueType::Continuous, 1);
		filter.addParameter("Sigma", iAValueType::Continuous, 1);
		filter.addParameter("StructRadius X", iAValueType::Discrete, 1, 1);
		filter.addParameter("StructRadius Y", iAValueType::Discrete, 1, 1);
		filter.addParameter("StructRadius Z", iAValueType::Discrete, 1, 1);	// (Vector Type ? )
	}

	bool convertStringToCentroids(QString centroidString, unsigned int numberOfClasses, QVector<double> & centroids)
	{
		auto centroidStringList = centroidString.split(" ");
		if (centroidStringList.size() < 0 || static_cast<unsigned int>(centroidStringList.size()) != numberOfClasses)
		{
			LOG(lvlError, "Number of classes doesn't match the count of centroids specified!");
			return false;
		}
		for (auto c : centroidStringList)
		{
			bool ok;
			double centroid = c.toDouble(&ok);
			if (!ok)
			{
				LOG(lvlError, QString("Could not convert string in centroid list to double: '%1' !").arg(c));
				return false;
			}
			centroids.push_back(centroid);
		}
		return true;
	}

	bool checkFCMParameters(QVariantMap const & parameters)
	{
		unsigned int numberOfClasses = parameters["Number of Classes"].toUInt();
		QVector<double> centroids;
		return convertStringToCentroids(parameters["Centroids"].toString(), numberOfClasses, centroids);
	}
}


// FCM

template <typename InputPixelType>
void fcm(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::Image<InputPixelType, DIM> InputImageType;
	typedef itk::FuzzyClassifierInitializationImageFilter<InputImageType> TFuzzyClassifier;
	typedef itk::FCMClassifierInitializationImageFilter<InputImageType> TClassifierFCM;

	unsigned int numberOfClasses = params["Number of Classes"].toUInt();
	QVector<double> centroids;
	convertStringToCentroids(params["Centroids"].toString(), numberOfClasses, centroids);
	auto classifier = TClassifierFCM::New();
	filter->progress()->observe(classifier);
	classifier->SetMaximumNumberOfIterations(params["Maximum Iterations"].toUInt());
	classifier->SetMaximumError(params["Maximum Error"].toDouble());
	classifier->SetM(params["M"].toDouble());
	classifier->SetNumberOfWorkUnits(params["Number of Threads"].toUInt());
	classifier->SetNumberOfClasses(numberOfClasses);
	typename TFuzzyClassifier::CentroidArrayType centroidsArray;
	for (int i = 0; static_cast<unsigned int>(i) < numberOfClasses; i++)
	{
		centroidsArray.push_back(centroids[i]);
	}
	classifier->SetCentroids(centroidsArray);
	classifier->SetIgnoreBackgroundPixels(params["Ignore Background"].toBool());
	classifier->SetBackgroundPixel(params["Background Value"].toDouble());
	classifier->SetInput(dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()));
	classifier->Update();
	auto probs = classifier->GetOutput();
	auto labelClass = TLabelClassifier::New();
	labelClass->SetInput(probs);
	filter->addOutput(labelClass->GetOutput());
	setProbabilities(probs, filter);
}

iAFCMFilter::iAFCMFilter() :
	iAFilter("FCM", "Segmentation/Fuzzy C-Means",
		"Pixel Classification based on Fuzzy C-Means (FCM). <br/>"
		"This implementation is based on Bezdek et al.'s paper \"FCM: The fuzzy "
		"c-means clustering algorithm\" (Computers & Geosciences, 10 (2), 191-203., "
		"1984).")
{
	addFCMParameters(*this);
}

bool iAFCMFilter::checkParameters(QVariantMap const & parameters)
{
	return iAFilter::checkParameters(parameters) && checkFCMParameters(parameters);
}

void iAFCMFilter::performWork(QVariantMap const & parameters)
{
	for (unsigned int i = 0; i < parameters["Number of Classes"].toUInt(); ++i)
	{
		setOutputName(i + 1, QString("Probability image label %1").arg(i));
	}
	ITK_TYPED_CALL(fcm, inputScalarType(), this, parameters);
}


// KFCM

iAKFCMFilter::iAKFCMFilter() :
	iAFilter("Kernelized FCM", "Segmentation/Fuzzy C-Means",
		"Spatially Constrained Fuzzy C-Means based on kernel-induced distance (KFCMS). <br/>"
		"This implementation is based on S.C.Chen. and D.Q.Zhang, \"Robust image segmentation using "
		"FCM with spatial constraints based on new kernel - induced distance measure\". Systems, Man, and "
		"Cybernetics, Part B : Cybernetics, IEEE Transactions on, 34(4) : 1907–1916, 2004. 1, 2.2")
{
	addKFCMParameters(*this);
}

bool iAKFCMFilter::checkParameters(QVariantMap const & parameters)
{
	return iAFilter::checkParameters(parameters) && checkFCMParameters(parameters);
}

template <typename InputPixelType>
void kfcm(iAFilter* filter, QVariantMap const & parameters)
{
	unsigned int numberOfClasses = parameters["Number of Classes"].toUInt();
	QVector<double> centroids;
	convertStringToCentroids(parameters["Centroids"].toString(), numberOfClasses, centroids);
	unsigned int seRadius[3] = {
		parameters["StructRadius X"].toUInt(),
		parameters["StructRadius Y"].toUInt(),
		parameters["StructRadius Z"].toUInt()
	};
	typedef itk::Image<InputPixelType, DIM> InputImageType;
	typedef itk::FuzzyClassifierInitializationImageFilter<InputImageType> TFuzzyClassifier;
	typedef itk::KFCMSClassifierInitializationImageFilter<InputImageType> TClassifierKFCMS;
	typedef typename TClassifierKFCMS::KernelDistanceMetricPointer KernelDistMetricPtr;
	typedef typename TFuzzyClassifier::CentroidType TCentroid;
	typedef itk::Statistics::RBFKernelInducedDistanceMetric<TCentroid> RBFKernelType;
	typedef itk::FlatStructuringElement<DIM> StructuringElementType;

	auto classifier = TClassifierKFCMS::New();
	filter->progress()->observe(classifier);
	classifier->SetMaximumNumberOfIterations(parameters["Maximum Iterations"].toUInt());
	classifier->SetMaximumError(parameters["Maximum Error"].toDouble());
	classifier->SetM(parameters["M"].toDouble());
	classifier->SetAlpha(parameters["Alpha"].toDouble());
	classifier->SetNumberOfWorkUnits(parameters["Number of Threads"].toUInt());
	classifier->SetNumberOfClasses(numberOfClasses);
	typename TFuzzyClassifier::CentroidArrayType centroidsArray;
	for (int i = 0; static_cast<unsigned int>(i) < numberOfClasses; i++)
	{
		centroidsArray.push_back(centroids[i]);
	}
	classifier->SetCentroids(centroidsArray);
	auto kernelDistancePtr = RBFKernelType::New();
	kernelDistancePtr->SetA(2.0);		// make a parameter?
	kernelDistancePtr->SetB(1.0);		// make a parameter?
	kernelDistancePtr->SetSigma(parameters["Sigma"].toDouble());
	classifier->SetKernelDistanceMetric(static_cast<KernelDistMetricPtr>(kernelDistancePtr));
	typename StructuringElementType::RadiusType elementRadius;
	for (int i = 0; i < DIM; i++)
	{
		elementRadius[i] = seRadius[i];
	}
	auto structuringElement = StructuringElementType::Box(elementRadius);
	classifier->SetStructuringElement(structuringElement);
	classifier->SetIgnoreBackgroundPixels(parameters["Ignore Background"].toBool());
	classifier->SetBackgroundPixel(parameters["Background Value"].toDouble());
	classifier->SetInput(dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()));
	classifier->Update();
	auto probs = classifier->GetOutput();
	TLabelClassifier::Pointer labelClass = TLabelClassifier::New();
	labelClass->SetInput(probs);
	labelClass->Update();
	filter->addOutput(labelClass->GetOutput());
	setProbabilities(probs, filter);
}

void iAKFCMFilter::performWork(QVariantMap const & parameters)
{
	for (unsigned int i = 0; i < parameters["Number of Classes"].toUInt(); ++i)
	{
		setOutputName(i + 1, QString("Probability image label %1").arg(i));
	}
	ITK_TYPED_CALL(kfcm, inputScalarType(), this, parameters);
}


// MSKFCM

#if ITK_VERSION_MAJOR < 5
iAMSKFCMFilter::iAMSKFCMFilter() :
	iAFilter("MSKFCM", "Segmentation/Fuzzy C-Means",
		"Modified Spatial Kernelized Fuzzy C-Means. <br/>"
		"This implementation is a modified version of the algorithm MSFKCM proposed "
		"by Castro et al. in the paper \"Comparison of various fuzzy clustering "
		"algorithms in the detection of ROI in lung CT and a modified "
		"kernelized - spatial fuzzy c-means algorithm\" (Proc. of 10th IEEE Int. Conf. "
		"On Inf.Tech. and Appl.in Biom., Corfu, Greece, 2010).")
{
	addKFCMParameters(*this);
	addParameter("P", iAValueType::Continuous, 2);
	addParameter("Q", iAValueType::Continuous, 1);
}

bool iAMSKFCMFilter::checkParameters(QVariantMap const & parameters)
{
	return iAFilter::checkParameters(parameters) && checkFCMParameters(parameters);
}

template <typename InputPixelType>
void mskfcm(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image<InputPixelType, DIM> InputImageType;
	typedef itk::FuzzyClassifierInitializationImageFilter<InputImageType> TFuzzyClassifier;
	typedef itk::MSKFCMClassifierInitializationImageFilter<InputImageType> TClassifierMSKFCM;
	typedef typename TClassifierMSKFCM::KernelDistanceMetricPointer KernelDistMetricPtr;

	unsigned int numberOfClasses = parameters["Number of Classes"].toUInt();
	QVector<double> centroids;
	convertStringToCentroids(parameters["Centroids"].toString(), numberOfClasses, centroids);
	unsigned int seRadius[3] = {
		parameters["StructRadius X"].toUInt(),
		parameters["StructRadius Y"].toUInt(),
		parameters["StructRadius Z"].toUInt()
	};
	auto classifier = TClassifierMSKFCM::New();
	filter->progress()->observe(classifier);
	classifier->SetMaximumNumberOfIterations(parameters["Maximum Iterations"].toUInt());
	classifier->SetMaximumError(parameters["Maximum Error"].toDouble());
	classifier->SetM(parameters["M"].toDouble());
	//classifier->SetAlpha(parameters["Alpha"].toDouble());
	classifier->SetP(parameters["P"].toDouble());
	classifier->SetQ(parameters["Q"].toDouble());
	classifier->SetNumberOfWorkUnits(parameters["Number of Threads"].toUInt());
	classifier->SetNumberOfClasses(numberOfClasses);
	typename TFuzzyClassifier::CentroidArrayType centroidsArray;
	for (unsigned int i = 0; i < numberOfClasses; i++)
	{
		centroidsArray.push_back(centroids[i]);
	}
	classifier->SetCentroids(centroidsArray);

	typedef typename TFuzzyClassifier::CentroidType TCentroid;
	typedef itk::Statistics::RBFKernelInducedDistanceMetric<TCentroid>
		RBFKernelType;
	auto kernelDistancePtr = RBFKernelType::New();
	kernelDistancePtr->SetA(2.0);		// make a parameter?
	kernelDistancePtr->SetB(1.0);		// make a parameter?
	kernelDistancePtr->SetSigma(parameters["Sigma"].toDouble());
	classifier->SetKernelDistanceMetric(static_cast<KernelDistMetricPtr>(kernelDistancePtr));
	typedef itk::FlatStructuringElement<DIM> StructuringElementType;
	typename StructuringElementType::RadiusType elementRadius;
	for (int i = 0; i < DIM; i++)
	{
		elementRadius[i] = seRadius[i];
	}
	auto structuringElement = StructuringElementType::Ball(elementRadius);
	classifier->SetStructuringElement(structuringElement);
	classifier->SetIgnoreBackgroundPixels(parameters["Ignore Background"].toBool());
	classifier->SetBackgroundPixel(parameters["Background Value"].toDouble());
	classifier->SetInput(dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()));
	classifier->Update();
	auto probs = classifier->GetOutput();
	auto labelClass = TLabelClassifier::New();
	labelClass->SetInput(probs);
	labelClass->Update();
	filter->addOutput(labelClass->GetOutput());
	setProbabilities(probs, filter);
}

void iAMSKFCMFilter::performWork(QVariantMap const & parameters)
{
	for (unsigned int i = 0; i < parameters["Number of Classes"].toUInt(); ++i)
	{
		setOutputName(i + 1, QString("Probability image label %1").arg(i));
	}
	ITK_TYPED_CALL(mskfcm, inputScalarType(), this, parameters);
}
#endif

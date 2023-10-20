// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "svm.h"

#include <defines.h>    // for DIM
#include <iAFilterDefault.h>
#include <iAImageCoordinate.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iAProgress.h>
#include <iASeedType.h>
#include <iAVtkDraw.h>
#include <iAToolsVTK.h>
#include <iATypedCallHelper.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkScalarImageKmeansImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <vtkImageData.h>

IAFILTER_DEFAULT_CLASS(iAKMeans);
IAFILTER_DEFAULT_CLASS(iASVMImageFilter);

namespace
{
	void myNullPrintFunc(char const *)
	{
	}
	const double MY_EPSILON = 1e-6;

	int MapKernelTypeToIndex(QString const & type)
	{
		if (type == "Linear") return 0;
		else if (type == "Polynomial") return 1;
		else if (type == "RBF") return 2;
		else if (type == "Sigmoid") return 3;
		else {
			LOG(lvlError, "Invalid SVM kernel type! Falling back to RBF!");
			return 2;
		}
	}
}

void iASVMImageFilter::performWork(QVariantMap const & parameters)
{
	if (inputCount() == 0)
	{
		LOG(lvlError, "No Input available!");
		return;
	}
	svm_set_print_string_function(myNullPrintFunc); // make libSVM shut up

	svm_parameter param;
	param.svm_type = C_SVC;
	param.kernel_type = MapKernelTypeToIndex(parameters["Kernel Type"].toString());
	param.gamma = parameters["Gamma"].toDouble();
	param.C = parameters["C"].toDouble();
	param.degree = parameters["Degree"].toInt();
	param.coef0 = parameters["Coef0"].toDouble();
	param.probability = 1;

	int const * dim = imageInput(0)->vtkImage()->GetDimensions();
	auto seeds = ExtractSeedVector(parameters["Seeds"].toString(), dim[0], dim[1], dim[2]);

	// default parameters:
	param.nu = 0.5;
	param.cache_size = 100;
	param.eps = 1e-3;
	param.p = 0.1;
	param.shrinking = 0;
	param.nr_weight = 0;
	param.weight_label = nullptr;
	param.weight = nullptr;

	svm_problem problem;
	typedef svm_node* p_svm_node;

	problem.l = seeds->size();
	problem.x = new p_svm_node[seeds->size()];
	problem.y = new double[seeds->size()];
	size_t xspacesize = seeds->size() * (inputCount() + 1);
	svm_node *x_space = new svm_node[xspacesize];

	int curSpaceIdx = 0;
	//if (m_seeds)
	//{
	int labelMin = std::numeric_limits<int>::max();
	int labelMax = std::numeric_limits<int>::min();
	for (int seedIdx = 0; seedIdx < seeds->size(); ++seedIdx)
	{
		iASeedType seed = seeds->at(seedIdx);
		problem.y[seedIdx] = seed.second;
		if (seed.second < labelMin) labelMin = seed.second;
		if (seed.second > labelMax) labelMax = seed.second;
		problem.x[seedIdx] = &x_space[curSpaceIdx];
		for (size_t m = 0; m < inputCount(); ++m)
		{
			x_space[curSpaceIdx].index = static_cast<int>(m);
			x_space[curSpaceIdx].value = imageInput(m)->vtkImage()
				->GetScalarComponentAsDouble(seed.first.x, seed.first.y, seed.first.z, 0);
				// TODO: potentially slow! use GetScalarPointer instead?
			++curSpaceIdx;
		}
		x_space[curSpaceIdx].index = -1;		// for terminating libsvm requires this - very inefficient space usage?
		++curSpaceIdx;
	}
	//}
	// else if (m_trainingValues)
	//{
	//	for (int f = 0; f < seed[j].featureCount(); ++f)
	//	{
	//		x_space[curSpaceIdx].index = f;
	//		x_space[curSpaceIdx].value = seed[j].featureValue(f);
	//	}
	//}
	// } else {
	//	LOG(lvlError, "Neither seeds nor training values specified!");
	//	return false;
	// }

	char const * error = svm_check_parameter(&problem, &param);
	if (error)
	{
		LOG(lvlError, QString("Error in SVM parameters: %1").arg(error));
	}
	// train the model
	svm_model* model = svm_train(&problem, &param);
	int labelCount = labelMax - labelMin + 1;

	QVector<vtkSmartPointer<vtkImageData> > probabilities(labelCount);
	svm_node* node = new svm_node[inputCount() + 1];
	node[inputCount()].index = -1;  // the termination marker
	double const* spc = imageInput(0)->vtkImage()->GetSpacing();
	double * prob_estimates = new double[labelCount];

	for (int l = 0; l < labelCount; ++l)
	{
		probabilities[l] = allocateImage(VTK_DOUBLE, dim, spc, 1);
		prob_estimates[l] = 0;
	}

	// for each pixel, execute svm_predict :
	FOR_VTKIMG_PIXELS(imageInput(0)->vtkImage(), x, y, z)
	{
		for (size_t m = 0; m < inputCount(); ++m)
		{
			node[m].index = m;
			node[m].value = imageInput(m)->vtkImage()->GetScalarComponentAsDouble(x, y, z, 0);
		}
		/*double label =*/ svm_predict_probability(model, node, prob_estimates);
		/*double label2 =*/ svm_predict(model, node);
		double probSum = 0;
		for (int l = 0; l < labelCount; ++l)
		{
			drawPixel(probabilities[l], x, y, z, prob_estimates[l]);
			probSum += prob_estimates[l];
			// DEBUG check begin
			if (prob_estimates[l] < -MY_EPSILON || prob_estimates[l] > 1.0+MY_EPSILON)
			{
				LOG(lvlWarn, QString("SVM: Invalid probability (%1) at %2, %3, %4")
					.arg(prob_estimates[l])
					.arg(x)
					.arg(y)
					.arg(z));
			}
			// DEBUG check end
		}
		// DEBUG check begin
		if (probSum - 1.0 > MY_EPSILON)
		{
			LOG(lvlWarn, QString("SVM: Probabilities at %1, %2, %3 add up to %4 instead of 1!")
				.arg(x)
				.arg(y)
				.arg(z)
				.arg(probSum) );
		}
		// DEBUG check end
	}
	for (int l = 0; l < labelCount; ++l)
	{
		addOutput(std::make_shared<iAImageData>(probabilities[l]));
	}
	delete[] prob_estimates;
	delete[] node;
	delete[] x_space;
	delete[] problem.x;
	delete[] problem.y;
}


iASVMImageFilter::iASVMImageFilter() :
	iAFilter("Probabilistic SVM", "Segmentation/Pixelwise Classification",
		"Classify pixels with Support Vector Machines (SVM).<br/>"
		"The parameters (gamma, dimension and r are depending on the choice of the kernel:"
		"<ul>"
		"<li>The Linear kernel does not consider any of these parameters</li>"
		"<li>The Polynomial kernel considers the Gamma and the Dimension parameter</li>"
		"<li>The RBF kernel only considers the Gamma parameter</li>"
		"<li>The Sigmoid kernel considers the Coef0 and Gamma parameters</li>"
		"</ul></p>"
		"Under seeds, specify text with one seed point per line in the following format :"
		"<pre>x y z label</pre>"
		"where x, y and z are the coordinates(set z = 0 for 2D images) and label is the index of the label "
		"for this seed point. Label indices should start at 0 and be contiguous (so if you have N different "
		"labels, you should use label indices 0..N - 1 and make sure that there is at least one seed per label).")
{
	QStringList kernels; kernels
		<< "Linear" << "Polynomial" << "RBF" << "Sigmoid";
	addParameter("Kernel Type", iAValueType::Categorical, kernels);
	addParameter("Gamma", iAValueType::Continuous, 0.1);
	addParameter("Dimension", iAValueType::Discrete, 2);
	addParameter("Coef0", iAValueType::Continuous, 1);
	addParameter("C", iAValueType::Continuous, 10);
	addParameter("Seeds", iAValueType::Text);
}



template<class T> void kmeansclustering(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image<T, iAITKIO::Dim> ImageType;
	typedef itk::Image<int, iAITKIO::Dim> IntImageType;

	typedef itk::ScalarImageKmeansImageFilter<ImageType, IntImageType> KMeansFilterType;
	auto kmeansFilter = KMeansFilterType::New();
	kmeansFilter->SetInput(dynamic_cast<ImageType*> (filter->imageInput(0)->itkImage()));
	kmeansFilter->SetUseNonContiguousLabels(parameters["Non-contiguous labels"].toBool());
	QStringList means = parameters["Initial means"].toString().split(" ");
	for (QString mean: means)
	{
		kmeansFilter->AddClassWithInitialMean(mean.toDouble());
	}
	filter->progress()->observe(kmeansFilter);
	kmeansFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(kmeansFilter->GetOutput()));
}

void iAKMeans::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(kmeansclustering, inputScalarType(), this, parameters);
}

iAKMeans::iAKMeans() :
	iAFilter("K-Means", "Segmentation/Pixelwise Classification",
		"Classifies the intensity values of a scalar image using the K-Means algorithm.<br/>"
		"Given an input image with scalar values, it uses the K-Means statistical classifier "
		"in order to define labels for every pixel in the image. Under <em>initial means</em>, specify "
		"your estimate for the mean intensity value for each cluster you suspect in the image,"
		"separate the means by spaces. If <em>Non-contiguous labels</em> is checked, the labels "
		"are selected in order to span the dynamic range of the output image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ScalarImageKmeansImageFilter.html\">"
		"Scalar Image K-Means Filter</a> in the ITK documentation.", 1)
{
	addParameter("Initial means", iAValueType::Text, "");
	addParameter("Non-contiguous labels", iAValueType::Boolean, false);
}

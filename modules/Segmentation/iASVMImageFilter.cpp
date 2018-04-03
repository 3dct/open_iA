/*************************************  open_iA  ************************************ *
 * * **********  A tool for scientific visualisation and 3D image processing  ********** *
 * * *********************************************************************************** *
 * * Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
 * *                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
 * * *********************************************************************************** *
 * * This program is free software: you can redistribute it and/or modify it under the   *
 * * terms of the GNU General Public License as published by the Free Software           *
 * * Foundation, either version 3 of the License, or (at your option) any later version. *
 * *                                                                                     *
 * * This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
 * * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
 * * PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
 * *                                                                                     *
 * * You should have received a copy of the GNU General Public License along with this   *
 * * program.  If not, see http://www.gnu.org/licenses/                                  *
 * * *********************************************************************************** *
 * * Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
 * *          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
 * * ************************************************************************************/
#include "iASVMImageFilter.h"

#include "defines.h"    // for DIM
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAImageCoordinate.h"
#include "iAProgress.h"
#include "iASeedType.h"
#include "iAVtkDraw.h"
#include "iAToolsVTK.h"
#include "iATypedCallHelper.h"
#include "svm.h"

#include <itkScalarImageKmeansImageFilter.h>

#include <vtkImageData.h>

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
			DEBUG_LOG("Invalid SVM kernel type! Falling back to RBF!");
			return 2;
		}
	}
}

void iASVMImageFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	if (Input().size() == 0)
	{
		DEBUG_LOG("No Input available!");
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

	int const * dim = Input()[0]->GetVTKImage()->GetDimensions();
	auto seeds = ExtractSeedVector(parameters["Seeds"].toString(), dim[0], dim[1], dim[2]);

	// default parameters:
	param.nu = 0.5;
	param.cache_size = 100;
	param.eps = 1e-3;
	param.p = 0.1;
	param.shrinking = 0;
	param.nr_weight = 0;
	param.weight_label = NULL;
	param.weight = NULL;

	svm_problem problem;
	typedef svm_node* p_svm_node;

	problem.l = seeds->size();
	problem.x = new p_svm_node[seeds->size()];
	problem.y = new double[seeds->size()];
	size_t xspacesize = seeds->size() * (Input().size() + 1);
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
		for (int m = 0; m < Input().size(); ++m)
		{
			x_space[curSpaceIdx].index = m;
			x_space[curSpaceIdx].value = Input()[m]->GetVTKImage()
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
	//	DEBUG_LOG("Neither seeds nor training values specified!");
	//	return false;
	// }

	char const * error = svm_check_parameter(&problem, &param);
	if (error)
	{
		DEBUG_LOG(QString("Error in SVM parameters: %1").arg(error));
	}
	// train the model
	svm_model* model = svm_train(&problem, &param);
	int labelCount = labelMax - labelMin + 1;

	QVector<vtkSmartPointer<vtkImageData> > probabilities(labelCount);
	svm_node* node = new svm_node[Input().size() + 1];
	node[Input().size()].index = -1;	// the termination marker
	double const* spc = Input()[0]->GetVTKImage()->GetSpacing();
	double * prob_estimates = new double[labelCount];

	for (int l = 0; l < labelCount; ++l)
	{
		probabilities[l] = AllocateImage(VTK_DOUBLE, dim, spc, 1);
		prob_estimates[l] = 0;
	}

	// for each pixel, execute svm_predict :
	FOR_VTKIMG_PIXELS(Input()[0]->GetVTKImage(), x, y, z)
	{
		for (int m = 0; m < Input().size(); ++m)
		{
			node[m].index = m;
			node[m].value = Input()[m]->GetVTKImage()->GetScalarComponentAsDouble(x, y, z, 0);
		}
		double label = svm_predict_probability(model, node, prob_estimates);
		double label2 = svm_predict(model, node);
		double probSum = 0;
		for (int l = 0; l < labelCount; ++l)
		{
			drawPixel(probabilities[l], x, y, z, prob_estimates[l]);
			probSum += prob_estimates[l];
			// DEBUG check begin
			if (prob_estimates[l] < -MY_EPSILON || prob_estimates[l] > 1.0+MY_EPSILON)
			{
				DEBUG_LOG(QString("SVM: Invalid probability (%1) at %2, %3, %4")
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
			DEBUG_LOG(QString("SVM: Probabilities at %1, %2, %3 add up to %4 instead of 1!")
				.arg(x)
				.arg(y)
				.arg(z)
				.arg(probSum) );
		}
		// DEBUG check end
	}
	for (int l = 0; l < labelCount; ++l)
	{
		AddOutput(probabilities[l]);
	}
	delete[] prob_estimates;
	delete[] node;
	delete[] x_space;
	delete[] problem.x;
	delete[] problem.y;
}


IAFILTER_CREATE(iASVMImageFilter)

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
	AddParameter("Kernel Type", Categorical, kernels);
	AddParameter("Gamma", Continuous, 0.1);
	AddParameter("Dimension", Discrete, 2);
	AddParameter("Coef0", Continuous, 1);
	AddParameter("C", Continuous, 10);
	AddParameter("Seeds", Text);
}



template<class T> void kmeansclustering(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::Image<int, DIM> IntImageType;

	typedef itk::ScalarImageKmeansImageFilter<ImageType, IntImageType> KMeansFilterType;
	auto kmeansFilter = KMeansFilterType::New();
	kmeansFilter->SetInput(dynamic_cast<ImageType*> (filter->Input()[0]->GetITKImage()));
	kmeansFilter->SetUseNonContiguousLabels(parameters["Non-contiguous labels"].toBool());
	QStringList means = parameters["Initial means"].toString().split(" ");
	for (QString mean: means)
	{
		kmeansFilter->AddClassWithInitialMean(mean.toDouble());
	}
	filter->Progress()->Observe(kmeansFilter);
	kmeansFilter->Update();
	filter->AddOutput(kmeansFilter->GetOutput());
}

void iAKMeans::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(kmeansclustering, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAKMeans)

iAKMeans::iAKMeans() :
	iAFilter("K-Means", "Segmentation/Pixelwise Classification",
		"Classifies the intensity values of a scalar image using the K-Means algorithm..<br/>"
		"Given an input image with scalar values, it uses the K-Means statistical classifier "
		"in order to define labels for every pixel in the image. Under <em>initial means</em>, specify "
		"your estimate for the mean intensity value for each cluster you suspect in the image,"
		"separate the means by spaces. If <em>Non-contiguous labels</em> is checked, the labels "
		"are selected in order to span the dynamic range of the output image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ScalarImageKmeansImageFilter.html\">"
		"Scalar Image K-Means Filter</a> in the ITK documentation.", 2)
{
	AddParameter("Initial means", Text, "");
	AddParameter("Non-contiguous labels", Boolean, false);
}

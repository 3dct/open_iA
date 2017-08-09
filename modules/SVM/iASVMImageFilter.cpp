/************************************  SVMERW  ****************************************
* **  Command line tool for segmenting multi-modal 3D data with a Support Vector   ** *
* **  Machine (SVM) and the Extended Random Walker (ERW)                           ** *
***************************************************************************************
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
***************************************************************************************
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
***************************************************************************************
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*       Stelzhamerstraße 23, 4600 Wels / Austria, Email: bernhard.froehler@fh-wels.at *
**************************************************************************************/
#include "iASVMImageFilter.h"

#include "iAConsole.h"
#include "iAImageCoordinate.h"
#include "iASeedType.h"
#include "iAVtkDraw.h"
#include "iAToolsVTK.h"
#include "svm.h"

#include <vtkImageData.h>

iASVMImageFilter::iASVMImageFilter(vtkImageData* i, iALogger* l):
	iAAlgorithm("SVM", i, NULL, l, NULL),
	m_probabilities(new QVector<ImagePointer>)
{}


void iASVMImageFilter::AddInput(ImagePointer input)
{
	m_input.push_back(input);
}

void iASVMImageFilter::SetParameters(int kernel, double c, double gamma, int degree, double coef0)
{
	m_c = c;
	m_gamma = gamma;
	m_coef0 = coef0;
	m_degree = degree;
	m_kernel = kernel;
}

void iASVMImageFilter::SetSeeds(iASeedsPointer seeds)
{
	m_seeds = seeds;
}

namespace
{
	void myNullPrintFunc(char const *)
	{
	}
	const double MY_EPSILON = 1e-6;
}

void iASVMImageFilter::performWork()
{
	if (m_input.size() == 0)
	{
		DEBUG_LOG("No Input available!");
		return;
	}
	svm_set_print_string_function(myNullPrintFunc); // make libSVM shut up

	svm_parameter param;
	param.svm_type = C_SVC;
	param.kernel_type = m_kernel;
	param.gamma = m_gamma;
	param.C = m_c;
	param.degree = m_degree;
	param.coef0 = m_coef0;
	param.probability = 1;

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

	problem.l = m_seeds->size();
	problem.x = new p_svm_node[m_seeds->size()];
	problem.y = new double[m_seeds->size()];
	svm_node *x_space = new svm_node[m_seeds->size() * (m_input.size() + 1)];

	int curSeedIdx = 0;
	int curSpaceIdx = 0;
	//if (m_seeds)
	//{
	int labelMin = std::numeric_limits<int>::max();
	int labelMax = std::numeric_limits<int>::min();
	for (int seedIdx = 0; seedIdx < m_seeds->size(); ++seedIdx)
	{
		iASeedType seed = m_seeds->at(seedIdx);
		problem.y[curSeedIdx] = seed.second;
		if (seed.second < labelMin) labelMin = seed.second;
		if (seed.second > labelMax) labelMax = seed.second;
		problem.x[curSeedIdx] = &x_space[curSpaceIdx];
		for (int m = 0; m < m_input.size(); ++m)
		{
			x_space[curSpaceIdx].index = m;
			x_space[curSpaceIdx].value = m_input[m]->GetScalarComponentAsDouble(seed.first.x, seed.first.y, seed.first.z, 0);
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

	// DEBUG OUTPUT -->
	/*
	DebugOut() << std::endl << "# of classes: " <<  model->nr_class << std::endl
	<< "# of SV: " << model->l << std::endl;
	for (int s=0; s<model->l; ++s)
	{
	DebugOut() << "    SV["<<s<<"]=(";
	for (int c=0; c<channelCount; ++c)
	{
	DebugOut() << model->SV[s][c].value << " ";
	}
	DebugOut() <<")"<<std::endl;
	}
	DebugOut() << "sv_coefs: "<< std::endl;
	for (int c=0; c<labelCount-1; ++c)
	{
	DebugOut()  << "    ";
	for (int s=0; s<model->l; ++s)
	{
	DebugOut() << model->sv_coef[c][s] << " ";
	}
	DebugOut() << std::endl;
	}
	DebugOut() << std::endl << "Labels:" << std::endl;
	std::ostringstream probAStr, probBStr;
	for (int l=0; l<labelCount; ++l)
	{
	DebugOut() << "    " << l  << ": " << model->label[l] << "; num of SV: " << model->nSV[l] << std::endl;
	probAStr << model->probA[l] << " ";
	probBStr << model->probB[l] << " ";
	}
	DebugOut() << "ProbA: " << probAStr.str() << std::endl
	<< "ProbB: " << probBStr.str() << std::endl;

	if (svm_check_probability_model(model) == 0)
	{
	DebugOut () << "Mode is NOT suitable for probability estimates!" << std::endl;
	}
	*/
	// DEBUG OUTPUT <--

	int labelCount = labelMax - labelMin + 1;

	m_probabilities->clear();
	m_probabilities->resize(labelCount);

	svm_node* node = new svm_node[m_input.size() + 1];
	node[m_input.size()].index = -1;	// the termination marker
	int const * dim = m_input[0]->GetDimensions();
	double const* spc = m_input[0]->GetSpacing();
	double * prob_estimates = new double[labelCount];

	for (int l = 0; l < labelCount; ++l)
	{
		(*m_probabilities)[l] = AllocateImage(VTK_DOUBLE, dim, spc, 1);
		prob_estimates[l] = 0;
	}

	// for each pixel, execute svm_predict :
	FOR_VTKIMG_PIXELS(m_input[0], x, y, z)
	{
		/*
		if (doDebug(coord))
		DebugOut() << "Pixel " << coord.x << ", " << coord.y << ", " << coord.z << ": channels=";
		*/

		for (int m = 0; m < m_input.size(); ++m)
		{
			node[m].index = m;
			node[m].value = m_input[m]->GetScalarComponentAsDouble(x, y, z, 0);
			++m;
		}

		// DEBUG OUTPUT -->
		/*
		if (doDebug(coord))
		{
		for (int i=0; i<channelCount; ++i)
		{
		DebugOut() << node[i].value;
		if (i < channelCount-1)
		{
		DebugOut() << ",";
		}
		}
		DebugOut() << "; probabilities=";
		}
		*/
		// DEBUG OUTPUT <--

		double label = svm_predict_probability(model, node, prob_estimates);
		double label2 = svm_predict(model, node);

		/*
		if (doDebug(coord))
		{
		for (int l=0; l<labelCount; ++l)
		{
		DebugOut() << prob_estimates[l] << ",";
		}
		DebugOut() << " label=" << label << ", label2=" << label2 << std::endl;
		}
		*/
		double probSum = 0;
		for (int l = 0; l < labelCount; ++l)
		{
			drawPixel((*m_probabilities)[l], x, y, z, prob_estimates[l]);

			probSum += prob_estimates[l];

			if (prob_estimates[l] < -MY_EPSILON || prob_estimates[l] > 1.0+MY_EPSILON)
			{
				DEBUG_LOG(QString("SVM: Invalid probability (%1) at %2, %3, %4")
					.arg(prob_estimates[l])
					.arg(x)
					.arg(y)
					.arg(z));
			}
		}
		if (probSum - 1.0 > MY_EPSILON)
		{
			DEBUG_LOG(QString("SVM: Probabilities at %1, %2, %3 add up to %4 instead of 1!")
				.arg(x)
				.arg(y)
				.arg(z)
				.arg(probSum) );
		}
	}

	delete[] prob_estimates;
	delete[] node;
	delete[] x_space;
	delete[] problem.x;
	delete[] problem.y;
}


iASVMImageFilter::ImagesPointer iASVMImageFilter::GetResult()
{
	return m_probabilities;
}

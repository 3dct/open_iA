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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "pch.h"
#include "SVMImageFilter.h"

#include "iAConsole.h"
#include "iAImageCoordinate.h"
#include "iASpectrumType.h"
#include "iAModality.h"
#include "iAVtkDraw.h"
#include "svm.h"

SVMImageFilter::SVMImageFilter(double c, double gamma,
	ModalitiesPointer modalities,
	SeedsPointer seeds,
	int channelCount):
	m_c(c),
	m_gamma(gamma),
	m_modalities(modalities),
	m_seeds(seeds),
	m_channelCount(channelCount),
	m_probabilities(new ProbabilityImagesType)
{
	
}

void myNullPrintFunc(char const *)
{
}

void SVMImageFilter::Run()
{
	svm_set_print_string_function(myNullPrintFunc); // make libSVM shut up


	svm_parameter param;
	param.svm_type = C_SVC;
	param.kernel_type = RBF;
	param.gamma = m_gamma;
	param.C = m_c;
	param.probability = 1;

	// default parameters:
	param.degree = 3;
	param.coef0 = 0;
	param.nu = 0.5;
	param.cache_size = 100;
	param.eps = 1e-3;
	param.p = 0.1;
	param.shrinking = 0;
	param.nr_weight = 0;
	param.weight_label = NULL;
	param.weight = NULL;

	svm_problem problem;

	int labelCount = m_seeds->size();
	int seedPointCount = 0;
	for (int i = 0; i < m_seeds->size(); ++i)
	{
		seedPointCount += (*m_seeds)[i].size();
	}
	int modalityCount = m_modalities->size();

	int channelCount = m_channelCount;

	int maxChannelCount = 0;
	for (int m=0; m<modalityCount; ++m)
	{
		maxChannelCount += static_cast<int>(m_modalities->Get(m)->GetData()->channelCount());
	}
	channelCount = std::min(channelCount, maxChannelCount);

	typedef svm_node* p_svm_node;

	problem.l = seedPointCount;
	problem.x = new p_svm_node[seedPointCount];
	problem.y = new double[seedPointCount];
	svm_node *x_space = new svm_node[seedPointCount * (channelCount + 1)];

	int curSeedIdx = 0;
	int curSpaceIdx = 0;
	//DebugOut() << "Seeds: ";
	for (int label = 0; label < labelCount; ++label)
	{
		QList<iAImageCoordinate> coords = (*m_seeds)[label];
		for (int j = 0; j < coords.size(); ++j)
		{
			//DebugOut() << std::endl << curSeedIdx << "(" << label << ") ";
			problem.y[curSeedIdx] = label;
			problem.x[curSeedIdx] = &x_space[curSpaceIdx];
			int curChannelIdx = 0;
			for (int m = 0; m < modalityCount; ++m)
			{
				QSharedPointer<iAModality const> modality = m_modalities->Get(m);
				auto idx = modality->GetConverter().GetIndexFromCoordinates(coords[j]);
				for (int c = 0; c < modality->GetData()->channelCount(); ++c)
				{
					x_space[curSpaceIdx].index = curChannelIdx;
					x_space[curSpaceIdx].value = modality->GetData()->get(idx, c);
					//DebugOut() << x_space[curSpaceIdx].index << ": " << x_space[curSpaceIdx].value << "  ";
					++curChannelIdx;
					++curSpaceIdx;
					//TODO: let user select which channels to pick!
					if (curChannelIdx == channelCount)
					{
						break;
					}
				}
				//TODO: let user select which channels to pick!
				if (curChannelIdx == channelCount)
				{
					break;
				}
			}
			x_space[curSpaceIdx].index = -1;		// for terminating libsvm requires this - very inefficient space usage?
			++curSpaceIdx;
			++curSeedIdx;
		}
	}

	char const * error = svm_check_parameter(&problem, &param);
	if (error)
	{
		DEBUG_LOG(QString("Error in SVM parameters: %1.").arg(error));
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

	QSharedPointer<iAModality const> refMod = m_modalities->Get(0);
	// create output: 1 prob. image per label

	m_probabilities->clear();
	m_probabilities->resize(labelCount);

	svm_node* node = new svm_node[channelCount + 1];
	node[channelCount].index = -1;	// the termination marker

	double * prob_estimates = new double[labelCount];

	for (int l = 0; l < labelCount; ++l)
	{
		(*m_probabilities)[l] = vtkSmartPointer<vtkImageData>::New();
		(*m_probabilities)[l]->SetExtent(0, refMod->GetWidth() - 1, 0, refMod->GetHeight() - 1, 0, refMod->GetDepth() - 1);
		double const* spc = refMod->GetSpacing();
		(*m_probabilities)[l]->SetSpacing(spc[0], spc[1], spc[2]);
		(*m_probabilities)[l]->AllocateScalars(VTK_DOUBLE, 1);
		prob_estimates[l] = 0;
	}

	// for each pixel, execute svm_predict :
	for (int i = 0; i < refMod->GetData()->size(); ++i)
	{
		iAImageCoordinate coord(refMod->GetConverter().GetCoordinatesFromIndex(i));
		/*
		if (doDebug(coord))
		DebugOut() << "Pixel " << coord.x << ", " << coord.y << ", " << coord.z << ": channels=";
		*/

		int curChannelIdx = 0;
		for (int m = 0; m < modalityCount; ++m)
		{
			QSharedPointer<iAModality const> modality = m_modalities->Get(m);
			for (int c = 0; c < modality->GetData()->channelCount(); ++c)
			{
				node[curChannelIdx].index = curChannelIdx;
				node[curChannelIdx].value = modality->GetData()->get(i, c);
				++curChannelIdx;
				//TODO: let user select which channels to pick!
				if (curChannelIdx == channelCount)
				{
					break;
				}
			}
			//TODO: let user select which channels to pick!
			if (curChannelIdx == channelCount)
			{
				break;
			}
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
			drawPixel((*m_probabilities)[l], coord.x, coord.y, coord.z, prob_estimates[l]);

			probSum += prob_estimates[l];

			if (prob_estimates[l] < 0.0 || prob_estimates[l] > 1.0)
			{
				DEBUG_LOG(QString("SVM: Invalid probability (%1) at %2, %3, %4")
					.arg(prob_estimates[l])
					.arg(coord.x)
					.arg(coord.y)
					.arg(coord.z));
			}
		}
		if (probSum - 1.0 > std::numeric_limits<float>::epsilon())
		{
			DEBUG_LOG(QString("SVM: Probabilities at %1, %2, %3 add up to %4 instead of 1!")
				.arg(coord.x)
				.arg(coord.y)
				.arg(coord.z)
				.arg(probSum) );
		}
	}

	delete[] prob_estimates;
	delete[] node;
	delete[] x_space;
	delete[] problem.x;
	delete[] problem.y;
}


SVMImageFilter::ProbabilityImagesPointer SVMImageFilter::GetResult()
{
	return m_probabilities;
}

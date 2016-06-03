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
#include "iAMMSegSampler.h"

#include "iAConsole.h"
#include "iACharacteristics.h"
#include "iAAttributes.h"
#include "iAAttributeDescriptor.h"
#include "iAImageCoordinate.h"
#include "iAMMSegParameter.h"
#include "iAMMSegParameterGenerator.h"
#include "iAMMSegParameterRange.h"
#include "iASingleResult.h"
#include "iASamplingResults.h"
#include "iAModality.h"
#include "iANormalizerImpl.h"
#include "iAPCA.h"
#include "iASpectraDistance.h"
#include "SVMImageFilter.h"

#include <QMap>
#include <QTextStream>

const int CONCURRENT_ERW_RUNS = 1;

iAMMSegSampler::iAMMSegSampler(
		QSharedPointer<iAModalityList const> modalities,
		QSharedPointer<iAMMSegParameterRange> range,
		QSharedPointer<iAMMSegParameterGenerator> sampleGenerator,
		SVMImageFilter::SeedsPointer seeds,
		QString const & outputBaseDir,
		QString const & parameterRangeFile,
		QString const & parameterSetFile,
		QString const & characteristicsFile,
		bool storeProbabilities) :
	m_modalities(modalities),
	m_range(range),
	m_sampleGenerator(sampleGenerator),
	m_seeds(seeds),
	m_curLoop(0),
	m_parameters(0),
	m_outputBaseDir(outputBaseDir),
	m_aborted(false),
	m_storeProbabilities(storeProbabilities),
	m_parameterRangeFile (parameterRangeFile),
	m_parameterSetFile   (parameterSetFile),
	m_characteristicsFile(characteristicsFile),
	m_runningOperations(0),
	m_calcERWDuration(0),
	m_calcChaDuration(0)
{
}

void iAMMSegSampler::StatusMsg(QString const & msg)
{
	emit Status(msg);
	DEBUG_LOG(msg+"\n");
}


void iAMMSegSampler::run()
{
	if (m_range->modalityParamRange.size() < 1)
	{
		DEBUG_LOG("At least one modality is required!\n");
		return;
	}
	StatusMsg("Generating sampling parameter sets...");
	m_parameters = m_sampleGenerator->GetParameterSets(m_range);
	if (!m_parameters)
	{
		DEBUG_LOG("No Parameters available!\n");
		return;
	}

	QMap<std::pair<int, int>, QSharedPointer<iASpectralVoxelData const> > m_pcaReducedSpectralData;

	// generate AttributeDescriptor:

	QString descriptor = QString("ERW Beta	Parameter	Continuous	%1	%2	%3\n")
		.arg(m_range->erw_beta_From)
		.arg(m_range->erw_beta_To)
		.arg(m_range->erw_beta_logScale ? "Logarithmic" : "Linear");
	descriptor += QString("ERW Gamma	Parameter	Continuous	%1	%2	%3\n")
		.arg(m_range->erw_gamma_From)
			.arg(m_range->erw_gamma_To)
			.arg(m_range->erw_gamma_logScale ? "Logarithmic" : "Linear");
	descriptor += QString("ERW Max.Iter.	Parameter	Discrete	%1	%2	%3\n")
		.arg(m_range->erw_maxIter_From)
			.arg(m_range->erw_maxIter_To)
			.arg(m_range->erw_maxIter_logScale ? "Logarithmic" : "Linear");
	descriptor += QString("SVM C	Parameter	Continuous	%1	%2	%3\n")
		.arg(m_range->svm_C_From)
			.arg(m_range->svm_C_To)
			.arg(m_range->svm_C_logScale ? "Logarithmic" : "Linear");
	descriptor += QString("SVM Gamma	Parameter	Continuous	%1	%2	%3\n")
			.arg(m_range->svm_gamma_From)
			.arg(m_range->svm_gamma_To)
			.arg(m_range->svm_gamma_logScale ? "Logarithmic" : "Linear");
	descriptor += QString("SVM Channels	Parameter	Discrete	%1	%2	%3\n")
			.arg(m_range->svm_channels_From)
			.arg(m_range->svm_channels_To)
			.arg("Linear");

	for (int i = 0; i < m_range->modalityParamRange.size(); ++i)
	{
		QString distFuncStr;
		int distFncCnt = m_range->modalityParamRange[i].distanceFuncs.size();
		for (int j = 0; j < distFncCnt; ++j)
		{
			distFuncStr += m_range->modalityParamRange[i].distanceFuncs[j]->GetShortName();
			if (j < distFncCnt - 1)
			{
				distFuncStr += ",";
			}
		}
		descriptor += QString("Mod%1 PCA Components	Parameter	Discrete	%2	%3	Linear\n"
			"Mod%1 Weight	Parameter	Continuous	%4	%5	Linear\n"
			"Mod%1 Distance Functions	Parameter	Categorical	%6\n")
			.arg(i)
			.arg(m_range->modalityParamRange[i].pcaDimMin)
			.arg(m_range->modalityParamRange[i].pcaDimMax)
			.arg(m_range->modalityParamRange[i].weightFrom)
			.arg(m_range->modalityParamRange[i].weightTo)
			.arg(distFuncStr);
	}
	descriptor += QString("Object Count	Derived Output	Discrete	10000000000	1\n"
		"Duration	Derived Output	Continuous	10000000000	0\n");

	QTextStream textStream(&descriptor);

	m_attributes = iAAttributes::Create(textStream);

	m_derivedOutputStart = NonModalityParamCount + m_range->modalityParamRange.size() * ModalityParamCount;

	m_results = QSharedPointer<iASamplingResults>(new iASamplingResults(m_attributes, m_sampleGenerator->GetName()));
	for (m_curLoop=0; !m_aborted && m_curLoop<m_parameters->size(); ++m_curLoop)
	{
		QSharedPointer<iAMMSegParameter> param = (*m_parameters)[m_curLoop];
		param->setID(m_curLoop);
		while (m_runningERW.size() >= CONCURRENT_ERW_RUNS && !m_aborted)
		{
			QThread::msleep(100);
		}
		if (m_aborted)
		{
			break;
		}
		StatusMsg(QString("SVM Run %1").arg(m_curLoop));
		iAPerformanceTimer svmPerfTimer;
		svmPerfTimer.start();
		SVMImageFilter svm(
			param->svm_c(),
			param->svm_gamma(),
			m_modalities,
			m_seeds,
			param->svm_channels());
		svm.Run();

		double svmTime = svmPerfTimer.elapsed();
		m_calcERWDuration += svmTime;
		
		
		StatusMsg(QString("SVM finished in %1 seconds; starting ERW Run %2")
			.arg(QString::number(svmTime))
			.arg(QString::number(m_curLoop)));

#if _DEBUG
		DEBUG_LOG(QString("%1: beta=%2, gamma=%3\n")
			.arg(m_curLoop)
			.arg(param->erw_beta())
			.arg(param->erw_gamma()));
		for (int i=0; i<param->modalityCount(); ++i)
		{
			DEBUG_LOG(QString(", modality %1 (weight=%2, metric=%3, pcaDim=%4)\n")
				.arg(i + 1)
				.arg(param->modalityWeight(i))
				.arg(param->distanceFunc(i)->GetShortName())
				.arg(param->pcaDim(i)));
		}
		DEBUG_LOG("\n");
#endif

		QSharedPointer<iAGaussianNormalizer> gauss(new iAGaussianNormalizer);
		gauss->SetBeta(param->erw_beta());
			
		QSharedPointer<QVector<iARWInputChannel> > inputChannels(new QVector<iARWInputChannel>());
		for (int i=0; i<param->modalityCount(); ++i)
		{
			iARWInputChannel input;
			QSharedPointer<iAModality const> modI = m_modalities->Get(i);
			if (modI->GetData()->channelCount() != param->pcaDim(i))
			{
				if (!m_pcaReducedSpectralData.contains(std::pair<int, int>(i, param->pcaDim(i))))
				{
					iAPCA pca(modI->GetData());
					m_pcaReducedSpectralData.insert(std::pair<int, int>(i, param->pcaDim(i)),
						pca.GetReduced(modI->GetConverter(), param->pcaDim(i)));
				}
				input.image = m_pcaReducedSpectralData[std::pair<int, int>(i, param->pcaDim(i))];
			}
			else
			{
				input.image = modI->GetData();
			}
			input.distanceFunc = param->distanceFunc(i);
			input.normalizeFunc = gauss;
			input.weight = param->modalityWeight(i);
			inputChannels->push_back(input);
		}
		QSharedPointer<iAModality const> mod0 = m_modalities->Get(0);
		iAExtendedRandomWalker* extendedRandomWalker = new iAExtendedRandomWalker(
			mod0->GetWidth(), mod0->GetHeight(), mod0->GetDepth(), mod0->GetSpacing(),
			inputChannels,
			svm.GetResult(),
			param->erw_gamma(),
			param->erw_maxIter()
		);
		m_erwPerfTimer.start();
		m_runningERW.insert(extendedRandomWalker, param);
		connect(extendedRandomWalker, SIGNAL(finished()), this, SLOT(erwFinished()) );
		
		m_mutex.lock();
		m_runningOperations++;
		m_mutex.unlock();
		extendedRandomWalker->start();
		// use threadpool:
		//QThreadPool::globalInstance()->start(extendedRandomWalker);
	}
	if (m_aborted)
	{
		StatusMsg("Aborted by user!");
		return;
	}
	// wait for running operations to finish:
	while (m_runningOperations > 0)
	{
		msleep(100);
	}
}

void iAMMSegSampler::erwFinished()
{
	QObject* o = QObject::sender();
	iAExtendedRandomWalker* extendedRandomWalker = dynamic_cast<iAExtendedRandomWalker*>(o);
	if (!extendedRandomWalker)
	{
		DEBUG_LOG("FATAL: erwFinished - NULL or non-ERW-object sender!\n");
		m_aborted = true;
		return;
	}
	iAPerformanceTimer::DurationType time = m_erwPerfTimer.elapsed();
	m_calcERWDuration += time;
	if (extendedRandomWalker->GetError() != "")
	{
		DEBUG_LOG(QString("ERW Error: %1\n")
			.arg(extendedRandomWalker->GetError()));
		m_aborted = true;

		// we don't start characteristics calculation (at which's end we would do this otherwise):
		m_mutex.lock();
		m_runningOperations--;
		m_mutex.unlock();
		return;
	}
	QSharedPointer<iAMMSegParameter> param = m_runningERW[extendedRandomWalker];

	QSharedPointer<iARWResult> rwResult(extendedRandomWalker->GetResult());

	QString resultDir = m_outputBaseDir + "/sample" + QString::number(param->id());
	QSharedPointer<iASingleResult> result(new iASingleResult(param->id(), resultDir/*, time.count()*/));

	double value = -1;
	// set attributes from param:
	for (int i = 0; i < NonModalityParamCount; ++i)
	{
		switch (i)
		{
			case erwBeta: value = param->erw_beta(); break;
			case erwGamma: value = param->erw_gamma(); break;
			case erwMaxIter: value = param->erw_maxIter(); break;
			case svmC: value = param->svm_c(); break;
			case svmGamma: value = param->svm_gamma(); break;
			case svmChannelCount: value = param->svm_channels(); break;
		}
		result->SetAttribute(i, value);
	}
	int modalityCount = m_range->modalityParamRange.size();
	for (int m = 0; m < modalityCount; ++m)
	{
		for (int i = 0; i < ModalityParamCount; ++i)
		{
			switch (i)
			{
			case pca: value = param->pcaDim(m); break;
			case weight: value = param->modalityWeight(m); break;
			case distance: value = param->distanceFuncIdx(m); break;
			}
			int idx = NonModalityParamCount + (m*ModalityParamCount) + i;
			result->SetAttribute(idx, value);
		}
	}

	result->SetLabelImage(rwResult->labelledImage);
	result->AddProbabilityImages(rwResult->probabilityImages);
	result->SetAttribute(m_derivedOutputStart+1, time);
	m_results->GetAttributes()->Get(m_derivedOutputStart + 1)->AdjustMinMax(time);
	//m_range->AdaptDurationMinMax(time);

	CharacteristicsCalculator * newCharCalc = new CharacteristicsCalculator (result, m_results->GetAttributes(), m_derivedOutputStart);
	m_runningCharCalc.insert(newCharCalc, result);
	connect(newCharCalc, SIGNAL(finished()), this, SLOT(charactCalcFinished()) );
	m_ChaPerfTimer.start();
	newCharCalc->start();

	m_runningERW.remove(extendedRandomWalker);
	delete extendedRandomWalker;

}

#include "iAITKIO.h"
#include "iAToolsITK.h"

#include <QDir>

bool WriteResult(QSharedPointer<iASingleResult> singleResult, int labelCount, bool storeProbabilities)
{
	QString folder(singleResult->GetFolder());
	assert(singleResult);
	QDir dir(folder);
		
	if (dir.exists())
	{
		DEBUG_LOG(QString("ERROR: output directory '%1' already exists!\n").arg(folder));
		singleResult->DiscardDetails();
		return false;
	}
	dir.mkpath(".");
	QString labelFileName(folder + "\\label.mhd");
	itk::ImageIOBase::IOComponentType pixelType = GetITKScalarPixelType(singleResult->GetLabelledImage());
	iAITKIO::writeFile(labelFileName, singleResult->GetLabelledImage(), pixelType, true);
		
	for (int i = 0; storeProbabilities && (i<labelCount); ++i)
	{
		QString probFileName(folder + "\\prob" + QString::number(i) + ".mhd");
		itk::ImageIOBase::IOComponentType pixelType = GetITKScalarPixelType(singleResult->GetProbabilityImg(i));
		iAITKIO::writeFile(probFileName, singleResult->GetProbabilityImg(i), pixelType, true);
	}
	return true;
}


void iAMMSegSampler::charactCalcFinished()
{
	QObject* o = QObject::sender();
	CharacteristicsCalculator* charactCalc = dynamic_cast<CharacteristicsCalculator*>(o);
	if (!charactCalc)
	{
		DEBUG_LOG("ERROR: charactCalcFinished - NULL or on-CharacteristicsCalculator sender!\n");
		return;
	}
	QSharedPointer<iASingleResult> result = m_runningCharCalc[charactCalc];
	m_results->GetAttributes()->Get(m_derivedOutputStart)->AdjustMinMax(result->GetAttribute(m_derivedOutputStart));
	double time = m_ChaPerfTimer.elapsed();
	m_calcChaDuration += time;
	
	// TODO: pass in from somewhere! Or don't store here at all? but what in case of a power outage/error?
	QString sampleMetaFile      = m_outputBaseDir + "/" + m_parameterRangeFile;
	QString parameterSetFile    = m_outputBaseDir + "/" + m_parameterSetFile;
	QString characteristicsFile = m_outputBaseDir + "/" + m_characteristicsFile;
	if (WriteResult(result, m_seeds->size(), m_storeProbabilities))
	{
		result->DiscardDetails();
		m_results->AddResult(result);
		emit Progress((100*m_results->size()) / m_parameters->size());
		if (!m_results->Store(sampleMetaFile, parameterSetFile, characteristicsFile))
		{
			DEBUG_LOG("Error writing parameter file.\n");
		}
	}
	m_runningCharCalc.remove(charactCalc);
	delete charactCalc;
	m_mutex.lock();
	m_runningOperations--;
	m_mutex.unlock();
}

double iAMMSegSampler::elapsed() const
{
	return m_calcERWDuration+m_calcChaDuration;
}

double iAMMSegSampler::estimatedTimeRemaining() const
{
	return 
		((m_calcERWDuration+m_calcChaDuration)/(m_curLoop+1)) // average duration of one cycle
		* static_cast<double>(m_parameters->size()-m_curLoop-1) // remaining cycles
	;
}

QSharedPointer<iASamplingResults> iAMMSegSampler::GetResults()
{
	return m_results;
}

void iAMMSegSampler::Abort()
{
	m_aborted = true;
}

bool iAMMSegSampler::IsAborted()
{
	return m_aborted;
}

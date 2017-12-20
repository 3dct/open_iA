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
#include "iAPatchFilter.h"

#include "defines.h"    // for DIM
#include "iAAttributeDescriptor.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAFilterRegistry.h"
#include "iAProgress.h"
#include "iAStringHelper.h"
#include "iAToolsITK.h"
#include "iATypedCallHelper.h"
#include "io/iAITKIO.h"

#include <itkImage.h>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace
{
	int getRequiredParts(int size, int partSize)
	{
		return size / partSize + ((size % partSize == 0) ? 0 : 1);
	}

	template <typename T>
	void patch_template(QMap<QString, QVariant> const & parameters, QSharedPointer<iAFilter> filter,
		QVector<iAConnector*> & con, iAProgress* progress, iALogger* log)
	{
		typedef itk::Image<T, DIM> InputImageType;
		typedef itk::Image<double, DIM> OutputImageType;
		auto size = dynamic_cast<InputImageType*>(con[0]->GetITKImage())->GetLargestPossibleRegion().GetSize();
		auto inputSpacing = dynamic_cast<InputImageType*>(con[0]->GetITKImage())->GetSpacing();
	
		QStringList filterParamStrs = SplitPossiblyQuotedString(parameters["Parameters"].toString());
		if (filter->Parameters().size() != filterParamStrs.size())
		{
			DEBUG_LOG(QString("PatchFilter: Invalid number of parameters: %1 expected, %2 given!")
				.arg(filter->Parameters().size())
				.arg(filterParamStrs.size()));
			return;
		}
		QMap<QString, QVariant> filterParams;
		for (int i = 0; i<filterParamStrs.size(); ++i)
			filterParams.insert(filter->Parameters()[i]->Name(), filterParamStrs[i]);
		
		QVector<iAConnector*> inputImages;
		inputImages.push_back(new iAConnector);
		inputImages[0]->SetImage(con[0]->GetITKImage());
		QVector<iAConnector*> smallImageInput;
		smallImageInput.push_back(new iAConnector);
		// TODO: read from con array?
		QStringList additionalInput = SplitPossiblyQuotedString(parameters["Additional input"].toString());
		for (QString fileName : additionalInput)
		{
			//fileName = MakeAbsolute(batchDir, fileName);
			auto newCon = new iAConnector();
			iAITKIO::ScalarPixelType pixelType;
			iAITKIO::ImagePointer img = iAITKIO::readFile(fileName, pixelType, false);
			newCon->SetImage(img);
			inputImages.push_back(newCon);
			smallImageInput.push_back(new iAConnector);
		}

		iAProgress dummyProgress;

		QStringList outputBuffer;

		QSharedPointer<iAFilter> extractImageFilter = iAFilterRegistry::Filter("Extract Image");

		int curOp = 0;
		QMap<QString, QVariant> extractParams;
		int patchSize[3] = {
			parameters["Patch size X"].toInt(),
			parameters["Patch size Y"].toInt(),
			parameters["Patch size Z"].toInt()
		};
		int stepSize[3] = {
			parameters["Step size X"].toInt(),
			parameters["Step size Y"].toInt(),
			parameters["Step size Z"].toInt()
		};
		int blockCount[DIM];
		double outputSpacing[DIM];
		for (int i = 0; i < DIM; ++i)
		{
			blockCount[i] = getRequiredParts(size[i], stepSize[i]);
			outputSpacing[i] = inputSpacing[i] * stepSize[i];
		}
		int totalOps = blockCount[0] * blockCount[1] * blockCount[2];
		QVector<iAConnector*> extractImageInput;
		extractImageInput.push_back(new iAConnector);
		bool warnOutputNotSupported = false;
		bool center = parameters["Center patch"].toBool();
		bool doImage = parameters["Write output value image"].toBool();
		QVector<iAITKIO::ImagePointer> outputImages;
		QStringList outputNames;
		// iterate over all patches:
		for (int x = 0; x < size[0]; x += stepSize[0])
		{
			extractParams.insert("Index X", center ? std::max(0, x-patchSize[0]/2) : x);
			extractParams.insert("Size X", (x < (size[0] - patchSize[0])) ? patchSize[0] - (center ?
				(std::abs(x-patchSize[0]/2) + (x-patchSize[0]/2)) / 2 : 0)	: size[0]-x);
			for (int y = 0; y < size[1]; y += stepSize[1])
			{
				extractParams.insert("Index Y", center ? std::max(0, y - patchSize[1]/2) : y);
				extractParams.insert("Size Y", (y < (size[1] - patchSize[1])) ? patchSize[1] - (center ?
					(std::abs(y-patchSize[1]/2) + (y-patchSize[1]/2)) / 2 : 0) : size[1] - y);
				for (int z = 0; z < size[2]; z += stepSize[2])
				{
					extractParams.insert("Index Z", center ? std::max(0, z - patchSize[2]/2) : z);
					extractParams.insert("Size Z", (z < (size[2] - patchSize[2])) ? patchSize[2] - (center ?
						(std::abs(z-patchSize[2]/2) + (z-patchSize[2]/2)) / 2 : 0) : size[2] - z);
					
					DEBUG_LOG(QString("Working on patch: upper left=(%1, %2, %3), dim=(%4, %5, %6).")
						.arg(extractParams["Index X"].toUInt())
						.arg(extractParams["Index Y"].toUInt())
						.arg(extractParams["Index Z"].toUInt())
						.arg(extractParams["Size X"].toUInt())
						.arg(extractParams["Size Y"].toUInt())
						.arg(extractParams["Size Z"].toUInt()));
					// apparently some ITK filters (e.g. statistics) have problems with images
					// with a size of 1 in one dimension, so let's skip such patches for the moment...
					if (extractParams["Size X"].toUInt() <= 1 ||
						extractParams["Size Y"].toUInt() <= 1 ||
						extractParams["Size Z"].toUInt() <= 1)
						DEBUG_LOG("    skipping because one side <= 1.");
						continue;
					try
					{
						// extract patch from all inputs:
						for (int i = 0; i < inputImages.size(); ++i)
						{
							extractImageInput[0]->SetImage(inputImages[i]->GetITKImage());
							extractImageFilter->SetUp(extractImageInput, log, &dummyProgress);
							extractImageFilter->Run(extractParams);
							smallImageInput[i]->SetImage(extractImageInput[0]->GetITKImage());
						}

						// run filter on inputs:
						filter->SetUp(smallImageInput, log, &dummyProgress);
						filter->Run(filterParams);

						// get output images and values from filter:
						if (filter->OutputCount() > 0)
							warnOutputNotSupported = true;

						if (filter->OutputValues().size() > 0)
						{
							if (curOp == 0)
							{
								QStringList captions;
								captions << "x" << "y" << "z";
								for (auto outValue : filter->OutputValues())
								{
									captions << outValue.first;
									outputNames << outValue.first;
								}
								outputBuffer.append(captions.join(","));
							}
							QStringList values;
							values << QString::number(x) << QString::number(y) << QString::number(z);
							for (auto outValue : filter->OutputValues())
								values.append(outValue.second.toString());
							outputBuffer.append(values.join(","));

							if (doImage)
							{
								while (outputImages.size() < filter->OutputValues().size())
									outputImages.push_back(AllocateImage(blockCount, outputSpacing, itk::ImageIOBase::DOUBLE));

								itk::Index<DIM> idx;
								idx[0] = x / stepSize[0]; idx[1] = y / stepSize[1]; idx[2] = z / stepSize[2];
								for (int i=0; i<filter->OutputValues().size(); ++i)
									(dynamic_cast<OutputImageType*>(outputImages[i].GetPointer()))->SetPixel(idx, filter->OutputValues()[i].second.toDouble());
							}
						}
					}
					catch (std::exception& e)
					{
						if (parameters["Continue on error"].toBool())
						{
							DEBUG_LOG(QString("Patch filter: An error has occurred: %1, continueing anyway.").arg(e.what()));
						}
						else
							throw e;
					}
					
					progress->ManualProgress(static_cast<int>(100.0 * curOp / totalOps));
					++curOp;
				}
			}
		}
		if (warnOutputNotSupported)
			DEBUG_LOG("Creating output images from each patch not yet supported!");

		QString outputFile = parameters["Output csv file"].toString();
		QFile file(outputFile);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream textStream(&file);
			for (QString line : outputBuffer)
			{
				textStream << line << endl;
			}
			file.close();
		}
		else
			DEBUG_LOG(QString("Output file not specified, or could not be opened (%1)").arg(outputFile));
		for (int i = 0; i < outputImages.size(); ++i)
		{
			QFileInfo fi(parameters["Output image base name"].toString());
			QString outFileName = QString("%1/%2%3%4.%5")
				.arg(fi.absolutePath())
				.arg(fi.baseName())
				.arg(outputNames[i])
				.arg(fi.completeSuffix());
			StoreImage(outputImages[i], outFileName, parameters["Compress image"].toBool());
			DEBUG_LOG(QString("Storing output for '%1' in file '%2'").arg(outputNames[i]).arg(outFileName));
		}
	}
}

iAPatchFilter::iAPatchFilter():
	iAFilter("Patch Filter", "Image Ensembles",
		"Create patches from an input image and apply a filter each patch.<br/>", 1, 0)
{
	AddParameter("Patch size X", Discrete, 1, 1);
	AddParameter("Patch size Y", Discrete, 1, 1);
	AddParameter("Patch size Z", Discrete, 1, 1);
	AddParameter("Step size X", Discrete, 1, 1);
	AddParameter("Step size Y", Discrete, 1, 1);
	AddParameter("Step size Z", Discrete, 1, 1);
	AddParameter("Center patch", Boolean, true);
	AddParameter("Filter", String, "Image Quality");
	AddParameter("Parameters", String, "");
	AddParameter("Additional input", String, "");
	AddParameter("Output csv file", String, "");
	AddParameter("Write output value image", Boolean, true);
	AddParameter("Output image base name", String, "output.mhd");
	AddParameter("Compress image", Boolean, true);
	AddParameter("Continue on error", Boolean, true);
}

void iAPatchFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	auto filter = iAFilterRegistry::Filter(parameters["Filter"].toString());
	if (!filter)
	{
		AddMsg(QString("Patch: Cannot run filter '%1', it does not exist!").arg(parameters["Filter"].toString()));
		return;
	}
	ITK_TYPED_CALL(patch_template, m_con->GetITKScalarPixelType(), parameters, filter, m_cons, m_progress, m_log);
}

IAFILTER_CREATE(iAPatchFilter);

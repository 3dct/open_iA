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
#include "iATypedCallHelper.h"
#include "io/iAITKIO.h"

#include <itkImage.h>

#include <QFile>
#include <QTextStream>

namespace
{
	int getRequiredParts(int size, int partSize)
	{
		return size / partSize + ((size % partSize == 0) ? 0 : 1);
	}

	QString DimName[DIM] = { "X", "Y", "Z" };

	template <typename T>
	void patch_template(QMap<QString, QVariant> const & parameters, QSharedPointer<iAFilter> filter,
		QVector<iAConnector*> & con, iAProgress* progress, iALogger* log)
	{
		typedef itk::Image<T, DIM> InputImageType;
		auto size = dynamic_cast<InputImageType*>(con[0]->GetITKImage())->GetLargestPossibleRegion().GetSize();
		int blockCount[DIM];
		for (int i=0; i<DIM; ++i)
			blockCount[i] = getRequiredParts(size[i], parameters[QString("Patch size %1").arg(DimName[i])].toUInt());
	
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

		int totalOps = blockCount[0] * blockCount[1] * blockCount[2];
		int curOp = 0;
		QMap<QString, QVariant> extractParams;
		unsigned int xPatchSize = parameters["Patch size X"].toUInt(),
			yPatchSize = parameters["Patch size Y"].toUInt(),
			zPatchSize = parameters["Patch size Z"].toUInt();
		QVector<iAConnector*> extractImageInput;
		extractImageInput.push_back(new iAConnector);
		bool warnOutputNotSupported = false;

		// iterate over all patches:
		for (int xBlock = 0; xBlock < blockCount[0]; ++xBlock)
		{
			extractParams.insert("Index X", xBlock * xPatchSize);
			extractParams.insert("Size X", (xBlock < blockCount[0] - 1) || (size[0] % xPatchSize == 0)
				? xPatchSize : size[0] % xPatchSize);
			for (int yBlock = 0; yBlock < blockCount[1]; ++yBlock)
			{
				extractParams.insert("Index Y", yBlock * yPatchSize);
				extractParams.insert("Size Y", (yBlock < blockCount[1] - 1) || (size[1] % yPatchSize == 0)
					? yPatchSize : size[1] % yPatchSize);
				for (int zBlock = 0; zBlock < blockCount[2]; ++zBlock)
				{
					extractParams.insert("Index Z", zBlock * zPatchSize);
					extractParams.insert("Size Z", (zBlock < blockCount[2] - 1) || (size[2] % zPatchSize == 0)
						? zPatchSize : size[2] % zPatchSize);
					// apparently some ITK filters (e.g. statistics) have problems with images
					// with a size of 1 in one dimension, so let's skip such patches for the moment...
					if (extractParams["Size X"].toUInt() <= 1 ||
							extractParams["Size Y"].toUInt() <= 1 ||
							extractParams["Size Z"].toUInt() <= 1)
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
								captions << "Patch x" << "Patch y" << "Patch z";
								for (auto outValue : filter->OutputValues())
									captions << outValue.first;
								outputBuffer.append(captions.join(","));
							}
							QStringList values;
							values << QString::number(xBlock*xPatchSize)
								<< QString::number(yBlock*yPatchSize)
								<< QString::number(zBlock*zPatchSize);
							for (auto outValue : filter->OutputValues())
								values.append(outValue.second.toString());
							outputBuffer.append(values.join(","));
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
	}
}

iAPatchFilter::iAPatchFilter():
	iAFilter("Patch Filter", "Image Ensembles",
		"Create patches from an input image and apply a filter each patch.<br/>", 1, 0)
{
	AddParameter("Patch size X", Discrete, 1, 1);
	AddParameter("Patch size Y", Discrete, 1, 1);
	AddParameter("Patch size Z", Discrete, 1, 1);
	AddParameter("Filter", String, "Image Quality");
	AddParameter("Parameters", String, "");
	AddParameter("Additional input", String, "");
	AddParameter("Output csv file", String, "");
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

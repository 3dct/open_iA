// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAParameterNames.h"

#include <defines.h>    // for DIM
#include <iAAttributeDescriptor.h>
#include <iAConnector.h>
#include <iAFileTypeRegistry.h>
#include <iAFilterDefault.h>
#include <iAFilterRegistry.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iAProgress.h>
#include <iAStringHelper.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>
#include <iAITKIO.h>

#include <itkImage.h>

#include <vtkImageData.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

IAFILTER_DEFAULT_CLASS(iAPatchFilter)

namespace
{
	size_t getRequiredParts(size_t size, size_t partSize)
	{
		return size / partSize + ((size % partSize == 0) ? 0 : 1);
	}

	size_t getLeft(size_t x, size_t patchSizeHalf, bool center)
	{
		return center ? std::max(static_cast<size_t>(0), x - patchSizeHalf) : x;
	}

	int getSize(size_t x, size_t left, size_t size, size_t patchSizeHalf, size_t patchSize, bool center)
	{
		return center ? (
			(x < patchSizeHalf) ?
			patchSize - (patchSizeHalf - (x - left)) :
			(
			(left >= size - patchSize) ?
				size - left :
				patchSize
				)
			)
			: (x < size - patchSize) ? patchSize : size - x;
	}

	template <typename T>
	void patch(iAPatchFilter* patchFilter, QVariantMap const & parameters)
	{
		auto filter = iAFilterRegistry::filter(parameters[spnFilter].toString());
		if (!filter)
		{
			LOG(lvlError, QString("Patch: Cannot run filter '%1', it does not exist!").arg(parameters[spnFilter].toString()));
			return;
		}
		typedef itk::Image<T, DIM> InputImageType;
		typedef itk::Image<double, DIM> OutputImageType;
		auto itkImg = dynamic_cast<InputImageType*>(patchFilter->imageInput(0)->itkImage());
		auto size = itkImg->GetLargestPossibleRegion().GetSize();
		//LOG(lvlInfo, QString("Size: (%1, %2, %3)").arg(size[0]).arg(size[1]).arg(size[2]));
		auto inputSpacing = itkImg->GetSpacing();

		QStringList filterParamStrs = splitPossiblyQuotedString(parameters["Parameters"].toString());
		if (filter->parameters().size() != filterParamStrs.size())
		{
			LOG(lvlError, QString("PatchFilter: Invalid number of parameters: %1 expected, %2 given!")
				.arg(filter->parameters().size())
				.arg(filterParamStrs.size()));
			return;
		}
		QVariantMap filterParams;
		for (int i = 0; i < filterParamStrs.size(); ++i)
		{
			filterParams.insert(filter->parameters()[i]->name(), filterParamStrs[i]);
		}

		std::vector<std::shared_ptr<iADataSet>> inputImages;
		inputImages.push_back(patchFilter->input(0));
		QStringList additionalInput = splitPossiblyQuotedString(parameters["Additional input"].toString());
		for (QString fileName : additionalInput)
		{
			auto io = iAFileTypeRegistry::createIO(fileName, iAFileIO::Load);
			QVariantMap dummyParams;    // TODO: CHECK whether I/O requires other parameters and error in that case!
			auto dataSet = io->load(fileName, dummyParams);
			if (dataSet->type() == iADataSetType::Collection)
			{
				auto c = dynamic_cast<iADataCollection*>(dataSet.get());
				for (auto d: c->dataSets())
				{
					inputImages.push_back(d);
				}
			}
			else
			{
				inputImages.push_back(dataSet);
			}
		}
		QStringList outputBuffer;

		int curOp = 0;
		size_t patchSize[3] = {
			parameters["Patch size X"].toULongLong(),
			parameters["Patch size Y"].toULongLong(),
			parameters["Patch size Z"].toULongLong()
		};
		size_t patchSizeHalf[3];
		size_t stepSize[3] = {
			parameters["Step size X"].toULongLong(),
			parameters["Step size Y"].toULongLong(),
			parameters["Step size Z"].toULongLong()
		};
		int blockCount[DIM];
		double outputSpacing[DIM];
		for (int i = 0; i < DIM; ++i)
		{
			blockCount[i] = getRequiredParts(size[i], stepSize[i]);
			outputSpacing[i] = inputSpacing[i] * stepSize[i];
			patchSizeHalf[i] = patchSize[i] / 2;
		}
		int totalOps = blockCount[0] * blockCount[1] * blockCount[2];
		bool center = parameters["Center patch"].toBool();
		bool doImage = parameters["Write output value image"].toBool();
		bool compress = parameters[spnCompressOutput].toBool();
		bool overwrite = parameters[spnOverwriteOutput].toBool();
		bool continueOnError = parameters[spnContinueOnError].toBool();
		QVector<iAITKIO::ImagePointer> outputImages;
		QStringList outputNames;
		if (doImage)
		{
			while (outputImages.size() < filter->outputValueNames().size())
			{
				outputImages.push_back(allocateImage(blockCount, outputSpacing, iAITKIO::ScalarType::DOUBLE));
				outputNames << filter->outputValueNames()[outputImages.size() - 1];
			}
		}
		QFileInfo fi(parameters["Output image base name"].toString());
		QDir outDir(fi.absolutePath());
		if (!outDir.exists())
		{
			if (!outDir.mkdir("."))
			{
				throw std::runtime_error(QString("Output directory %1 does not exist and cannot create it .")
					.arg(fi.absolutePath()).toStdString());
			}
		}
		filter->setLogger(patchFilter->logger());
		// iterate over all patches:
		itk::Index<DIM> outIdx; outIdx[0] = 0;
		for (size_t x = 0; x < size[0] && !patchFilter->isAborted(); x += stepSize[0])
		{
			outIdx[1] = 0;
			size_t extractIndex[3], extractSize[3];
			extractIndex[0] = getLeft(x, patchSizeHalf[0], center);
			extractSize[0] = getSize(x, extractIndex[0], size[0], patchSizeHalf[0], patchSize[0], center);
			for (size_t y = 0; y < size[1] && !patchFilter->isAborted(); y += stepSize[1])
			{
				outIdx[2] = 0;
				extractIndex[1] = getLeft(y, patchSizeHalf[1], center);
				extractSize[1] = getSize(y, extractIndex[1], size[1], patchSizeHalf[1], patchSize[1], center);
				for (size_t z = 0; z < size[2] && !patchFilter->isAborted(); z += stepSize[2])
				{
					extractIndex[2] = getLeft(z, patchSizeHalf[2], center);
					extractSize[2] = getSize(z, extractIndex[2], size[2], patchSizeHalf[2], patchSize[2], center);
					/*
					LOG(lvlInfo, QString("Working on patch: upper left=(%1, %2, %3), dim=(%4, %5, %6), outIdx=(%10,%11,%12).")
						.arg(extractParams["Index X"].toUInt())
						.arg(extractParams["Index Y"].toUInt())
						.arg(extractParams["Index Z"].toUInt())
						.arg(extractParams["Size X"].toUInt())
						.arg(extractParams["Size Y"].toUInt())
						.arg(extractParams["Size Z"].toUInt())
						.arg(outIdx[0]).arg(outIdx[1]).arg(outIdx[2]));
					*/
					// apparently some ITK filters (e.g. statistics) have problems with images
					// with a size of 1 in one dimension, so let's skip such patches for the moment...
					if (extractSize[0] <= 1 || extractSize[1] <= 1 || extractSize[2] <= 1)
					{
						//LOG(lvlInfo, "    skipping because one side <= 1.");
						continue;
					}
					try
					{
						// extract patch from all inputs and add to filter input:
						filter->clearInput();
						for (size_t i = 0; i < inputImages.size(); ++i)
						{
							auto itkExtractImg = extractImage(dynamic_cast<iAImageData*>(inputImages[i].get())->itkImage(), extractIndex, extractSize);
							// maybe modify original filename to reflect that only a patch of it is passed on?
							filter->addInput(std::make_shared<iAImageData>(itkExtractImg));
						}
						// run filter on inputs:
						filter->run(filterParams);

						// get output images and values from filter:
						for (size_t o = 0; o < filter->finalOutputCount(); ++o)
						{
							QString outFileName = QString("%1/%2-patch%3%4.%5")
								.arg(fi.absolutePath())
								.arg(fi.baseName())
								.arg(curOp)
								.arg(filter->finalOutputCount() == 1 ? "" : "-"+filter->outputName(o))
								.arg(fi.completeSuffix());
							if (QFile::exists(outFileName))
							{
								LOG(lvlWarn, QString("Output file %1 already exists; if you want to overwrite it, "
									"you need to set the '%2' parameter to true.")
									.arg(outFileName).arg(spnOverwriteOutput));
								if (!continueOnError)
								{
									throw std::runtime_error(QString("Aborting patch filter since an output file already existed, "
										"and '%1' and '%2' are disabled.")
										.arg(spnOverwriteOutput)
										.arg(spnContinueOnError).toStdString());
								}
							}
							storeImage(filter->imageOutput(o)->itkImage(), outFileName, compress);
						}
						if (filter->outputValues().size() > 0)
						{
							if (curOp == 0)
							{
								QStringList captions;
								captions << "x" << "y" << "z";
								for (auto outValue : filter->outputValues())
								{
									captions << outValue.first;
								}
								outputBuffer.append(captions.join(","));
							}
							QStringList values;
							values << QString::number(x) << QString::number(y) << QString::number(z);
							for (auto outValue : filter->outputValues())
							{
								values.append(outValue.second.toString());
							}
							outputBuffer.append(values.join(","));
							if (doImage)
							{
								for (int i = 0; i < filter->outputValues().size(); ++i)
								{
									(dynamic_cast<OutputImageType*>(outputImages[i].GetPointer()))->SetPixel(outIdx, filter->outputValues()[i].second.toDouble());
								}
							}
						}
					}
					catch (std::exception& e)
					{
						if (continueOnError)
						{
							LOG(lvlError, QString("Patch filter: An error has occurred: %1, continueing anyway.").arg(e.what()));
						}
						else
						{
							throw;
						}
					}

					patchFilter->progress()->emitProgress(curOp * 100.0 / totalOps);
					++curOp;
					++outIdx[2];
				}
				++outIdx[1];
			}
			++outIdx[0];
		}
		if (patchFilter->isAborted())
		{
			throw std::runtime_error("Aborted by user!");
		}
		QString outputFile = parameters["Output csv file"].toString();
		if (!outputFile.isEmpty())
		{
			QFile file(outputFile);
			if (file.exists() && !overwrite)
			{
				LOG(lvlError, QString("Output file %1 already exists; if you want to overwrite it, "
					"you need to set the '%2' parameter to true.")
					.arg(outputFile).arg(spnOverwriteOutput));
			}
			else if (file.open(QIODevice::WriteOnly | QIODevice::Text))
			{
				QTextStream textStream(&file);
				for (QString line : outputBuffer)
				{
					textStream << line << Qt::endl;
				}
				file.close();
			}
			else
			{
				LOG(lvlError, QString("Output csv file could not be opened (%1)").arg(outputFile));
			}
		}
		for (int i = 0; i < outputImages.size(); ++i)
		{
			QString outFileName = QString("%1/%2%3.%4")
				.arg(fi.absolutePath())
				.arg(fi.baseName())
				.arg(outputNames[i])
				.arg(fi.completeSuffix());
			storeImage(outputImages[i], outFileName, compress);
			//LOG(lvlInfo, QString("Storing output for '%1' in file '%2'").arg(outputNames[i]).arg(outFileName));
		}
	}
}

iAPatchFilter::iAPatchFilter():
	iAFilter("Patch Filter", "Image Ensembles",
		QString(
		"Create patches from an input image and apply a filter each patch.<br/>"
		"If you just want to extract image blocks from a larger image without applying a filter, "
		"you can choose the 'Copy' operation as <em>%1</em> parameter. "
		"<em>%2</em> determines whether output images are compressed (.mhd + .zraw) or uncompressed (.mhd + .raw). "
		"When <em>%3</em> is enabled, then batch processing will continue with the next file "
		"in case there is an error. If it is disabled, an error will interrupt the whole batch run. ")
		.arg(spnFilter)
		.arg(spnCompressOutput)
		.arg(spnContinueOnError)
		, 1, 0, true)
{
	addParameter("Patch size X", iAValueType::Discrete, 1, 1);
	addParameter("Patch size Y", iAValueType::Discrete, 1, 1);
	addParameter("Patch size Z", iAValueType::Discrete, 1, 1);
	addParameter("Step size X", iAValueType::Discrete, 1, 1);
	addParameter("Step size Y", iAValueType::Discrete, 1, 1);
	addParameter("Step size Z", iAValueType::Discrete, 1, 1);
	addParameter("Center patch", iAValueType::Boolean, true);
	addParameter(spnFilter, iAValueType::FilterName, "Image Quality");
	addParameter("Parameters", iAValueType::FilterParameters, "");
	addParameter("Additional input", iAValueType::FileNamesOpen, "");
	addParameter("Output csv file", iAValueType::FileNameSave, ".csv");
	addParameter("Write output value image", iAValueType::Boolean, true);
	addParameter("Output image base name", iAValueType::String, "output.mhd");
	addParameter(spnCompressOutput, iAValueType::Boolean, true);
	addParameter(spnContinueOnError, iAValueType::Boolean, false);
	addParameter(spnOverwriteOutput, iAValueType::Boolean, false);
}

void iAPatchFilter::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(patch, inputScalarType(), this, parameters);
}

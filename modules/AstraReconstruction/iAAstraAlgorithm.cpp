// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAstraAlgorithm.h"

#include <iAImageData.h>
#include <iALog.h>
#include <iAToolsVTK.h>
#include <iATypedCallHelper.h>
#include <iAVec3.h>

#include <astra/CudaBackProjectionAlgorithm3D.h>
#include <astra/CudaFDKAlgorithm3D.h>
#include <astra/CudaCglsAlgorithm3D.h>
#include <astra/CudaSirtAlgorithm3D.h>
#include <astra/CudaForwardProjectionAlgorithm3D.h>
#include <astra/CudaProjector3D.h>

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <QtMath>  // for qDegreesToRadians

// names of all parameters (to avoid ambiguous strings)
const QString AstraParameters::ProjGeometry("Projection Geometry");
const QString AstraParameters::DetSpcX("Detector Spacing X");
const QString AstraParameters::DetSpcY("Detector Spacing Y");
const QString AstraParameters::ProjAngleStart("Projection Angle Start");
const QString AstraParameters::ProjAngleEnd("Projection Angle End");
const QString AstraParameters::ProjAngleCnt("Projection Angle Count");
const QString AstraParameters::DetRowCnt("Detector Row Count");
const QString AstraParameters::DetColCnt("Detector Column Count");
const QString AstraParameters::ProjAngleDim("Projection Angle Dimension");
const QString AstraParameters::DetRowDim("Detector Row Dimension");
const QString AstraParameters::DetColDim("Detector Column Dimension");
const QString AstraParameters::DstOrigDet("Distance Origin-Detector");
const QString AstraParameters::DstOrigSrc("Distance Origin-Source");
const QString AstraParameters::CenterOfRotCorr("Center of Rotation Correction");
const QString AstraParameters::CenterOfRotOfs("Center of Rotation Offset");
const QString AstraParameters::InitWithFDK("Initialize with FDK");
const QString AstraParameters::VolDimX("Volume Dimension X");
const QString AstraParameters::VolDimY("Volume Dimension Y");
const QString AstraParameters::VolDimZ("Volume Dimension Z");
const QString AstraParameters::VolSpcX("Volume Spacing X");
const QString AstraParameters::VolSpcY("Volume Spacing Y");
const QString AstraParameters::VolSpcZ("Volume Spacing Z");
const QString AstraParameters::AlgoType("Algorithm Type");
const QString AstraParameters::NumberOfIterations("Number of Iterations");

namespace
{
	QString linspace(double projAngleStart, double projAngleEnd, int projAnglesCount)
	{
		QString result;
		for (int i = 0; i <= projAnglesCount - 2; i++)
		{
			double temp = projAngleStart + i*(projAngleEnd - projAngleStart) / (std::floor(static_cast<double>(projAnglesCount)) - 1);
			result.append(QString::number(temp) + ",");
		}
		result.append(QString::number(projAngleEnd));
		return result;
	}

	void createConeProjGeom(astra::Config & projectorConfig, QVariantMap const & parameters, size_t detRowCnt, size_t detColCnt, size_t projAngleCnt)
	{
		astra::XMLNode projGeomNode = projectorConfig.self.addChildNode("ProjectionGeometry");
		projGeomNode.addAttribute("type", "cone");
		projGeomNode.addChildNode("DetectorSpacingX", parameters[AstraParameters::DetSpcX].toDouble());
		projGeomNode.addChildNode("DetectorSpacingY", parameters[AstraParameters::DetSpcY].toDouble());
		projGeomNode.addChildNode("DetectorRowCount", detRowCnt);
		projGeomNode.addChildNode("DetectorColCount", detColCnt);
		projGeomNode.addChildNode("ProjectionAngles", linspace(
			qDegreesToRadians(parameters[AstraParameters::ProjAngleStart].toDouble()),
			qDegreesToRadians(parameters[AstraParameters::ProjAngleEnd].toDouble()),
			projAngleCnt).toStdString());
		projGeomNode.addChildNode("DistanceOriginDetector", parameters[AstraParameters::DstOrigDet].toDouble());
		projGeomNode.addChildNode("DistanceOriginSource",   parameters[AstraParameters::DstOrigSrc].toDouble());
	}


	void createConeVecProjGeom(astra::Config & projectorConfig, QVariantMap const & parameters, size_t detRowCnt, size_t detColCnt, size_t projAngleCnt)
	{
		QString vectors;
		for (size_t i = 0; i<projAngleCnt; ++i)
		{
			double curAngle = qDegreesToRadians(parameters[AstraParameters::ProjAngleStart].toDouble()) +
				i*(qDegreesToRadians(parameters[AstraParameters::ProjAngleEnd].toDouble())
					- qDegreesToRadians(parameters[AstraParameters::ProjAngleStart].toDouble())) /
				(projAngleCnt - 1);
			iAVec3f sourcePos(
				std::sin(curAngle) * parameters[AstraParameters::DstOrigSrc].toDouble(),
				-std::cos(curAngle) * parameters[AstraParameters::DstOrigSrc].toDouble(),
				0);
			iAVec3f detectorCenter(
				-std::sin(curAngle) * parameters[AstraParameters::DstOrigDet].toDouble(),
				std::cos(curAngle) * parameters[AstraParameters::DstOrigDet].toDouble(),
				0);
			iAVec3f detectorPixelHorizVec(				// vector from detector pixel(0, 0) to(0, 1)
				std::cos(curAngle) * parameters[AstraParameters::DetSpcX].toDouble(),
				std::sin(curAngle) * parameters[AstraParameters::DetSpcX].toDouble(),
				0);
			iAVec3f detectorPixelVertVec(0, 0, parameters[AstraParameters::DetSpcY].toDouble()); // vector from detector pixel(0, 0) to(1, 0)
			iAVec3f shiftVec = detectorPixelHorizVec.normalized() * parameters[AstraParameters::CenterOfRotOfs].toDouble();
			sourcePos += shiftVec;
			detectorCenter += shiftVec;

			if (!vectors.isEmpty())
			{
				vectors += ",";
			}
			vectors += QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12")
				.arg(sourcePos.x()).arg(sourcePos.y()).arg(sourcePos.z())
				.arg(detectorCenter.x()).arg(detectorCenter.y()).arg(detectorCenter.z())
				.arg(detectorPixelHorizVec.x()).arg(detectorPixelHorizVec.y()).arg(detectorPixelHorizVec.z())
				.arg(detectorPixelVertVec.x()).arg(detectorPixelVertVec.y()).arg(detectorPixelVertVec.z());
		}
		astra::XMLNode projGeomNode = projectorConfig.self.addChildNode("ProjectionGeometry");
		projGeomNode.addAttribute("type", "cone_vec");
		projGeomNode.addChildNode("DetectorRowCount", detRowCnt);
		projGeomNode.addChildNode("DetectorColCount", detColCnt);
		projGeomNode.addChildNode("Vectors", vectors.toStdString());
	}

	void fillVolumeGeometryNode(astra::XMLNode & volGeomNode, int const volDim[3], double const volSpacing[3])
	{
		volGeomNode.addChildNode("GridColCount",   volDim[1]);      // columns are "y-direction" (second index component in buffer) in astra
		volGeomNode.addChildNode("GridRowCount",   volDim[0]);      // rows are "x-direction" (first index component in buffer) in astra
		volGeomNode.addChildNode("GridSliceCount", volDim[2]);

		astra::XMLNode winMinXOption = volGeomNode.addChildNode("Option");
		winMinXOption.addAttribute("key", "WindowMinX");
		winMinXOption.addAttribute("value", -volDim[1] * volSpacing[1] / 2.0);
		astra::XMLNode winMaxXOption = volGeomNode.addChildNode("Option");
		winMaxXOption.addAttribute("key", "WindowMaxX");
		winMaxXOption.addAttribute("value", volDim[1] * volSpacing[1] / 2.0);
		astra::XMLNode winMinYOption = volGeomNode.addChildNode("Option");
		winMinYOption.addAttribute("key", "WindowMinY");
		winMinYOption.addAttribute("value", -volDim[0] * volSpacing[0] / 2.0);
		astra::XMLNode winMaxYOption = volGeomNode.addChildNode("Option");
		winMaxYOption.addAttribute("key", "WindowMaxY");
		winMaxYOption.addAttribute("value", volDim[0] * volSpacing[0] / 2.0);
		astra::XMLNode winMinZOption = volGeomNode.addChildNode("Option");
		winMinZOption.addAttribute("key", "WindowMinZ");
		winMinZOption.addAttribute("value", -volDim[2] * volSpacing[2] / 2.0);
		astra::XMLNode winMaxZOption = volGeomNode.addChildNode("Option");
		winMaxZOption.addAttribute("key", "WindowMaxZ");
		winMaxZOption.addAttribute("value", volDim[2] * volSpacing[2] / 2.0);
	}


	template <typename T>
	void swapXYandCastToFloat(vtkSmartPointer<vtkImageData> img, astra::float32* buf)
	{
		T* imgBuf = static_cast<T*>(img->GetScalarPointer());
		int * dim = img->GetDimensions();
		size_t inIdx = 0;
		for (size_t z = 0; z < static_cast<size_t>(dim[2]); ++z)
		{
			for (int y = 0; y < dim[1]; ++y)
			{
#pragma omp parallel for
				for (int x = 0; x < dim[0]; ++x)
				{
					size_t outIdx = y + ((x + z * dim[0]) * dim[1]);
					buf[outIdx] = static_cast<float>(imgBuf[inIdx + x]);
				}
				inIdx += dim[0];
			}
		}
	}

	void addCommonForwardReconstructParams(iAFilter* filter)
	{
		QStringList projectionGeometries; projectionGeometries << "cone";
		filter->addParameter(AstraParameters::ProjGeometry, iAValueType::Categorical, projectionGeometries);
		filter->addParameter(AstraParameters::DetSpcX, iAValueType::Continuous, 1.0, 0.0);
		filter->addParameter(AstraParameters::DetSpcY, iAValueType::Continuous, 1.0, 0.0);
		filter->addParameter(AstraParameters::ProjAngleStart, iAValueType::Continuous, 0.0);
		filter->addParameter(AstraParameters::ProjAngleEnd, iAValueType::Continuous, 359.0);
		filter->addParameter(AstraParameters::DstOrigDet, iAValueType::Continuous, 1.0);
		filter->addParameter(AstraParameters::DstOrigSrc, iAValueType::Continuous, 1.0);
	}
}



QStringList AstraParameters::algorithmStrings()
{
	static QStringList algorithms;
	if (algorithms.empty())
	{
		algorithms << "BP" << "FDK" << "SIRT" << "CGLS";
	}
	return algorithms;
}

int AstraParameters::mapAlgoStringToIndex(QString const& algo)
{
	if (algorithmStrings().indexOf(algo) == -1)
	{
		LOG(lvlWarn, QString("Invalid algorithm string - "
			"%1 is not in the list of valid algorithms (%2)!")
			.arg(algo).arg(algorithmStrings().join(", ")));
		return FDK3D;
	}
	return algorithmStrings().indexOf(algo);
}

QString AstraParameters::mapAlgoIndexToString(int astraIndex)
{
	if (astraIndex < 0 || astraIndex >= algorithmStrings().size())
	{
		LOG(lvlWarn, QString("Invalid algorithm index - "
			"%1 is not in the valid range (0..%2)!")
			.arg(astraIndex).arg(algorithmStrings().size() - 1));
		return "Invalid";
	}
	return algorithmStrings()[astraIndex];
}



iAAstraForwardProject::iAAstraForwardProject() :
	iAFilter("ASTRA Forward Projection", "Reconstruction/ASTRA Toolbox",
		"Forward Projection with the ASTRA Toolbox")
{
	addCommonForwardReconstructParams(this);
	addParameter(AstraParameters::DetRowCnt, iAValueType::Discrete, 512);
	addParameter(AstraParameters::DetColCnt, iAValueType::Discrete, 512);
	addParameter(AstraParameters::ProjAngleCnt, iAValueType::Discrete, 360);
}

class CPPAstraCustomMemory: public astra::CFloat32CustomMemory
{
public:
	CPPAstraCustomMemory(size_t size)
	{
		m_fPtr = new astra::float32[size];
	}
	~CPPAstraCustomMemory()
	{
		delete [] m_fPtr;
	}
};

void iAAstraForwardProject::performWork(QVariantMap const & parameters)
{
	vtkSmartPointer<vtkImageData> volImg = imageInput(0)->vtkImage();
	int * volDim = volImg->GetDimensions();
	astra::Config projectorConfig;
	projectorConfig.initialize("Projector3D");
	astra::XMLNode gpuIndexOption = projectorConfig.self.addChildNode("Option");
	gpuIndexOption.addAttribute("key", "GPUIndex");
	gpuIndexOption.addAttribute("value", "0");
	/*
	// further (optional) "Option"s (as GPUIndex):
	"ProjectionKernel"
	"VoxelSuperSampling"
	"DetectorSuperSampling"
	"DensityWeighting"
	*/
	createConeProjGeom(projectorConfig, parameters, parameters[AstraParameters::DetRowCnt].toUInt(), parameters[AstraParameters::DetColCnt].toUInt(), parameters[AstraParameters::ProjAngleCnt].toUInt());

	astra::XMLNode volGeomNode = projectorConfig.self.addChildNode("VolumeGeometry");
	fillVolumeGeometryNode(volGeomNode, volDim, volImg->GetSpacing());

	CPPAstraCustomMemory * volumeBuf = new CPPAstraCustomMemory(static_cast<size_t>(volDim[0]) * volDim[1] * volDim[2]);
	VTK_TYPED_CALL(swapXYandCastToFloat, volImg->GetScalarType(), volImg, volumeBuf->m_fPtr);

	astra::CCudaProjector3D* projector = new astra::CCudaProjector3D();
	projector->initialize(projectorConfig);
	astra::CFloat32ProjectionData3DMemory * projectionData = new astra::CFloat32ProjectionData3DMemory(projector->getProjectionGeometry(), 0.0);
	astra::CFloat32VolumeData3DMemory * volumeData = new astra::CFloat32VolumeData3DMemory(projector->getVolumeGeometry(), volumeBuf);
	astra::CCudaForwardProjectionAlgorithm3D* algorithm = new astra::CCudaForwardProjectionAlgorithm3D();
	algorithm->initialize(projector, projectionData, volumeData);
	algorithm->run();

	int projDim[3] = {
		parameters[AstraParameters::DetColCnt].toInt(),
		parameters[AstraParameters::DetRowCnt].toInt(),
		parameters[AstraParameters::ProjAngleCnt].toInt() };
	double projSpacing[3] = {
		parameters[AstraParameters::DetSpcX].toDouble(),
		parameters[AstraParameters::DetSpcY].toDouble(),
		// "normalize" z spacing with projections count to make sinograms with different counts more easily comparable:
		parameters[AstraParameters::DetSpcX].toDouble() * 180 / parameters[AstraParameters::ProjAngleCnt].toDouble() };
	auto projImg = allocateImage(VTK_FLOAT, projDim, projSpacing);
	float* projImgBuf = static_cast<float*>(projImg->GetScalarPointer());
	astra::float32* projData = projectionData->getData();
	size_t imgIndex = 0;
	unsigned int projAngleCount = parameters[AstraParameters::ProjAngleCnt].toUInt();
	unsigned int detectorColCnt = parameters[AstraParameters::DetColCnt].toUInt();
	for (size_t z = 0; z < static_cast<size_t>(projDim[2]); ++z)
	{
		for (int y = 0; y < projDim[1]; ++y)
		{
			size_t startIdx = ((y * projDim[2]) + (projAngleCount - z - 1)) * projDim[0];
			astra::float32* row = &(projData[startIdx]);
#pragma omp parallel for
			for (int x = 0; x < projDim[0]; ++x)
			{
				projImgBuf[imgIndex + x] = row[detectorColCnt - x - 1];
			}
			imgIndex += projDim[0];
		}
	}
	addOutput(std::make_shared<iAImageData>(projImg));

	delete algorithm;
	delete volumeData;
	delete projectionData;
	delete projector;
}




iAAstraReconstruct::iAAstraReconstruct() :
	iAFilter("ASTRA Reconstruction", "Reconstruction/ASTRA Toolbox",
		"Reconstruction with the ASTRA Toolbox")
{
	addCommonForwardReconstructParams(this);
	addParameter(AstraParameters::DetRowDim, iAValueType::Discrete, 1, 0, 5);
	addParameter(AstraParameters::DetColDim, iAValueType::Discrete, 3, 0, 5);
	addParameter(AstraParameters::ProjAngleDim, iAValueType::Discrete, 5, 0, 5);

	addParameter(AstraParameters::VolDimX, iAValueType::Discrete, 512, 1);
	addParameter(AstraParameters::VolDimY, iAValueType::Discrete, 512, 1);
	addParameter(AstraParameters::VolDimZ, iAValueType::Discrete, 512, 1);
	addParameter(AstraParameters::VolSpcX, iAValueType::Continuous, 1.0);
	addParameter(AstraParameters::VolSpcY, iAValueType::Continuous, 1.0);
	addParameter(AstraParameters::VolSpcZ, iAValueType::Continuous, 1.0);

	addParameter(AstraParameters::AlgoType, iAValueType::Categorical, AstraParameters::algorithmStrings());

	addParameter(AstraParameters::NumberOfIterations, iAValueType::Discrete, 100, 0);
	addParameter(AstraParameters::CenterOfRotCorr, iAValueType::Boolean, false);
	addParameter(AstraParameters::CenterOfRotOfs, iAValueType::Continuous, 0.0);
	addParameter(AstraParameters::InitWithFDK, iAValueType::Boolean, true);
}


template <typename T>
void swapDimensions(vtkSmartPointer<vtkImageData> img, astra::float32* buf, int detColDim, int detRowDim, int projAngleDim)
{
	T* imgBuf = static_cast<T*>(img->GetScalarPointer());
	int * dim = img->GetDimensions();
	int detColDimIdx = detColDim % 3;		// only do modulus once before loop
	int detRowDimIdx = detRowDim % 3;
	int projAngleDimIdx = projAngleDim % 3;
	int idx[3];
	size_t imgBufIdx = 0;
	for (idx[2] = 0; idx[2] < dim[2]; ++idx[2])
	{
		for (idx[1] = 0; idx[1] < dim[1]; ++idx[1])
		{
#pragma omp parallel for
			for (int x = 0; x < dim[0]; ++x)
			{
				idx[0] = x;
				size_t detCol    = idx[detColDimIdx];     if (detColDim >= 3)    { detCol    = dim[detColDimIdx]    - detCol    - 1; }
				size_t detRow    = idx[detRowDimIdx];     if (detRowDim >= 3)    { detRow    = dim[detRowDimIdx]    - detRow    - 1; }
				size_t projAngle = idx[projAngleDimIdx];  if (projAngleDim >= 3) { projAngle = dim[projAngleDimIdx] - projAngle - 1; }
				size_t bufIndex = detCol + ((projAngle + detRow*dim[projAngleDimIdx])*dim[detColDimIdx]);
				buf[bufIndex] = static_cast<float>(imgBuf[imgBufIdx + idx[0]]);
			}
			imgBufIdx += dim[0];
		}
	}
}


void iAAstraReconstruct::performWork(QVariantMap const & parameters)
{
	vtkSmartPointer<vtkImageData> projImg = imageInput(0)->vtkImage();
	int * projDim = projImg->GetDimensions();
	if (projDim[0] == 0 || projDim[1] == 0 || projDim[2] == 0)
	{
		LOG(lvlError, "File not fully loaded or invalid, at least one side is reported to have size 0.");
		return;
	}
	if (parameters[AstraParameters::DetRowDim].toUInt() % 3 == parameters[AstraParameters::DetColDim].toUInt() % 3 ||
		parameters[AstraParameters::DetRowDim].toUInt() % 3 == parameters[AstraParameters::ProjAngleDim].toUInt() % 3 ||
		parameters[AstraParameters::ProjAngleDim].toUInt() % 3 == parameters[AstraParameters::DetColDim].toUInt() % 3)
	{
		LOG(lvlError, "Invalid parameters: One dimension referenced multiple times!");
		return;
	}
	size_t detRowCnt = projDim[parameters[AstraParameters::DetRowDim].toUInt() % 3];
	size_t detColCnt = projDim[parameters[AstraParameters::DetColDim].toUInt() % 3];
	size_t projAngleCnt = projDim[parameters[AstraParameters::ProjAngleDim].toUInt() % 3];
	CPPAstraCustomMemory * projBuf = new CPPAstraCustomMemory(static_cast<size_t>(projDim[0]) * projDim[1] * projDim[2]);
	//VTK_TYPED_CALL(swapDimensions, img->GetScalarType(), img, buf, m_detColDim, m_detRowDim, m_projAngleDim, m_detRowCnt, m_detColCnt, m_projAnglesCount);
	VTK_TYPED_CALL(swapDimensions, projImg->GetScalarType(), projImg, projBuf->m_fPtr,
		parameters[AstraParameters::DetColDim].toUInt(),
		parameters[AstraParameters::DetRowDim].toUInt(),
		parameters[AstraParameters::ProjAngleDim].toUInt());

	// create XML configuration:
	astra::Config projectorConfig;
	projectorConfig.initialize("Projector3D");
	astra::XMLNode gpuIndexOption = projectorConfig.self.addChildNode("Option");
	gpuIndexOption.addAttribute("key", "GPUIndex");
	gpuIndexOption.addAttribute("value", "0");
	assert(parameters[AstraParameters::ProjGeometry].toString() == "cone");
	if (parameters[AstraParameters::CenterOfRotCorr].toBool())
	{
		createConeVecProjGeom(projectorConfig, parameters, detRowCnt, detColCnt, projAngleCnt);
	}
	else
	{
		createConeProjGeom(projectorConfig, parameters, detRowCnt, detColCnt, projAngleCnt);
	}
	astra::XMLNode volGeomNode = projectorConfig.self.addChildNode("VolumeGeometry");
	double volSpacing[3] = {
		parameters[AstraParameters::VolSpcX].toDouble(),
		parameters[AstraParameters::VolSpcY].toDouble(),
		parameters[AstraParameters::VolSpcZ].toDouble()
	};
	int volDim[3] = {
		parameters[AstraParameters::VolDimX].toInt(),
		parameters[AstraParameters::VolDimY].toInt(),
		parameters[AstraParameters::VolDimZ].toInt()
	};
	fillVolumeGeometryNode(volGeomNode, volDim, volSpacing);

	// create Algorithm and run:
	astra::CCudaProjector3D* projector = new astra::CCudaProjector3D();
	projector->initialize(projectorConfig);
	astra::CFloat32ProjectionData3DMemory * projectionData = new astra::CFloat32ProjectionData3DMemory(projector->getProjectionGeometry(), projBuf);
	astra::CFloat32VolumeData3DMemory * volumeData = new astra::CFloat32VolumeData3DMemory(projector->getVolumeGeometry(), 0.0f);
	int algo = AstraParameters::mapAlgoStringToIndex(parameters[AstraParameters::AlgoType].toString());
	if ((algo == SIRT3D || SIRT3D == CGLS3D) && parameters[AstraParameters::InitWithFDK].toBool())
	{
		astra::CCudaFDKAlgorithm3D* fdkalgo = new astra::CCudaFDKAlgorithm3D();
		fdkalgo->initialize(projector, projectionData, volumeData);
		fdkalgo->run();
		delete fdkalgo;
	}
	switch (algo)
	{
		case BP3D: {
			astra::CCudaBackProjectionAlgorithm3D* bp3dalgo = new astra::CCudaBackProjectionAlgorithm3D();
			bp3dalgo->initialize(projector, projectionData, volumeData);		// unfortunately, initialize is not virtual in CReconstructionAlgorithm3D, otherwise we could call it once after the switch...
			bp3dalgo->run();
			delete bp3dalgo;
			break;
		}
		case FDK3D: {
			astra::CCudaFDKAlgorithm3D* fdkalgo = new astra::CCudaFDKAlgorithm3D();
			fdkalgo->initialize(projector, projectionData, volumeData);
			fdkalgo->run();
			delete fdkalgo;
			break;
		}
		case SIRT3D: {
			astra::CCudaSirtAlgorithm3D* sirtalgo = new astra::CCudaSirtAlgorithm3D();
			sirtalgo->initialize(projector, projectionData, volumeData);
			sirtalgo->run(parameters[AstraParameters::NumberOfIterations].toInt());
			delete sirtalgo;
			break;
		}
		case CGLS3D: {
			astra::CCudaCglsAlgorithm3D* cglsalgo = new astra::CCudaCglsAlgorithm3D();
			cglsalgo->initialize(projector, projectionData, volumeData);
			cglsalgo->run(parameters[AstraParameters::NumberOfIterations].toInt());
			delete cglsalgo;
			break;
		}
		default:
			LOG(lvlError, "Unknown reconstruction algorithm selected!");
	}

	// retrieve result image:
	auto volImg = allocateImage(VTK_FLOAT, volDim, volSpacing);
	float* volImgBuf = static_cast<float*>(volImg->GetScalarPointer());
	size_t imgIndex = 0;
	size_t sliceOffset = static_cast<size_t>(volDim[1]) * volDim[0];
	astra::float32* slice = volumeData->getData();
	for (size_t z = 0; z < static_cast<size_t>(volDim[2]); ++z)
	{
		for (int y = 0; y < volDim[1]; ++y)
		{
#pragma omp parallel for
			for (long long x = 0; x < volDim[0]; ++x)
			{
				volImgBuf[imgIndex + x] = slice[x*volDim[1] + y];
			}
			imgIndex += volDim[0];
		}
		slice += sliceOffset;
	}
	addOutput(std::make_shared<iAImageData>(volImg));

	delete volumeData;
	delete projectionData;
	delete projector;
}

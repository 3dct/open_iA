/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAAstraAlgorithm.h"

#include "dlg_ProjectionParameters.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAPerformanceHelper.h"
#include "iAToolsVTK.h"
#include "iATypedCallHelper.h"
#include "iAvec3.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <astra/CudaBackProjectionAlgorithm3D.h>
#include <astra/CudaFDKAlgorithm3D.h>
#include <astra/CudaCglsAlgorithm3D.h>
#include <astra/CudaSirtAlgorithm3D.h>
#include <astra/CudaForwardProjectionAlgorithm3D.h>
#include <astra/CudaProjector3D.h>
#include <astra/Logging.h>

#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

#include <QMessageBox>
#include <QtMath>  // for qDegreesToRadians

#include <cuda_runtime_api.h>

namespace
{
	// names of all parameters (to avoid ambiguous strings)
	QString ProjGeometry = "Projection Geometry";
	QString DetSpcX = "Detector Spacing X";
	QString DetSpcY = "Detector Spacing Y";
	QString ProjAngleStart = "Projection Angle Start";
	QString ProjAngleEnd = "Projection Angle End";
	QString ProjAngleCnt = "Projection Angle Count";
	QString DetRowCnt = "Detector Row Count";
	QString DetColCnt = "Detector Column Count";
	QString ProjAngleDim = "Projection Angle Dimension";
	QString DetRowDim = "Detector Row Dimension";
	QString DetColDim = "Detector Column Dimension";
	QString DstOrigDet = "Distance Origin-Detector";
	QString DstOrigSrc = "Distance Origin-Source";
	QString CenterOfRotCorr = "Center of Rotation Correction";
	QString CenterOfRotOfs = "Center of Rotation Offset";
	QString InitWithFDK = "Initialize with FDK";
	QString VolDimX = "Volume Dimension X";
	QString VolDimY = "Volume Dimension Y";
	QString VolDimZ = "Volume Dimension Z";
	QString VolSpcX = "Volume Spacing X";
	QString VolSpcY = "Volume Spacing Y";
	QString VolSpcZ = "Volume Spacing Z";
	QString AlgoType = "Algorithm Type";
	QString NumberOfIterations = "Number of Iterations";

	QString linspace(double projAngleStart, double projAngleEnd, int projAnglesCount)
	{
		QString result;
		for (int i = 0; i <= projAnglesCount - 2; i++)
		{
			double temp = projAngleStart + i*(projAngleEnd - projAngleStart) / (floor((double)projAnglesCount) - 1);
			result.append(QString::number(temp) + ",");
		}
		result.append(QString::number(projAngleEnd));
		return result;
	}

	void createConeProjGeom(astra::Config & projectorConfig, QMap<QString, QVariant> const & parameters, size_t detRowCnt, size_t detColCnt, size_t projAngleCnt)
	{
		astra::XMLNode projGeomNode = projectorConfig.self.addChildNode("ProjectionGeometry");
		projGeomNode.addAttribute("type", "cone");
		projGeomNode.addChildNode("DetectorSpacingX", parameters[DetSpcX].toDouble());
		projGeomNode.addChildNode("DetectorSpacingY", parameters[DetSpcY].toDouble());
		projGeomNode.addChildNode("DetectorRowCount", detRowCnt);
		projGeomNode.addChildNode("DetectorColCount", detColCnt);
		projGeomNode.addChildNode("ProjectionAngles", linspace(
			qDegreesToRadians(parameters[ProjAngleStart].toDouble()),
			qDegreesToRadians(parameters[ProjAngleEnd].toDouble()),
			projAngleCnt).toStdString());
		projGeomNode.addChildNode("DistanceOriginDetector", parameters[DstOrigDet].toDouble());
		projGeomNode.addChildNode("DistanceOriginSource",   parameters[DstOrigSrc].toDouble());
	}


	void createConeVecProjGeom(astra::Config & projectorConfig, QMap<QString, QVariant> const & parameters, size_t detRowCnt, size_t detColCnt, size_t projAngleCnt)
	{
		QString vectors;
		for (size_t i = 0; i<projAngleCnt; ++i)
		{
			double curAngle = qDegreesToRadians(parameters[ProjAngleStart].toDouble()) +
				i*(qDegreesToRadians(parameters[ProjAngleEnd].toDouble())
					- qDegreesToRadians(parameters[ProjAngleStart].toDouble())) /
				(projAngleCnt - 1);
			iAVec3f sourcePos(
				sin(curAngle) * parameters[DstOrigSrc].toDouble(),
				-cos(curAngle) * parameters[DstOrigSrc].toDouble(),
				0);
			iAVec3f detectorCenter(
				-sin(curAngle) * parameters[DstOrigDet].toDouble(),
				cos(curAngle) * parameters[DstOrigDet].toDouble(),
				0);
			iAVec3f detectorPixelHorizVec(				// vector from detector pixel(0, 0) to(0, 1)
				cos(curAngle) * parameters[DetSpcX].toDouble(),
				sin(curAngle) * parameters[DetSpcX].toDouble(),
				0);
			iAVec3f detectorPixelVertVec(0, 0, parameters[DetSpcY].toDouble()); // vector from detector pixel(0, 0) to(1, 0)
			iAVec3f shiftVec = detectorPixelHorizVec.normalized() * parameters[CenterOfRotOfs].toDouble();
			sourcePos += shiftVec;
			detectorCenter += shiftVec;

			if (!vectors.isEmpty()) vectors += ",";
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


	bool isCUDAAvailable()
	{
		int deviceCount = 0;
		cudaGetDeviceCount(&deviceCount);
		if (deviceCount == 0)
			return false;
		// TODO: Allow choosing which device(s) to use!
		else
		{
			/*
			size_t mostMem = 0;	int idx = -1;
			for (int dev = 0; dev < deviceCount; dev++)
			{
				cudaDeviceProp deviceProp;
				cudaGetDeviceProperties(&deviceProp, dev);
				DEBUG_LOG(QString("%1. Compute Capability: %2.%3. Clock Rate (kHz): %5. Memory Clock Rate (kHz): %6. Memory Bus Width (bits): %7. Concurrent kernels: %8. Total memory: %9.")
					.arg(deviceProp.name)
					.arg(deviceProp.major)
					.arg(deviceProp.minor)
					.arg(deviceProp.clockRate)
					.arg(deviceProp.memoryClockRate)
					.arg(deviceProp.memoryBusWidth)
					.arg(deviceProp.concurrentKernels)
					.arg(deviceProp.totalGlobalMem)
				);
				if (deviceProp.totalGlobalMem > mostMem)
				{
					mostMem = deviceProp.totalGlobalMem;
					idx = dev;
				}
			}
			astra::SGPUParams gpuParams;
			gpuParams.GPUIndices.push_back(idx);
			gpuParams.memory = mostMem ;
			astra::CCompositeGeometryManager::setGlobalGPUParams(gpuParams);
			*/
		}
		return true;
	}

	QStringList algorithmStrings()
	{
		static QStringList algorithms;
		if (algorithms.empty())
			algorithms << "BP" << "FDK" << "SIRT" << "CGLS";
		return algorithms;
	}

	int mapAlgoStringToIndex(QString const & algo)
	{
		if (algorithmStrings().indexOf(algo) == -1)
		{
			DEBUG_LOG("Invalid Algorithm Type selection!");
			return FDK3D;
		}
		return algorithmStrings().indexOf(algo);
	}

	QString mapAlgoIndexToString(int astraIndex)
	{
		if (astraIndex < 0 || astraIndex >= algorithmStrings().size())
		{
			DEBUG_LOG("Invalid Algorithm Type selection!");
			return "Invalid";
		}
		return algorithmStrings()[astraIndex];
	}

	void addCommonForwardReconstructParams(iAFilter* filter)
	{
		QStringList projectionGeometries; projectionGeometries << "cone";
		filter->addParameter(ProjGeometry, Categorical, projectionGeometries);
		filter->addParameter(DetSpcX, Continuous, 1.0, 0.0);
		filter->addParameter(DetSpcY, Continuous, 1.0, 0.0);
		filter->addParameter(ProjAngleStart, Continuous, 0.0);
		filter->addParameter(ProjAngleEnd, Continuous, 359.0);
		filter->addParameter(DstOrigDet, Continuous, 1.0);
		filter->addParameter(DstOrigSrc, Continuous, 1.0);
	}
}


IAFILTER_CREATE(iAASTRAForwardProject)


iAASTRAForwardProject::iAASTRAForwardProject() :
	iAFilter("ASTRA Forward Projection", "Reconstruction/ASTRA Toolbox",
		"Forward Projection with the ASTRA Toolbox")
{
	addCommonForwardReconstructParams(this);
	addParameter(DetRowCnt, Discrete, 512);
	addParameter(DetColCnt, Discrete, 512);
	addParameter(ProjAngleCnt, Discrete, 360);
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

void iAASTRAForwardProject::performWork(QMap<QString, QVariant> const & parameters)
{
	vtkSmartPointer<vtkImageData> volImg = input()[0]->vtkImage();
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
	createConeProjGeom(projectorConfig, parameters, parameters[DetRowCnt].toUInt(), parameters[DetColCnt].toUInt(), parameters[ProjAngleCnt].toUInt());

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
		parameters[DetColCnt].toInt(),
		parameters[DetRowCnt].toInt(),
		parameters[ProjAngleCnt].toInt() };
	double projSpacing[3] = {
		parameters[DetSpcX].toDouble(),
		parameters[DetSpcY].toDouble(),
		// "normalize" z spacing with projections count to make sinograms with different counts more easily comparable:
		parameters[DetSpcX].toDouble() * 180 / parameters[ProjAngleCnt].toDouble() };
	auto projImg = allocateImage(VTK_FLOAT, projDim, projSpacing);
	float* projImgBuf = static_cast<float*>(projImg->GetScalarPointer());
	astra::float32* projData = projectionData->getData();
	size_t imgIndex = 0;
	unsigned int projAngleCount = parameters[ProjAngleCnt].toUInt();
	unsigned int detectorColCnt = parameters[DetColCnt].toUInt();
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
	addOutput(projImg);

	delete algorithm;
	delete volumeData;
	delete projectionData;
	delete projector;
}



IAFILTER_CREATE(iAASTRAReconstruct)


iAASTRAReconstruct::iAASTRAReconstruct() :
	iAFilter("ASTRA Reconstruction", "Reconstruction/ASTRA Toolbox",
		"Reconstruction with the ASTRA Toolbox")
{
	addCommonForwardReconstructParams(this);
	addParameter(DetRowDim, Discrete, 1, 0, 5);
	addParameter(DetColDim, Discrete, 3, 0, 5);
	addParameter(ProjAngleDim, Discrete, 5, 0, 5);

	addParameter(VolDimX, Discrete, 512, 1);
	addParameter(VolDimY, Discrete, 512, 1);
	addParameter(VolDimZ, Discrete, 512, 1);
	addParameter(VolSpcX, Continuous, 1.0);
	addParameter(VolSpcY, Continuous, 1.0);
	addParameter(VolSpcZ, Continuous, 1.0);

	addParameter(AlgoType, Categorical, algorithmStrings());

	addParameter(NumberOfIterations, Discrete, 100, 0);
	addParameter(CenterOfRotCorr, Boolean, false);
	addParameter(CenterOfRotOfs, Continuous, 0.0);
	addParameter(InitWithFDK, Boolean, true);
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


void iAASTRAReconstruct::performWork(QMap<QString, QVariant> const & parameters)
{
	vtkSmartPointer<vtkImageData> projImg = input()[0]->vtkImage();
	int * projDim = projImg->GetDimensions();
	if (projDim[0] == 0 || projDim[1] == 0 || projDim[2] == 0)
	{
		DEBUG_LOG("File not fully loaded or invalid, at least one side is reported to have size 0.");
		return;
	}
	if (parameters[DetRowDim].toUInt() % 3 == parameters[DetColDim].toUInt() % 3 ||
		parameters[DetRowDim].toUInt() % 3 == parameters[ProjAngleDim].toUInt() % 3 ||
		parameters[ProjAngleDim].toUInt() % 3 == parameters[DetColDim].toUInt() % 3)
	{
		DEBUG_LOG("Invalid parameters: One dimension referenced multiple times!");
		return;
	}
	size_t detRowCnt = projDim[parameters[DetRowDim].toUInt() % 3];
	size_t detColCnt = projDim[parameters[DetColDim].toUInt() % 3];
	size_t projAngleCnt = projDim[parameters[ProjAngleDim].toUInt() % 3];
	CPPAstraCustomMemory * projBuf = new CPPAstraCustomMemory(static_cast<size_t>(projDim[0]) * projDim[1] * projDim[2]);
	//VTK_TYPED_CALL(swapDimensions, img->GetScalarType(), img, buf, m_detColDim, m_detRowDim, m_projAngleDim, m_detRowCnt, m_detColCnt, m_projAnglesCount);
	VTK_TYPED_CALL(swapDimensions, projImg->GetScalarType(), projImg, projBuf->m_fPtr,
		parameters[DetColDim].toUInt(),
		parameters[DetRowDim].toUInt(),
		parameters[ProjAngleDim].toUInt());

	// create XML configuration:
	astra::Config projectorConfig;
	projectorConfig.initialize("Projector3D");
	astra::XMLNode gpuIndexOption = projectorConfig.self.addChildNode("Option");
	gpuIndexOption.addAttribute("key", "GPUIndex");
	gpuIndexOption.addAttribute("value", "0");
	assert(parameters[ProjGeometry].toString() == "cone");
	if (parameters[CenterOfRotCorr].toBool())
	{
		createConeVecProjGeom(projectorConfig, parameters, detRowCnt, detColCnt, projAngleCnt);
	}
	else
	{
		createConeProjGeom(projectorConfig, parameters, detRowCnt, detColCnt, projAngleCnt);
	}
	astra::XMLNode volGeomNode = projectorConfig.self.addChildNode("VolumeGeometry");
	double volSpacing[3] = {
		parameters[VolSpcX].toDouble(),
		parameters[VolSpcY].toDouble(),
		parameters[VolSpcZ].toDouble()
	};
	int volDim[3] = {
		parameters[VolDimX].toInt(),
		parameters[VolDimY].toInt(),
		parameters[VolDimZ].toInt()
	};
	fillVolumeGeometryNode(volGeomNode, volDim, volSpacing);

	// create Algorithm and run:
	astra::CCudaProjector3D* projector = new astra::CCudaProjector3D();
	projector->initialize(projectorConfig);
	astra::CFloat32ProjectionData3DMemory * projectionData = new astra::CFloat32ProjectionData3DMemory(projector->getProjectionGeometry(), projBuf);
	astra::CFloat32VolumeData3DMemory * volumeData = new astra::CFloat32VolumeData3DMemory(projector->getVolumeGeometry(), 0.0f);
	int algo = mapAlgoStringToIndex(parameters[AlgoType].toString());
	if ((algo == SIRT3D || SIRT3D == CGLS3D) && parameters[InitWithFDK].toBool())
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
			sirtalgo->run(parameters[NumberOfIterations].toInt());
			delete sirtalgo;
			break;
		}
		case CGLS3D: {
			astra::CCudaCglsAlgorithm3D* cglsalgo = new astra::CCudaCglsAlgorithm3D();
			cglsalgo->initialize(projector, projectionData, volumeData);
			cglsalgo->run(parameters[NumberOfIterations].toInt());
			delete cglsalgo;
			break;
		}
		default:
			DEBUG_LOG("Unknown reconstruction algorithm selected!");
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
	addOutput(volImg);

	delete volumeData;
	delete projectionData;
	delete projector;
}


IAFILTER_RUNNER_CREATE(iAASTRAFilterRunner);

namespace
{
	void  astraLogCallback(const char *msg, size_t len)
	{
		char * allMsg = new char[len + 1];
		std::memcpy(allMsg, msg, len);
		allMsg[len] = 0;
		QString qtStr(allMsg);
		DEBUG_LOG(qtStr.trimmed());
	}
}

void iAASTRAFilterRunner::run(QSharedPointer<iAFilter> filter, MainWindow* mainWnd)
{
	if (!isCUDAAvailable())
	{
		QMessageBox::warning(mainWnd, "ASTRA",
			"ASTRA toolbox operations require a CUDA-capable device, but no CUDA device was found."
			"In case this machine has an NVidia card, please install the latest driver!");
		return;
	}
	astra::CLogger::setOutputScreen(1, astra::LOG_INFO);
	bool success = astra::CLogger::setCallbackScreen(astraLogCallback);
	if (!success)
		DEBUG_LOG("Setting Astra log callback failed!");
	iAFilterRunnerGUI::run(filter, mainWnd);
}

bool iAASTRAFilterRunner::askForParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> & parameters,
	MdiChild* sourceMdi, MainWindow* mainWnd, bool /*askForAdditionalInput*/)
{
	dlg_ProjectionParameters dlg;
	dlg.setWindowTitle(filter->name());
	int const * inputDim = sourceMdi->imageData()->GetDimensions();
	if (filter->name() == "ASTRA Forward Projection")
	{
		dlg.fillProjectionGeometryValues(
			parameters[ProjGeometry].toString(),
			parameters[DetSpcX].toDouble(),
			parameters[DetSpcY].toDouble(),
			parameters[DetRowCnt].toUInt(),
			parameters[DetColCnt].toUInt(),
			parameters[ProjAngleStart].toDouble(),
			parameters[ProjAngleEnd].toDouble(),
			parameters[ProjAngleCnt].toUInt(),
			parameters[DstOrigDet].toDouble(),
			parameters[DstOrigSrc].toDouble());
	}
	else
	{
		dlg.fillProjectionGeometryValues(
			parameters[ProjGeometry].toString(),
			parameters[DetSpcX].toDouble(),
			parameters[DetSpcY].toDouble(),
			parameters[ProjAngleStart].toDouble(),
			parameters[ProjAngleEnd].toDouble(),
			parameters[DstOrigDet].toDouble(),
			parameters[DstOrigSrc].toDouble());
		int volDim[3] = {
			parameters[VolDimX].toInt(),
			parameters[VolDimY].toInt(),
			parameters[VolDimZ].toInt()
		};
		double volSpacing[3] = {
			parameters[VolSpcX].toDouble(),
			parameters[VolSpcY].toDouble(),
			parameters[VolSpcZ].toDouble()
		};
		dlg.fillVolumeGeometryValues(volDim, volSpacing);
		dlg.fillProjInputMapping(parameters[DetRowDim].toInt(),
			parameters[DetColDim].toInt(),
			parameters[ProjAngleDim].toInt(),
			inputDim);
		if (parameters[AlgoType].toString().isEmpty())
			parameters[AlgoType] = algorithmStrings()[1];
		dlg.fillAlgorithmValues(mapAlgoStringToIndex(parameters[AlgoType].toString()),
			parameters[NumberOfIterations].toUInt(),
			parameters[InitWithFDK].toBool());
		dlg.fillCorrectionValues(parameters[CenterOfRotCorr].toBool(),
			parameters[CenterOfRotOfs].toDouble());
	}
	if (dlg.exec() != QDialog::Accepted)
		return false;

	parameters[ProjGeometry] = dlg.ProjGeomType->currentText();
	parameters[DetSpcX] = dlg.ProjGeomDetectorSpacingX->value();
	parameters[DetSpcY] = dlg.ProjGeomDetectorSpacingY->value();
	parameters[ProjAngleStart] = dlg.ProjGeomProjAngleStart->value();
	parameters[ProjAngleEnd] = dlg.ProjGeomProjAngleEnd->value();
	parameters[DstOrigDet] = dlg.ProjGeomDistOriginDetector->value();
	parameters[DstOrigSrc] = dlg.ProjGeomDistOriginSource->value();
	if (filter->name() == "ASTRA Forward Projection")
	{
		parameters[DetRowCnt] = dlg.ProjGeomDetectorPixelsY->value();
		parameters[DetColCnt] = dlg.ProjGeomDetectorPixelsX->value();
		parameters[ProjAngleCnt] = dlg.ProjGeomProjCount->value();
	}
	else    // Reconstruction:
	{
		int detRowDim = dlg.ProjInputDetectorRowDim->currentIndex();
		int detColDim = dlg.ProjInputDetectorColDim->currentIndex();
		int projAngleDim = dlg.ProjInputProjAngleDim->currentIndex();
		parameters[VolDimX] = dlg.VolGeomDimensionX->value();
		parameters[VolDimY] = dlg.VolGeomDimensionY->value();
		parameters[VolDimZ] = dlg.VolGeomDimensionZ->value();
		parameters[VolSpcX] = dlg.VolGeomSpacingX->value();
		parameters[VolSpcY] = dlg.VolGeomSpacingY->value();
		parameters[VolSpcZ] = dlg.VolGeomSpacingZ->value();
		parameters[AlgoType] = mapAlgoIndexToString(dlg.AlgorithmType->currentIndex());
		parameters[NumberOfIterations] = dlg.AlgorithmIterations->value();
		parameters[InitWithFDK] = dlg.InitWithFDK->isChecked();
		parameters[CenterOfRotCorr] = dlg.CorrectionCenterOfRotation->isChecked();
		parameters[CenterOfRotOfs] = dlg.CorrectionCenterOfRotationOffset->value();
		if ((detColDim % 3) == (detRowDim % 3) || (detColDim % 3) == (projAngleDim % 3) || (detRowDim % 3) == (projAngleDim % 3))
		{
			QMessageBox::warning(mainWnd, "ASTRA", "One of the axes (x, y, z) has been specified for more than one usage out of "
				"(detector row / detector column / projection angle) dimensions. "
				"Make sure each axis is used exactly for one dimension!");
			return false;
		}
		parameters[DetRowDim] = detRowDim;
		parameters[DetColDim] = detColDim;
		parameters[ProjAngleDim] = projAngleDim;
	}
	return true;
}

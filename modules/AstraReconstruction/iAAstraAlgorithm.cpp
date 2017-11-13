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

#define ASTRA_CUDA
//#include <astra/CompositeGeometryManager.h>
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

	void CreateConeProjGeom(astra::Config & projectorConfig, QMap<QString, QVariant> const & parameters)
	{
		astra::XMLNode projGeomNode = projectorConfig.self.addChildNode("ProjectionGeometry");
		projGeomNode.addAttribute("type", "cone");
		projGeomNode.addChildNode("DetectorSpacingX", parameters[DetSpcX].toDouble());
		projGeomNode.addChildNode("DetectorSpacingY", parameters[DetSpcY].toDouble());
		projGeomNode.addChildNode("DetectorRowCount", parameters[DetRowCnt].toDouble());
		projGeomNode.addChildNode("DetectorColCount", parameters[DetColCnt].toDouble());
		projGeomNode.addChildNode("ProjectionAngles", linspace(
			qDegreesToRadians(parameters[ProjAngleStart].toDouble()),
			qDegreesToRadians(parameters[ProjAngleEnd].toDouble()),
			parameters[ProjAngleCnt].toUInt()).toStdString());
		projGeomNode.addChildNode("DistanceOriginDetector", parameters[DstOrigDet].toDouble());
		projGeomNode.addChildNode("DistanceOriginSource",   parameters[DstOrigSrc].toDouble());
	}


	void CreateConeVecProjGeom(astra::Config & projectorConfig, QMap<QString, QVariant> const & parameters)
	{
		QString vectors;
		for (int i = 0; i<parameters[ProjAngleCnt].toUInt(); ++i)
		{
			double curAngle = qDegreesToRadians(parameters[ProjAngleStart].toDouble()) +
				i*(qDegreesToRadians(parameters[ProjAngleEnd].toDouble())
					- qDegreesToRadians(parameters[ProjAngleStart].toDouble())) /
				(parameters[ProjAngleCnt].toUInt() - 1);
			iAVec3 sourcePos(
				sin(curAngle) * parameters[DstOrigSrc].toDouble(),
				-cos(curAngle) * parameters[DstOrigSrc].toDouble(),
				0);
			iAVec3 detectorCenter(
				-sin(curAngle) * parameters[DstOrigDet].toDouble(),
				cos(curAngle) * parameters[DstOrigDet].toDouble(),
				0);
			iAVec3 detectorPixelHorizVec(				// vector from detector pixel(0, 0) to(0, 1)
				cos(curAngle) * parameters[DetSpcX].toDouble(),
				sin(curAngle) * parameters[DetSpcX].toDouble(),
				0);
			iAVec3 detectorPixelVertVec(0, 0, parameters[DetSpcY].toDouble()); // vector from detector pixel(0, 0) to(1, 0)
			iAVec3 shiftVec = detectorPixelHorizVec.normalize() * parameters[CenterOfRotOfs].toDouble();
			sourcePos += shiftVec;
			detectorCenter += shiftVec;

			if (!vectors.isEmpty()) vectors += ",";
			vectors += QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12")
				.arg(sourcePos.x).arg(sourcePos.y).arg(sourcePos.z)
				.arg(detectorCenter.x).arg(detectorCenter.y).arg(detectorCenter.z)
				.arg(detectorPixelHorizVec.x).arg(detectorPixelHorizVec.y).arg(detectorPixelHorizVec.z)
				.arg(detectorPixelVertVec.x).arg(detectorPixelVertVec.y).arg(detectorPixelVertVec.z);
		}
		astra::XMLNode projGeomNode = projectorConfig.self.addChildNode("ProjectionGeometry");
		projGeomNode.addAttribute("type", "cone_vec");
		projGeomNode.addChildNode("DetectorRowCount", parameters[DetRowCnt].toDouble());
		projGeomNode.addChildNode("DetectorColCount", parameters[DetColCnt].toDouble());
		projGeomNode.addChildNode("Vectors", vectors.toStdString());
	}

	void FillVolumeGeometryNode(astra::XMLNode & volGeomNode, int const volDim[3], double const volSpacing[3])
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
	void SwapXYandCastToFloat(vtkSmartPointer<vtkImageData> img, astra::float32* buf)
	{
		T* imgBuf = static_cast<T*>(img->GetScalarPointer());
		int * dim = img->GetDimensions();
		size_t inIdx = 0;
		for (int z = 0; z < dim[2]; ++z)
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


	bool IsCUDAAvailable()
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

	QStringList AlgorithmStrings()
	{
		static QStringList algorithms;
		if (algorithms.empty())
			algorithms << "BP" << "FDK" << "SIRT" << "CGLS";
		return algorithms;
	}

	int MapAlgoStringToIndex(QString const & algo)
	{
		if (AlgorithmStrings().indexOf(algo) == -1)
		{
			DEBUG_LOG("Invalid Algorithm Type selection!");
			return iAASTRAReconstruct::FDK3D;
		}
		return AlgorithmStrings().indexOf(algo);
	}

	QString MapAlgoIndexToString(int astraIndex)
	{
		if (astraIndex < 0 || astraIndex >= AlgorithmStrings().size())
		{
			DEBUG_LOG("Invalid Algorithm Type selection!");
			return "Invalid";
		}
		return AlgorithmStrings()[astraIndex];
	}

	void AddCommonForwardReconstructParams(iAFilter* filter)
	{
		QStringList projectionGeometries; projectionGeometries << "cone";
		filter->AddParameter(ProjGeometry, Categorical, projectionGeometries);
		filter->AddParameter(DetSpcX, Continuous, 1.0);
		filter->AddParameter(DetSpcY, Continuous, 1.0);
		filter->AddParameter(ProjAngleStart, Continuous, 0.0);
		filter->AddParameter(ProjAngleEnd, Continuous, 359.0);
		filter->AddParameter(DstOrigDet, Continuous, 1.0);
		filter->AddParameter(DstOrigSrc, Continuous, 1.0);
	}
}


IAFILTER_CREATE(iAASTRAForwardProject)


iAASTRAForwardProject::iAASTRAForwardProject() :
	iAFilter("ASTRA Forward Projection", "Reconstruction/ASTRA Toolbox",
		"Forward Projection with the ASTRA Toolbox")
{
	AddCommonForwardReconstructParams(this);
	AddParameter(DetRowCnt, Discrete, 512);
	AddParameter(DetColCnt, Discrete, 512);
	AddParameter(ProjAngleCnt, Discrete, 360);
}

class CPPAstraCustomMemory: public astra::CFloat32CustomMemory
{
public:
	CPPAstraCustomMemory(size_t size)
	{
		m_fPtr = new astra::float32[size];
	}
	virtual ~CPPAstraCustomMemory() override
	{
		delete [] m_fPtr;
	}
};

void iAASTRAForwardProject::Run(QMap<QString, QVariant> const & parameters)
{
	vtkSmartPointer<vtkImageData> volImg = m_con->GetVTKImage();
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
	CreateConeProjGeom(projectorConfig, parameters);

	astra::XMLNode volGeomNode = projectorConfig.self.addChildNode("VolumeGeometry");
	FillVolumeGeometryNode(volGeomNode, volDim, volImg->GetSpacing());

	CPPAstraCustomMemory * volumeBuf = new CPPAstraCustomMemory(static_cast<size_t>(volDim[0]) * volDim[1] * volDim[2]);
	VTK_TYPED_CALL(SwapXYandCastToFloat, volImg->GetScalarType(), volImg, volumeBuf->m_fPtr);

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
	auto projImg = AllocateImage(VTK_FLOAT, projDim, projSpacing);
	float* projImgBuf = static_cast<float*>(projImg->GetScalarPointer());
	astra::float32*** projData3D = projectionData->getData3D();
	size_t imgIndex = 0;
	unsigned int projAngleCount = parameters[ProjAngleCnt].toUInt();
	unsigned int detectorColCnt = parameters[DetColCnt].toUInt();
	for (int z = 0; z < projDim[2]; ++z)
	{
		for (int y = 0; y < projDim[1]; ++y)
		{
#pragma omp parallel for
			for (int x = 0; x < projDim[0]; ++x)
			{
				projImgBuf[imgIndex + x] = projData3D[y][projAngleCount - z - 1][detectorColCnt - x - 1];
			}
			imgIndex += projDim[0];
		}
	}
	m_con->SetImage(projImg);
	m_con->Modified();

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
	AddCommonForwardReconstructParams(this);
	AddParameter(DetRowDim, Discrete, 1);
	AddParameter(DetColDim, Discrete, 3);
	AddParameter(ProjAngleDim, Discrete, 5);

	AddParameter(VolDimX, Discrete, 512);
	AddParameter(VolDimY, Discrete, 512);
	AddParameter(VolDimZ, Discrete, 512);
	AddParameter(VolSpcX, Continuous, 1.0);
	AddParameter(VolSpcY, Continuous, 1.0);
	AddParameter(VolSpcZ, Continuous, 1.0);

	AddParameter(AlgoType, Categorical, AlgorithmStrings());

	AddParameter(NumberOfIterations, Discrete, 100);
	AddParameter(CenterOfRotCorr, Boolean, false);
	AddParameter(CenterOfRotOfs, Continuous, 0.0);
}


template <typename T>
void SwapDimensions(vtkSmartPointer<vtkImageData> img, astra::float32* buf, int detColDim, int detRowDim, int projAngleDim)
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
			for (idx[0] = 0; idx[0] < dim[0]; ++idx[0])
			{
				int detCol    = idx[detColDimIdx];     if (detColDim >= 3)    { detCol    = dim[detColDimIdx]    - detCol    - 1; }
				int detRow    = idx[detRowDimIdx];     if (detRowDim >= 3)    { detRow    = dim[detRowDimIdx]    - detRow    - 1; }
				int projAngle = idx[projAngleDimIdx];  if (projAngleDim >= 3) { projAngle = dim[projAngleDimIdx] - projAngle - 1; }
				int bufIndex = detCol + ((projAngle + detRow*dim[projAngleDimIdx])*dim[detColDimIdx]);
				buf[bufIndex] = static_cast<float>(imgBuf[imgBufIdx + idx[0]]);
			}
			imgBufIdx += dim[0];
		}
	}
}


void iAASTRAReconstruct::Run(QMap<QString, QVariant> const & parameters)
{
	vtkSmartPointer<vtkImageData> projImg = m_con->GetVTKImage();
	int * projDim = projImg->GetDimensions();
	
	CPPAstraCustomMemory * projBuf = new CPPAstraCustomMemory(static_cast<size_t>(projDim[0]) * projDim[1] * projDim[2]);
	//VTK_TYPED_CALL(SwapDimensions, img->GetScalarType(), img, buf, m_detColDim, m_detRowDim, m_projAngleDim, m_detRowCnt, m_detColCnt, m_projAnglesCount);
	VTK_TYPED_CALL(SwapDimensions, projImg->GetScalarType(), projImg, projBuf->m_fPtr,
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
		CreateConeVecProjGeom(projectorConfig, parameters);
	}
	else
	{
		CreateConeProjGeom(projectorConfig, parameters);
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
	FillVolumeGeometryNode(volGeomNode, volDim, volSpacing);

	// create Algorithm and run:
	astra::CCudaProjector3D* projector = new astra::CCudaProjector3D();
	projector->initialize(projectorConfig);
	astra::CFloat32ProjectionData3DMemory * projectionData = new astra::CFloat32ProjectionData3DMemory(projector->getProjectionGeometry(), projBuf);
	astra::CFloat32VolumeData3DMemory * volumeData = new astra::CFloat32VolumeData3DMemory(projector->getVolumeGeometry(), 0.0f);
	switch (MapAlgoStringToIndex(parameters[AlgoType].toString()))
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
	auto volImg = AllocateImage(VTK_FLOAT, volDim, volSpacing);
	float* volImgBuf = static_cast<float*>(volImg->GetScalarPointer());
	astra::float32*** volData3D = volumeData->getData3D();
	size_t imgIndex = 0;
	for (int z = 0; z < volDim[2]; ++z)
	{
		for (int y = 0; y < volDim[1]; ++y)
		{
			#pragma omp parallel for
			for (int x = 0; x < volDim[0]; ++x)
			{
				volImgBuf[imgIndex + x] = volData3D[z][x][y];
			}
			imgIndex += volDim[0];
		}
	}
	m_con->SetImage(volImg);
	m_con->Modified();

	delete volumeData;
	delete projectionData;
	delete projector;
}



QSharedPointer<iAFilterRunnerGUI> iAASTRAFilterRunner::Create()
{
	return QSharedPointer<iAASTRAFilterRunner>(new iAASTRAFilterRunner());
}

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

void iAASTRAFilterRunner::Run(QSharedPointer<iAFilter> filter, MainWindow* mainWnd)
{
	if (!IsCUDAAvailable())
	{
		QMessageBox::warning(mainWnd, "ASTRA", "No CUDA device available. ASTRA toolbox operations require a CUDA-capable device.");
		return;
	}
	astra::CLogger::setOutputScreen(1, astra::LOG_INFO);
	bool success = astra::CLogger::setCallbackScreen(astraLogCallback);
	if (!success)
		DEBUG_LOG("Setting Astra log callback failed!");
	iAFilterRunnerGUI::Run(filter, mainWnd);
}

bool iAASTRAFilterRunner::AskForParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> & parameters,
	MdiChild* sourceMdi, MainWindow* mainWnd)
{
	dlg_ProjectionParameters dlg;
	dlg.setWindowTitle(filter->Name());
	int const * inputDim = sourceMdi->getImageData()->GetDimensions();
	if (filter->Name() == "ASTRA Forward Projection")
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
			parameters[AlgoType] = AlgorithmStrings()[1];
		dlg.fillAlgorithmValues(MapAlgoStringToIndex(parameters[AlgoType].toString()),
			parameters[NumberOfIterations].toUInt());
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
	if (filter->Name() == "ASTRA Forward Projection")
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
		parameters[AlgoType] = MapAlgoIndexToString(dlg.AlgorithmType->currentIndex());
		parameters[NumberOfIterations] = dlg.AlgorithmIterations->value();
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
		parameters[DetRowCnt] = inputDim[detRowDim % 3];
		parameters[DetColCnt] = inputDim[detColDim % 3];
		parameters[ProjAngleCnt] = inputDim[projAngleDim % 3];
	}
	return true;
}
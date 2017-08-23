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

#include "iAConnector.h"
#include "iAConsole.h"
#include "iAPerformanceHelper.h"
#include "iAToolsVTK.h"
#include "iATypedCallHelper.h"
#include "iAvec3.h"

#define ASTRA_CUDA
//#include <astra/AstraObjectManager.h>
#include <astra/CudaBackProjectionAlgorithm3D.h>
#include <astra/CudaFDKAlgorithm3D.h>
#include <astra/CudaCglsAlgorithm3D.h>
#include <astra/CudaSirtAlgorithm3D.h>
#include <astra/CudaForwardProjectionAlgorithm3D.h>
#include <astra/CudaProjector3D.h>

#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

#include <QtMath>  // for qDegreesToRadians

namespace
{
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
}


iAAstraAlgorithm::iAAstraAlgorithm(iAAstraAlgorithm::AlgorithmType type, QString const & filterName, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent) :
	iAAlgorithm(filterName, i, p, logger, parent),
	m_type(type)
{}


void iAAstraAlgorithm::performWork()
{
// errors are printed to the console by ASTRA:
//#ifdef _MSC_VER
//	AllocConsole();
//	freopen("CON", "w", stdout);
//#endif
	switch (m_type)
	{
	case FP3D:
		ForwardProject();
		break;
	case BP3D:
	case FDK3D:
	case SIRT3D:
	case CGLS3D:
		Reconstruct(m_type);
		break;
	default:
		DEBUG_LOG("Invalid Algorithm!");
	}
}


void iAAstraAlgorithm::SetFwdProjectParams(QString const & projGeomType, double detSpacingX, double detSpacingY, int detRowCnt, int detColCnt,
	double projAngleStart, double projAngleEnd, int projAnglesCount, double distOrigDet, double distOrigSource)
{
	m_projGeomType = projGeomType;
	m_detSpacingX = detSpacingX;
	m_detSpacingY = detSpacingY;
	m_detRowCnt = detRowCnt;
	m_detColCnt = detColCnt;
	m_projAngleStart = projAngleStart;
	m_projAngleEnd = projAngleEnd;
	m_projAnglesCount = projAnglesCount;
	m_distOrigDet = distOrigDet;
	m_distOrigSource = distOrigSource;
}


void iAAstraAlgorithm::SetReconstructParams(QString const & projGeomType, double detSpacingX, double detSpacingY, int detRowCnt, int detColCnt,
	double projAngleStart, double projAngleEnd, int projAnglesCount, double distOrigDet, double distOrigSource,
	int detRowDim, int detColDim, int projAngleDim, int volDim[3], double volSpacing[3], int numOfIterations,
	bool correctCenterOfRotation, double correctCenterOfRotationOffset)
{
	SetFwdProjectParams(projGeomType, detSpacingX, detSpacingY, detRowCnt, detColCnt, projAngleStart, projAngleEnd, projAnglesCount, distOrigDet, distOrigSource);
	m_detRowDim = detRowDim;
	m_detColDim = detColDim;
	m_projAngleDim = projAngleDim;
	for (int i = 0; i < 3; ++i)
	{
		m_volDim[i] = volDim[i];
		m_volSpacing[i] = volSpacing[i];
	}
	m_numberOfIterations = numOfIterations;
	m_correctCenterOfRotation = correctCenterOfRotation;
	m_correctCenterOfRotationOffset = correctCenterOfRotationOffset;
}


namespace
{
	void FillVolumeGeometryNode(astra::XMLNode & volGeomNode, int const volDim[3], double const volSpacing[3])
	{
		volGeomNode.addChildNode("GridColCount", volDim[1]);      // columns are "y-direction" (second index component in buffer) in astra
		volGeomNode.addChildNode("GridRowCount", volDim[0]);      // rows are "x-direction" (first index component in buffer) in astra
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


void iAAstraAlgorithm::ForwardProject()
{
	vtkSmartPointer<vtkImageData> img = getConnector()->GetVTKImage();
	int * dim = img->GetDimensions();
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
	astra::XMLNode projGeomNode = projectorConfig.self.addChildNode("ProjectionGeometry");
	projGeomNode.addAttribute("type", m_projGeomType.toStdString());
	projGeomNode.addChildNode("DetectorSpacingX", m_detSpacingX);
	projGeomNode.addChildNode("DetectorSpacingY", m_detSpacingY);
	projGeomNode.addChildNode("DetectorRowCount", m_detRowCnt);
	projGeomNode.addChildNode("DetectorColCount", m_detColCnt);
	projGeomNode.addChildNode("ProjectionAngles", linspace(qDegreesToRadians(m_projAngleStart),
		qDegreesToRadians(m_projAngleEnd), m_projAnglesCount).toStdString());
	projGeomNode.addChildNode("DistanceOriginDetector", m_distOrigDet);
	projGeomNode.addChildNode("DistanceOriginSource", m_distOrigSource);

	astra::XMLNode volGeomNode = projectorConfig.self.addChildNode("VolumeGeometry");
	FillVolumeGeometryNode(volGeomNode, dim, img->GetSpacing());

	iAPerformanceHelper inCopyTimer;
	inCopyTimer.start("Cast and copy to input buffer");
	astra::float32* buf = new astra::float32[dim[0] * dim[1] * dim[2]];
	VTK_TYPED_CALL(SwapXYandCastToFloat, img->GetScalarType(), img, buf);
	inCopyTimer.stop();

	iAPerformanceHelper astraSetupAndRunTimer;
	astraSetupAndRunTimer.start("ASTRA setup and run");
	astra::CCudaProjector3D* projector = new astra::CCudaProjector3D();
	projector->initialize(projectorConfig);
	astra::CFloat32ProjectionData3DMemory * projectionData = new astra::CFloat32ProjectionData3DMemory(projector->getProjectionGeometry());
	astra::CFloat32VolumeData3DMemory * volumeData = new astra::CFloat32VolumeData3DMemory(projector->getVolumeGeometry(), buf);
	astra::CCudaForwardProjectionAlgorithm3D* algorithm = new astra::CCudaForwardProjectionAlgorithm3D();
	algorithm->initialize(projector, projectionData, volumeData);
	algorithm->run();
	astraSetupAndRunTimer.stop();

	iAPerformanceHelper outCopyTimer;
	outCopyTimer.start("Copy to output buffer");
	int projDim[3] = { m_detColCnt, m_detRowCnt, m_projAnglesCount };     // "normalize" z spacing with projections count to make sinograms with different counts more easily comparable
	double projSpacing[3] = { m_detSpacingX, m_detSpacingY, m_detSpacingX * 180 / m_projAnglesCount };
	auto projImg = AllocateImage(VTK_FLOAT, projDim, projSpacing);
	float* projImgBuf = static_cast<float*>(projImg->GetScalarPointer());
	astra::float32*** projData3D = projectionData->getData3D();
	size_t imgIndex = 0;
	for (int z = 0; z < projDim[2]; ++z)
	{
		for (int y = 0; y < projDim[1]; ++y)
		{
#pragma omp parallel for
			for (int x = 0; x < projDim[0]; ++x)
			{
				projImgBuf[imgIndex + x] = projData3D[y][m_projAnglesCount - z - 1][m_detColCnt - x - 1];
			}
			imgIndex += projDim[0];
		}
	}
	getConnector()->SetImage(projImg);
	getConnector()->Modified();
	outCopyTimer.stop();

	iAPerformanceHelper deleteTimer;
	deleteTimer.start("Cleanup");
	delete[] buf;
	delete algorithm;
	delete volumeData;
	delete projectionData;
	delete projector;
	deleteTimer.stop();
}


void iAAstraAlgorithm::CreateConeProjGeom(astra::Config & projectorConfig)
{
	astra::XMLNode projGeomNode = projectorConfig.self.addChildNode("ProjectionGeometry");
	projGeomNode.addAttribute("type", "cone");
	projGeomNode.addChildNode("DetectorSpacingX", m_detSpacingX);
	projGeomNode.addChildNode("DetectorSpacingY", m_detSpacingY);
	projGeomNode.addChildNode("DetectorRowCount", m_detRowCnt);
	projGeomNode.addChildNode("DetectorColCount", m_detColCnt);
	projGeomNode.addChildNode("ProjectionAngles", linspace(qDegreesToRadians(m_projAngleStart),
		qDegreesToRadians(m_projAngleEnd), m_projAnglesCount).toStdString());
	projGeomNode.addChildNode("DistanceOriginDetector", m_distOrigDet);
	projGeomNode.addChildNode("DistanceOriginSource", m_distOrigSource);
}


void iAAstraAlgorithm::CreateConeVecProjGeom(astra::Config & projectorConfig, double centerOfRotationOffset)
{
	QString vectors;
	for (int i = 0; i<m_projAnglesCount; ++i)
	{
		double curAngle = qDegreesToRadians(m_projAngleStart) + i*(qDegreesToRadians(m_projAngleEnd) - qDegreesToRadians(m_projAngleStart)) / (m_projAnglesCount-1);
		iAVec3 sourcePos(
			sin(curAngle) * m_distOrigSource,
			-cos(curAngle) * m_distOrigSource,
			0);
		iAVec3 detectorCenter(
			-sin(curAngle) * m_distOrigDet,
			cos(curAngle) * m_distOrigDet,
			0);
		iAVec3 detectorPixelHorizVec(				// vector from detector pixel(0, 0) to(0, 1)
			cos(curAngle) * m_detSpacingX,
			sin(curAngle) * m_detSpacingX,
			0);
		iAVec3 detectorPixelVertVec(0, 0, m_detSpacingY); // vector from detector pixel(0, 0) to(1, 0)
		iAVec3 shiftVec = detectorPixelHorizVec.normalize() * m_correctCenterOfRotationOffset;
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
	projGeomNode.addChildNode("DetectorRowCount", m_detRowCnt);
	projGeomNode.addChildNode("DetectorColCount", m_detColCnt);
	projGeomNode.addChildNode("Vectors", vectors.toStdString());
}


template <typename T>
void SwapDimensions(vtkSmartPointer<vtkImageData> img, astra::float32* buf, int detColDim, int detRowDim, int projAngleDim)
{
	float* imgBuf = static_cast<float*>(img->GetScalarPointer());
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

/*
template <typename T>
void SwapDimensions(vtkSmartPointer<vtkImageData> img, astra::float32* buf, int detColDim, int detRowDim, int projAngleDim, int detRowCnt, int detColCnt, int projAngleCnt)
{
	int * dim = img->GetDimensions();
	vtkNew<vtkImageCast> cast;
	cast->SetInputData(img);
	cast->SetOutputScalarTypeToFloat();
	cast->Update();
	vtkSmartPointer<vtkImageData> float32Img = cast->GetOutput();
	FOR_VTKIMG_PIXELS(img, x, y, z)
	{
		int detCol = ((detColDim % 3) == 0) ? x : ((detColDim % 3) == 1) ? y : z;
		if (detColDim >= 3)
		{
			detCol = detColCnt - detCol - 1;
		}
		int detRow = ((detRowDim % 3) == 0) ? x : ((detRowDim % 3) == 1) ? y : z;
		if (detRowDim >= 3)
		{
			detRow = detRowCnt - detRow - 1;
		}
		int projAngle = ((projAngleDim % 3) == 0) ? x : ((projAngleDim % 3) == 1) ? y : z;
		if (projAngleDim >= 3)
		{
			projAngle = projAngleCnt - projAngle - 1;
		}
		int index = detCol + projAngle*detColCnt + detRow*detColCnt*projAngleCnt;
		if (index < 0 || index >= projAngleCnt*detRowCnt*detColCnt)
		{
			DEBUG_LOG(QString("Index out of bounds: %1 (valid range: 0..%2)").arg(index).arg(projAngleCnt*detRowCnt*detColCnt));
		}
		buf[index] = img->GetScalarComponentAsFloat(x, y, z, 0);
	}
}
*/


void iAAstraAlgorithm::Reconstruct(AlgorithmType type)
{
	vtkSmartPointer<vtkImageData> img = getConnector()->GetVTKImage();
	int * dim = img->GetDimensions();

	iAPerformanceHelper inCopyTimer;
	inCopyTimer.start("Cast and Copy to input buffer");
	astra::float32* buf = new astra::float32[dim[0] * dim[1] * dim[2]];
	//VTK_TYPED_CALL(SwapDimensions, img->GetScalarType(), img, buf, m_detColDim, m_detRowDim, m_projAngleDim, m_detRowCnt, m_detColCnt, m_projAnglesCount);
	VTK_TYPED_CALL(SwapDimensions, img->GetScalarType(), img, buf, m_detColDim, m_detRowDim, m_projAngleDim);
	inCopyTimer.stop();

	iAPerformanceHelper configTimer;
	configTimer.start("Config");
	// create XML configuration:
	astra::Config projectorConfig;
	projectorConfig.initialize("Projector3D");
	astra::XMLNode gpuIndexOption = projectorConfig.self.addChildNode("Option");
	gpuIndexOption.addAttribute("key", "GPUIndex");
	gpuIndexOption.addAttribute("value", "0");
	assert(m_projGeomType == "cone");
	if (m_correctCenterOfRotation)
	{
		CreateConeVecProjGeom(projectorConfig, m_correctCenterOfRotationOffset);
	}
	else
	{
		CreateConeProjGeom(projectorConfig);
	}
	astra::XMLNode volGeomNode = projectorConfig.self.addChildNode("VolumeGeometry");
	FillVolumeGeometryNode(volGeomNode, m_volDim, m_volSpacing);
	configTimer.stop();

	iAPerformanceHelper astraSetupAndRunTimer;
	astraSetupAndRunTimer.start("ASTRA setup and run");
	// create Algorithm and run:
	astra::CCudaProjector3D* projector = new astra::CCudaProjector3D();
	projector->initialize(projectorConfig);
	astra::CFloat32ProjectionData3DMemory * projectionData = new astra::CFloat32ProjectionData3DMemory(projector->getProjectionGeometry(), buf);
	astra::CFloat32VolumeData3DMemory * volumeData = new astra::CFloat32VolumeData3DMemory(projector->getVolumeGeometry(), 0.0f);
	switch (type)
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
			sirtalgo->run(m_numberOfIterations);
			delete sirtalgo;
			break;
		}
		case CGLS3D: {
			astra::CCudaCglsAlgorithm3D* cglsalgo = new astra::CCudaCglsAlgorithm3D();
			cglsalgo->initialize(projector, projectionData, volumeData);
			cglsalgo->run(m_numberOfIterations);
			delete cglsalgo;
			break;
		}
		default:
			DEBUG_LOG("Unknown backprojection algorithm selected!");
	}
	astraSetupAndRunTimer.stop();

	iAPerformanceHelper outCopyTimer;
	outCopyTimer.start("Copy to output buffer");
	// retrieve result image:
	auto volImg = AllocateImage(VTK_FLOAT, m_volDim, m_volSpacing);
	float* volImgBuf = static_cast<float*>(volImg->GetScalarPointer());
	/*
	FOR_VTKIMG_PIXELS(volImg, x, y, z)
	{
		volImg->SetScalarComponentFromFloat(x, y, z, 0, volumeData->getData3D()[z][x][y]);
	}
	*/
	astra::float32*** volData3D = volumeData->getData3D();
	size_t imgIndex = 0;
	for (int z = 0; z < m_volDim[2]; ++z)
	{
		for (int y = 0; y < m_volDim[1]; ++y)
		{
			#pragma omp parallel for
			for (int x = 0; x < m_volDim[0]; ++x)
			{
				volImgBuf[imgIndex + x] = volData3D[z][x][y];
			}
			imgIndex += m_volDim[0];
		}
	}

	getConnector()->SetImage(volImg);
	getConnector()->Modified();
	outCopyTimer.stop();

	iAPerformanceHelper deleteTimer;
	deleteTimer.start("Cleanup");
	delete[] buf;
	delete volumeData;
	delete projectionData;
	delete projector;
	deleteTimer.stop();
}

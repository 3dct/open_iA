/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#pragma once

#include "iAImageGraphTypes.h"
#include "iASeedType.h"

#include "iAFilter.h"

#include <QSharedPointer>
#include <QVector>

#include "iAImageTypes.h"

class iANormalizer;
class iALabelMapper;
class iAImageGraph;
class iASpectraDistance;

struct iARWInputChannel
{
	QSharedPointer<iASpectralVoxelData const> image;
	QSharedPointer<iASpectraDistance> distanceFunc;
	QSharedPointer<iANormalizer> normalizeFunc;
	double             weight;
};

struct iARWResult
{
	iAITKIO::ImagePointer labelledImage;
	QVector<iAITKIO::ImagePointer> probabilityImages;
};

class iARandomWalker: public QThread
{
public:
	//! initialize "standard" random walker
	iARandomWalker(iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType depth,
		double const spacing[3],
		QSharedPointer<QVector<iARWInputChannel> > inputChannels,
		SeedVector const & seeds
	);
	QSharedPointer<iARWResult> GetResult();
private:
	int m_vertexCount;
	QSharedPointer<QVector<iARWInputChannel> > m_inputChannels;
	QSharedPointer<iAImageGraph> m_imageGraph;
	SeedVector const & m_seeds;
	int m_minLabel, m_maxLabel;
	double m_spacing[3];
	QSharedPointer<iARWResult>  m_result;

	virtual void run();
};

class vtkImageData;
class vtkPolyData;
class QObject;

class iAExtendedRandomWalker: public QThread
{
public:
	//! initialize "extended" random walker
	iAExtendedRandomWalker(
		iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType depth,
		double const spacing[3],
		QSharedPointer<QVector<iARWInputChannel> > inputChannels,
		QSharedPointer<QVector<PriorModelImagePointer> > priorModel,
		double priorModelWeight,
		int maxIterations
	);
	iAExtendedRandomWalker(
		iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType depth,
		double const spacing[3],
		QSharedPointer<QVector<iARWInputChannel> > inputChannels,
		QSharedPointer<QVector<PriorModelImagePointer> > priorModel,
		double priorModelWeight,
		int maxIterations,
		vtkImageData* i,
		vtkPolyData* p,
		iALogger* logger,
		QObject* parent
	);
	QSharedPointer<iARWResult> GetResult();
	QString const & GetError() const;
private:
	int m_vertexCount;
	QSharedPointer<QVector<iARWInputChannel> > m_inputChannels;
	QSharedPointer<iAImageGraph> m_imageGraph;
	QSharedPointer<QVector<PriorModelImagePointer> > m_priorModel;
	double m_priorModelWeight;
	int m_labelCount;
	double m_spacing[3];
	QSharedPointer<iARWResult>  m_result;
	QString m_errorMsg;
	int m_maxIterations;

	virtual void run();
};

/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "iAUncertaintyImages.h"

#include <QSharedPointer>
#include <QString>
#include <QVector>

class iAEnsembleDescriptorFile;
class iAMember;
class iAModalityList;
class iASamplingResults;

class iAEnsemble: public iAUncertaintyImages
{
public:
	enum MemberAttributes
	{
		UncertaintyMean,
		UncertaintyVar
	};
	~iAEnsemble();
	//! create from string
	static QSharedPointer<iAEnsemble> Create(int entropyBinCount,
		QSharedPointer<iAEnsembleDescriptorFile> ensembleFile);
	static QSharedPointer<iAEnsemble> Create(int entropyBinCount,
		QVector<QSharedPointer<iAMember> > members,
		QSharedPointer<iASamplingResults> superSet, int labelCount, QString const & cachePath, int id,
		IntImage::Pointer referenceImage);
	vtkImagePointer GetEntropy(int source) const override;
	vtkImagePointer GetReference() const override;
	bool HasReference() const override;
	QString GetSourceName(int source) const override;
	QVector<IntImage::Pointer> const & GetLabelDistribution() const;
	int LabelCount() const;
	double * EntropyHistogram() const;
	int EntropyBinCount() const;
	QSharedPointer<iAMember> const Member(size_t memberIdx) const;
	size_t MemberCount() const;
	std::vector<double> const & MemberAttribute(size_t idx) const;
	QSharedPointer<iASamplingResults> Sampling(size_t idx) const;
	QString const & CachePath() const;
	QSharedPointer<iAEnsemble> AddSubEnsemble(QVector<int> memberIDs, int newEnsembleID);
	QVector<QSharedPointer<iAEnsemble> > SubEnsembles() const;
	int ID() const;
	void Store();
	QSharedPointer<iAEnsembleDescriptorFile> EnsembleFile();
	void WriteFullDataFile(QString const & filename, bool writeIntensities, bool writeMemberLabels, bool writeMemberProbabilities, bool writeEnsembleUncertainties, QSharedPointer<iAModalityList> modalities);
private:
	bool LoadSampling(QString const & fileName, int labelCount, int id);
	void CreateUncertaintyImages();
	//! constructor; use static Create methods instead!
	iAEnsemble(int entropyBinCount);
	QVector<QSharedPointer<iASamplingResults> > m_samplings;

	QVector<vtkImagePointer> m_entropy;
	QVector<IntImage::Pointer> m_labelDistr;
	IntImage::Pointer m_referenceImage;
	DoubleImage::Pointer m_entropyAvgEntropy;
	DoubleImage::Pointer m_labelDistrEntropy;
	DoubleImage::Pointer m_probSumEntropy;
	DoubleImage::Pointer m_neighbourhoodAvgEntropy3x3;
	DoubleImage::Pointer m_neighbourhoodAvgEntropy5x5;
	QVector<DoubleImage::Pointer> m_probDistr;
	std::vector<double> m_memberEntropyAvg;
	std::vector<double> m_memberEntropyVar;
	int m_labelCount;
	double * m_entropyHistogram;
	int m_entropyBinCount;
	QString m_cachePath;
	QSharedPointer<iAEnsembleDescriptorFile> m_ensembleFile;
	QVector<QSharedPointer<iAEnsemble> > m_subEnsembles;
};

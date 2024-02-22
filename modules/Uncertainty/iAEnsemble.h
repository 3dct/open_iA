// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAUncertaintyImages.h"

#include <iAITKImageTypes.h>

#include <QString>
#include <QVector>

#include <memory>

class iADataSet;
class iAEnsembleDescriptorFile;
class iASingleResult;
class iASamplingResults;

class iAEnsemble: public iAUncertaintyImages
{
public:
	enum MemberAttributes
	{
		UncertaintyMean,
		UncertaintyVar
	};
	~iAEnsemble() override;
	//! create from string
	static std::shared_ptr<iAEnsemble> Create(int entropyBinCount,
		std::shared_ptr<iAEnsembleDescriptorFile> ensembleFile);
	static std::shared_ptr<iAEnsemble> Create(int entropyBinCount,
		QVector<std::shared_ptr<iASingleResult> > members,
		std::shared_ptr<iASamplingResults> superSet, int labelCount, QString const & cachePath, int id,
		IntImage::Pointer referenceImage);
	vtkImagePointer GetEntropy(int source) const override;
	vtkImagePointer GetReference() const override;
	bool HasReference() const override;
	QString GetSourceName(int source) const override;
	QVector<IntImage::Pointer> const & GetLabelDistribution() const;
	int LabelCount() const;
	double * EntropyHistogram() const;
	int EntropyBinCount() const;
	std::shared_ptr<iASingleResult> const Member(size_t memberIdx) const;
	size_t MemberCount() const;
	std::vector<double> const & MemberAttribute(size_t idx) const;
	std::shared_ptr<iASamplingResults> Sampling(size_t idx) const;
	QString const & CachePath() const;
	std::shared_ptr<iAEnsemble> AddSubEnsemble(QVector<int> memberIDs, int newEnsembleID);
	QVector<std::shared_ptr<iAEnsemble> > SubEnsembles() const;
	int ID() const;
	std::shared_ptr<iAEnsembleDescriptorFile> EnsembleFile();
	void writeFullDataFile(QString const & filename, bool writeIntensities, bool writeMemberLabels, bool writeMemberProbabilities, bool writeEnsembleUncertainties, std::map<size_t, std::shared_ptr<iADataSet>> dataSets);
private:
	bool LoadSampling(QString const & fileName, int id);
	void CreateUncertaintyImages();
	//! constructor; use static Create methods instead!
	iAEnsemble(int entropyBinCount);
	QVector<std::shared_ptr<iASamplingResults> > m_samplings;

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
	std::shared_ptr<iAEnsembleDescriptorFile> m_ensembleFile;
	QVector<std::shared_ptr<iAEnsemble> > m_subEnsembles;
};

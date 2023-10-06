// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "metafilters_export.h"

#include <iAAttributes.h>
#include <iAITKImageTypes.h>
#include <iAITKIO.h>

#include <QString>
#include <QVector>

#include <memory>

class iASamplingResults;

class MetaFilters_API iASingleResult
{
public:
	static const QString ValueSplitString;
	//! create from string
	static std::shared_ptr<iASingleResult> create(
		QString const & line,
		iASamplingResults const & sampling,
		std::shared_ptr<iAAttributes> attributes,
		bool showErrorOutput);

	static std::shared_ptr<iASingleResult> create(
		int id,
		iASamplingResults const & sampling,
		QVector<QVariant> const & parameter,
		QString const & fileName);

	//! retrieve all attritutes of the given type as string
	//! (such as can be passed into Create method above)
	QString toString(std::shared_ptr<iAAttributes> attributes, int type);

	//! retrieve labelled image
	iAITKIO::ImagePointer const labelImage();

	//! discards full detail images from memory
	// TODO: check if that can be done automatically somehow
	void discardDetails();

	void discardProbability();

	//! get attribute (parameter or characteristic)
	double attribute(int id) const;

	//! set attribute (parameter or characteristic)
	void setAttribute(int id, double value);

	//! returns the ID of the single result
	int id() const;

	//! loads a single probability image with the  given index
	iAITKIO::ImagePointer probabilityImg(int l);

	//! loads all probability images (given the number of them) and returns them as QVector
	QVector<ProbabilityImagePointer> probabilityImgs(int labelCount);

	bool probabilityAvailable() const;

	void setLabelImage(iAITKIO::ImagePointer labelImg);

	void addProbabilityImages(QVector<iAITKIO::ImagePointer> & probImgs);

	int datasetID() const;
	std::shared_ptr<iAAttributes> attributes() const;
private:
	//! constructor; use static Create methods instead!
	iASingleResult(int id, iASamplingResults const & sampling);
	QVector<QVariant> m_attributeValues;
	iASamplingResults const & m_sampling;
	int m_id;
	iAITKIO::ImagePointer m_labelImg;
	QVector<iAITKIO::ImagePointer> m_probabilityImg;
	QString m_fileName;

	bool loadLabelImage();

	QString labelPath() const;
	QString probabilityPath(int label) const;
	QString folder() const;
};

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
#pragma once

#include "MetaFilters_export.h"

#include <iAAttributes.h>
#include <iAITKImageTypes.h>

#include <QSharedPointer>
#include <QString>
#include <QVector>

class iASamplingResults;

class MetaFilters_API iASingleResult
{
public:
	static const QString ValueSplitString;
	//! create from string
	static QSharedPointer<iASingleResult> create(
		QString const & line,
		iASamplingResults const & sampling,
		QSharedPointer<iAAttributes> attributes);

	static QSharedPointer<iASingleResult> create(
		int id,
		iASamplingResults const & sampling,
		QVector<double> const & parameter,
		QString const & fileName);

	//! retrieve all attritutes of the given type as string
	//! (such as can be passed into Create method above)
	QString toString(QSharedPointer<iAAttributes> attributes, int type);

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
	QVector<ProbabilityImagePointer> iASingleResult::probabilityImgs(int labelCount);

	bool probabilityAvailable() const;

	void setLabelImage(iAITKIO::ImagePointer labelImg);

	void addProbabilityImages(QVector<iAITKIO::ImagePointer> & probImgs);

	int datasetID() const;
	QSharedPointer<iAAttributes> attributes() const;
private:
	//! constructor; use static Create methods instead!
	iASingleResult(int id, iASamplingResults const & sampling);
	//! for now, value-type agnostic storage of values:
	QVector<double> m_attributeValues;
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

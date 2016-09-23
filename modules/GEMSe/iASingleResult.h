/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include "iAITKIO.h" // TODO: replace?

#include <QSharedPointer>
#include <QString>
#include <QVector>

class iAAttributes;
class iASamplingResults;

class iASingleResult
{
public:

	//! create from string
	static QSharedPointer<iASingleResult> Create(
		QString const & line,
		iASamplingResults const & sampling,
		QSharedPointer<iAAttributes> attributes);

	static QSharedPointer<iASingleResult> Create(
		int id,
		iASamplingResults const & sampling,
		QVector<double> const & parameter);

	//! retrieve all attritutes of the given type as string
	//! (such as can be passed into Create method above)
	QString ToString(QSharedPointer<iAAttributes> attributes, int type);

	iASingleResult(int id, iASamplingResults const & sampling);

	//! retrieve labelled image
	iAITKIO::ImagePointer const GetLabelledImage();

	//! discards full detail images from memory
	// TODO: check if that can be done automatically somehow
	void DiscardDetails();

	void DiscardProbability();

	//! get attribute (parameter or characteristic)
	double GetAttribute(int id) const;
	
	//! set attribute (parameter or characteristic)
	void SetAttribute(int id, double value);

	int GetID();

	iAITKIO::ImagePointer GetProbabilityImg(int l);

	void SetLabelImage(iAITKIO::ImagePointer labelImg);

	void AddProbabilityImages(QVector<iAITKIO::ImagePointer> & probImgs);

	int GetDatasetID() const;
	QSharedPointer<iAAttributes> GetAttributes() const;
private:
	//! for now, value-type agnostic storage of values:
	QVector<double> m_attributeValues;
	iASamplingResults const & m_sampling;
	int m_id;
	iAITKIO::ImagePointer m_labelImg;
	QVector<iAITKIO::ImagePointer> m_probabilityImg;

	bool LoadLabelImage();

	QString GetLabelPath() const;
	QString GetProbabilityPath(int label) const;
	QString GetFolder() const;
};

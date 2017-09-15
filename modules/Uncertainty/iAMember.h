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
#pragma once

#include "iAITKIO.h" // TODO: replace?

#include <QSharedPointer>
#include <QString>
#include <QVector>

class iAAttributes;
class iASamplingResults;

typedef itk::Image<double, 3> DoubleImage;

class iAMember
{
public:

	//! create from string
	static QSharedPointer<iAMember> Create(
		QString const & line,
		iASamplingResults const & sampling,
		QSharedPointer<iAAttributes> attributes);

	static QSharedPointer<iAMember> Create(
		int id,
		iASamplingResults const & sampling,
		QVector<double> const & parameter,
		QString const & fileName);

	//! retrieve all attritutes of the given type as string
	//! (such as can be passed into Create method above)
	QString ToString(QSharedPointer<iAAttributes> attributes, int type);

	//! retrieve labelled image
	iAITKIO::ImagePointer const LabelImage();

	//! get attribute (parameter or characteristic)
	double Attribute(int id) const;
	
	//! set attribute (parameter or characteristic)
	void SetAttribute(int id, double value);

	int ID();

	QVector<DoubleImage::Pointer> ProbabilityImgs(int labelCount);

	bool ProbabilityAvailable() const;

	int DatasetID() const;
	QSharedPointer<iAAttributes> Attributes() const;
private:
	//! constructor; use static Create methods instead!
	iAMember(int id, iASamplingResults const & sampling);
	//! for now, value-type agnostic storage of values:
	QVector<double> m_attributeValues;
	iASamplingResults const & m_sampling;
	int m_id;
	QString m_fileName;

	QString LabelPath() const;
	QString ProbabilityPath(int label) const;
	QString Folder() const;
};

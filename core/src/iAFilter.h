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

#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>

#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QVector>

class iAAttributeDescriptor;

class vtkImageData;

typedef QSharedPointer<iAAttributeDescriptor> pParameter;

//! Base class for image filters
//! Derived classes should:
//!     - fill the m_parameters vector in their constructor
//!     - override the SetParameters method to transfer the parameters to the actual algorithm
//!     - override the Run method to perform the actual calculations
//!       on m_inImg and (allocate and) store the result in m_outImg
class open_iA_Core_API iAFilter
{
public:
	iAFilter(QString const & name, QString const & category, QString const & description);
	virtual ~iAFilter();
	QString Name() const;
	QString Category() const;
	QString Description() const;
	QVector<pParameter> const & Parameters() const;
	void SetInput(vtkSmartPointer<vtkImageData> inImg);
	vtkSmartPointer<vtkImageData> Output() const;
	virtual void Run(QMap<QString, QVariant> parameters) = 0;
protected:
	QVector<pParameter> m_parameters;
	vtkSmartPointer<vtkImageData> m_inImg;
	vtkSmartPointer<vtkImageData> m_outImg;
	QString m_name, m_category, m_description;
};

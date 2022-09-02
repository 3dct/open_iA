/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iADataSet.h"

#include <vtkSmartPointer.h>

class vtkPolyDataAlgorithm;

//! a geometric object produced by some VTK algorithm such as the various sources
class iAbase_API iAGeometricObject : public iADataSet
{
public:
	iAGeometricObject(QString const& name, vtkSmartPointer<vtkPolyDataAlgorithm> source);
	vtkSmartPointer<vtkPolyDataAlgorithm> source();
	QString info() const override;
	std::array<double, 3> unitDistance() const override;
	double const* bounds() const;

private:
	iAGeometricObject(iAGeometricObject const& other) = delete;
	iAGeometricObject& operator=(iAGeometricObject const& other) = delete;
	vtkSmartPointer<vtkPolyDataAlgorithm> m_polySource;
};

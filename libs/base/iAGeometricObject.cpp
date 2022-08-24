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
#include "iAGeometricObject.h"

#include <vtkPolyDataAlgorithm.h>

#include <array>

iAGeometricObject::iAGeometricObject(QString const& name, QString const& fileName, vtkSmartPointer<vtkPolyDataAlgorithm> source) :
	iADataSet(fileName, iADataSetType::Mesh, name),
	m_polySource(source)
{}

vtkSmartPointer<vtkPolyDataAlgorithm> iAGeometricObject::source()
{
	return m_polySource;
}

QString iAGeometricObject::info() const
{
	return QString("Geometric object: %1; %2")
		.arg(name())
		.arg(boundsStr(bounds()));
}

double const* iAGeometricObject::bounds() const
{
	auto out = m_polySource->GetOutput();
	static double bds[6];
	out->GetBounds(bds);
	return bds;
}

std::array<double, 3> iAGeometricObject::unitDistance() const
{
	m_polySource->Update();
	const double Divisor = 100;
	return {
		(bounds()[1] - bounds()[0]) / Divisor,
		(bounds()[3] - bounds()[2]) / Divisor,
		(bounds()[5] - bounds()[4]) / Divisor
	};

}

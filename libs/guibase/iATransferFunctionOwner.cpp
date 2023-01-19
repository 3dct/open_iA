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
#include "iATransferFunctionOwner.h"

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <iALog.h>

#include <cassert>

iATransferFunctionOwner::iATransferFunctionOwner(vtkSmartPointer<vtkColorTransferFunction> ctf, vtkSmartPointer<vtkPiecewiseFunction> otf, bool opacityRamp):
	m_ctf(ctf),
	m_otf(otf),
	m_opacityRamp(opacityRamp)
{}

iATransferFunctionOwner::iATransferFunctionOwner(double const range[2], bool opacityRamp):
	m_ctf(vtkSmartPointer<vtkColorTransferFunction>::New()),
	m_otf(vtkSmartPointer<vtkPiecewiseFunction>::New()),
	m_opacityRamp(opacityRamp)
{
	resetFunctions(range);
}

void iATransferFunctionOwner::resetFunctions()
{
	resetFunctions(m_ctf->GetRange());
}

vtkPiecewiseFunction* iATransferFunctionOwner::opacityTF()
{
	assert(m_otf);
	return m_otf;
}

vtkColorTransferFunction* iATransferFunctionOwner::colorTF()
{
	assert(m_ctf);
	return m_ctf;
}

void iATransferFunctionOwner::resetFunctions(double const range[2])
{
	defaultColorTF(m_ctf, range);
	defaultOpacityTF(m_otf, range, m_opacityRamp);
}

/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iATransferFunction.h"
#include "iAcore_export.h"

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartPointer.h>

#include <QSharedPointer>

class vtkImageData;

class QString;

//! Unite a color transfer function, an opacity transfer function
//! and GUI classes used for viewing a histogram of the data and for editing the transfer functions.
class iAcore_API iAModalityTransfer : public iATransferFunction
{
public:
	iAModalityTransfer(double const range[2]);
	void computeStatistics(vtkSmartPointer<vtkImageData> img);
	//void resetHistogram();
	bool statisticsComputed() const;

	//! @{ functions overridden from iATransferFunction:
	vtkPiecewiseFunction* opacityTF() override;
	vtkColorTransferFunction* colorTF() override;
	void resetFunctions() override;
	//! @}

private:
	vtkSmartPointer<vtkColorTransferFunction> m_ctf;
	vtkSmartPointer<vtkPiecewiseFunction> m_otf;
	bool m_statisticsComputed;
	bool m_opacityRamp;  //! whether to use a varying opacity in default TF
	double m_range[2];
};

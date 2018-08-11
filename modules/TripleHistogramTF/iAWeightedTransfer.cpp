/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include "iAWeightedTransfer.h"
#include "iAModalityTransfer.h"

// TODO: remove
#include "ColorInterpolator.h"

// Weighted iATransferFunction

iAWeightedTransfer::iAWeightedTransfer(iATransferFunction* tf1, iATransferFunction* tf2, iATransferFunction* tf3)
{
	setTransferFunctions(tf1, tf2, tf3);
}

iAWeightedTransfer::~iAWeightedTransfer()
{

}

void iAWeightedTransfer::setTransferFunctions(iATransferFunction* tf1, iATransferFunction* tf2, iATransferFunction* tf3) {
	m_tf1 = tf1;
	m_tf2 = tf2;
	m_tf3 = tf3;
}

/*iAWeightedColorFunction* iAWeightedTransfer::GetColorFunction()
{
	return m_cf;
}

iAWeightedOpacityFunction* iAWeightedTransfer::GetOpacityFunction()
{
	return m_of;
}*/

void iAWeightedTransfer::GetColor(double v, double rgb[3])
{
	double c1[3], c2[3], c3[3];

	m_tf1->GetColorFunction()->GetColor(v, c1);
	m_tf2->GetColorFunction()->GetColor(v, c1);
	m_tf3->GetColorFunction()->GetColor(v, c1);

	ColorInterpolator::getInstance()->interpolateColor3(
		c1, c2, c3,
		m_weight.getAlpha(), m_weight.getBeta(),
		rgb);
}

double iAWeightedTransfer::GetOpacity(double v)
{
	return ColorInterpolator::getInstance()->interpolateAlpha3(
		m_tf1->GetOpacityFunction()->GetValue(v),
		m_tf2->GetOpacityFunction()->GetValue(v),
		m_tf3->GetOpacityFunction()->GetValue(v),
		m_weight.getAlpha(), m_weight.getBeta());
}
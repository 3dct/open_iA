/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenb�ck, B. Fr�hler, M. Schiwarth       *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/

#include "iATripleModalityWidget.h"
#include "iAHistogramStack.h"
#include "iAHistogramTriangle.h"

iAHistogramAbstract* iAHistogramAbstract::buildHistogramAbstract(iAHistogramAbstractType type, iATripleModalityWidget *tmw, MdiChild *mdiChild, Qt::WindowFlags f) {
	switch (type) {
	case STACK:
		return new iAHistogramStack(tmw, tmw, mdiChild, f);

	case TRIANGLE:
		return new iAHistogramTriangle(tmw, tmw, mdiChild, f);
	}

	throw "Unexpected type";
}

void iAHistogramAbstract::updateModalityNames(QString const names[3])
{}

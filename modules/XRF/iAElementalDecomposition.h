/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#pragma once

#include "iAEnergySpectrum.h"

#include <QSharedPointer>

class iAElementSpectralInfo;

/**
 * \brief determine distribution of given elements in the given spectrum
 * \param unknownSpectrum the spectrum to analyze
 * \param elementSpectra the spectra of the elements the unknown spectrum is composed of
 * \param threshold the minimum count in the unknown spectrum to consider  (i.e. all
 *		data points having everywhere (data and all elemental spectra) a count below that
 *		threshold are discarded)
 * \param result the storage space for the result;
 * \return true if a fit could be found; false if it was not possible (because the unknown
 *		spectrum e.g. did not contain any data values above threshold)
 */
bool fitSpectrum(
	iAEnergySpectrum const & unknownSpectrum,
	QSharedPointer<QVector<QSharedPointer<iAEnergySpectrum> > > elements,
	CountType threshold,
	QVector<double> & result);

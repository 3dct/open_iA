// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAEnergySpectrum.h"

#include <memory>

class iAElementSpectralInfo;

//! Determine the distribution of given elements in the given spectrum.
//! @param unknownSpectrum the spectrum to analyze
//! @param elementSpectra the spectra of the elements the unknown spectrum is composed of
//! @param threshold the minimum count in the unknown spectrum to consider  (i.e. all
//!		data points having everywhere (data and all elemental spectra) a count below that
//!		threshold are discarded)
//! @param result the storage space for the result;
//! @return true if a fit could be found; false if it was not possible (because the unknown
//!		spectrum e.g. did not contain any data values above threshold)
bool fitSpectrum(
	iAEnergySpectrum const & unknownSpectrum,
	std::shared_ptr<QVector<std::shared_ptr<iAEnergySpectrum> > > elements,
	CountType threshold,
	QVector<double> & result);

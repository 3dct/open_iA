// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAProximityDistance.h"

iAProximityDistance::iAProximityDistance(
	std::vector<double>* weights, csvDataType::ArrayType* data, int amountOfCharas, int amountOfElems)
	: m_weights(weights),
	m_matrix(data),
	m_amountOfCharas(amountOfCharas),
	m_amountOfElems(amountOfElems)
{
}

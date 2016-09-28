#pragma once

#include "iAImageTree.h"

void CalculateMeasures(LabelImagePointer refImg, LabelImagePointer curImg, int labelCount,
	double & dice, double & kappa, double & oa, double &precision, double &recall);

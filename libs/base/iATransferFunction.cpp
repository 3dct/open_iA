// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iATransferFunction.h"

#include "iALog.h"
#include "iAMathUtility.h"

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <QString>    // to be able to create QString from const char *

iATransferFunction::~iATransferFunction()
{}

void iATransferFunction::ensureValidity(double valueRange[2])
{
	auto ctf = colorTF();
	auto otf = opacityTF();
	double ctfRange[2], otfRange[2];
	ctf->GetRange(ctfRange);
	otf->GetRange(otfRange);
	if (ctf->GetSize() != otf->GetSize() || !dblApproxEqual(otfRange[0], ctfRange[0]) || !dblApproxEqual(otfRange[1], ctfRange[1]))
	{
		LOG(lvlWarn, "Invalid transfer function - either the number of values or the ranges between color and opacity transfer function do not match. Resetting to default!");
		defaultColorTF(ctf, valueRange);
		defaultOpacityTF(otf, valueRange, true);
		return;
	}
	double minValColor[3], maxValColor[3];
	double minValOpacity, maxValOpacity;
	ctf->GetColor(valueRange[0], minValColor);
	ctf->GetColor(valueRange[1], maxValColor);
	minValOpacity = otf->GetValue(valueRange[0]);
	maxValOpacity = otf->GetValue(valueRange[1]);

	// we need points exactly at value start and end:
	if (!dblApproxEqual(valueRange[0], ctfRange[0]))
	{
		ctf->AddRGBPoint(valueRange[0], minValColor[0], minValColor[1], minValColor[2]);
		otf->AddPoint(valueRange[0], minValOpacity);
	}
	if (!dblApproxEqual(valueRange[1], ctfRange[1]))
	{
		ctf->AddRGBPoint(valueRange[1], maxValColor[0], maxValColor[1], maxValColor[2]);
		otf->AddPoint(valueRange[1], maxValOpacity);
	}
	// remove values outside of range:
	for (int i = 0; i < ctf->GetSize(); ++i)
	{
		double ctfVals[6], otfVals[4];
		ctf->GetNodeValue(i, ctfVals);
		otf->GetNodeValue(i, otfVals);
		if (!dblApproxEqual(ctfVals[0], otfVals[0]))
		{
			LOG(lvlError, QString("Invalid transfer function - values at index %1 don't match (ctf: %2, otf: %3").arg(ctfVals[0]).arg(otfVals[0]));
		}
		if (ctfVals[0] < valueRange[0])
		{
			ctf->RemovePoint(ctfVals[0]);
			otf->RemovePoint(otfVals[0]);
		}
		if (ctfVals[0] > valueRange[1])
		{
			ctf->RemovePoint(ctfVals[0]);
			otf->RemovePoint(otfVals[0]);
		}
	}
	if (!dblApproxEqual(valueRange[0], ctfRange[0]) || !dblApproxEqual(valueRange[1], ctfRange[1]))
	{
		LOG(lvlInfo, QString("Transfer function range (%1, %2) did not fit the value range (%3, %4), it was adapted to match!")
			.arg(ctfRange[0]).arg(ctfRange[1])
			.arg(valueRange[0]).arg(valueRange[1]));
	}
}

void defaultColorTF(vtkSmartPointer<vtkColorTransferFunction> cTF, double const range[2])
{
	cTF->RemoveAllPoints();
	cTF->AddRGBPoint(range[0], 0.0, 0.0, 0.0);
	cTF->AddRGBPoint(range[1], 1.0, 1.0, 1.0);
	cTF->Build();
}

void defaultOpacityTF(vtkSmartPointer<vtkPiecewiseFunction> pWF, double const range[2], bool opacityRamp)
{
	pWF->RemoveAllPoints();
	pWF->AddPoint(range[0], opacityRamp ? 0.0 : 1.0);
	pWF->AddPoint(range[1], 1.0);
}

vtkSmartPointer<vtkColorTransferFunction> defaultColorTF(double const range[2])
{
	auto cTF = vtkSmartPointer<vtkColorTransferFunction>::New();
	defaultColorTF(cTF, range);
	return cTF;
}

vtkSmartPointer<vtkPiecewiseFunction> defaultOpacityTF(double const range[2], bool opacityRamp)
{
	auto pWF = vtkSmartPointer<vtkPiecewiseFunction>::New();
	defaultOpacityTF(pWF, range, opacityRamp);
	return pWF;
}

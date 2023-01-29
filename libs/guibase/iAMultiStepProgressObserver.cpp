// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMultiStepProgressObserver.h"

#include <vtkAlgorithm.h>

iAMultiStepProgressObserver::iAMultiStepProgressObserver(double overallSteps) :
	m_currentStep(0),
	m_overallSteps(overallSteps)
{}

void iAMultiStepProgressObserver::setCompletedSteps(int steps)
{
	m_currentStep = steps;
}

iAProgress* iAMultiStepProgressObserver::progressObject()
{
	return &m_progress;
}

void iAMultiStepProgressObserver::observe(vtkAlgorithm* caller)
{
	caller->AddObserver(vtkCommand::ProgressEvent, this);
}

void iAMultiStepProgressObserver::Execute(vtkObject *caller, unsigned long, void*)
{
	m_progress.emitProgress((m_currentStep + ((vtkAlgorithm*)caller)->GetProgress()) * 100.0 / m_overallSteps);
}

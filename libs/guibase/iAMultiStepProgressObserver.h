// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include "iAProgress.h"

#include <vtkCommand.h>

#include <QObject>

//! Enables Observing the progress of multiple vtk algorithms executed sequentially via signals.
//! @todo should be merged/consolidated with iAProgress!
class iAguibase_API iAMultiStepProgressObserver : public QObject, public vtkCommand
{
	Q_OBJECT
public:
	iAMultiStepProgressObserver(double overallSteps);
	void setCompletedSteps(int steps);
	void observe(vtkAlgorithm* caller);
	iAProgress* progressObject();
private:
	virtual void Execute(vtkObject *caller, unsigned long, void*);
	double m_currentStep;
	double m_overallSteps;
	iAProgress m_progress;
};

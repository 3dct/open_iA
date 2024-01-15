// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

#include <itkCommand.h>
#include <vtkSmartPointer.h>

#include <QObject>

class iAProgress;

//! Helper class for forwarding progress events in ITK (where the observer pattern is used) to an iAProgress object.
//! @todo hide from iAProgress users (maybe via PIMPL idiom)
class iAitkCommand : public itk::Command
{
public:
	using Self = iAitkCommand;
	using Superclass = itk::Object;
	using Pointer = itk::SmartPointer<Self>;
	itkNewMacro(iAitkCommand);
	void setProgress(iAProgress const* progress);
	void Execute(itk::Object* caller, const itk::EventObject& event) override;
	void Execute(const itk::Object* caller, const itk::EventObject& event) override;
private:
	iAProgress const* m_progress;
};

class iAvtkCommand;

class vtkAlgorithm;

//! Connects computation with progress listeners through signals.
//! Can be used to track progress of vtk and itk filters,
//! and provides an interface for manual progress tracking.
class iAbase_API iAProgress : public QObject
{
	Q_OBJECT
public:
	//! observe an ITK algorithm (and pass on its progress report)
	//! @param caller the ITK algorithm to observe
	void observe( itk::Object *caller ) const;
	//! observe a VTK algorithm (and pass on its progress report)
	//! @param caller the VTK algorithm to observe
	void observe( vtkAlgorithm* caller ) const;

public slots:
	//! Trigger a progress event manually.
	//! @param p the current percentage of progress (number between 0 and 100)
	void emitProgress(double p) const;
	//! Set additional status information.
	//! @param status the new status to report to the user
	void setStatus(QString const & status) const;

signals:
	//! Signal emitted whenever the progress has changed.
	//! Connect this to a method that updates the indication of the current progression to the user.
	//! @param p the current percentage of progress (number between 0 and 100)
	void progress(double p) const;
	//! Signal emitted whenever the status has changed.
	//! Connect this to a method that updates the output of the status to the user.
	//! @param status the new status.
	void statusChanged(QString const & status) const;

private:
	mutable itk::SmartPointer<iAitkCommand> m_itkCommand;
	mutable vtkSmartPointer<iAvtkCommand> m_vtkCommand;
};

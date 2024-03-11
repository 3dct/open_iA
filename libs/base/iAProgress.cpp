// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAProgress.h"

#include <vtkAlgorithm.h>
#include <vtkCommand.h>

#include <itkProcessObject.h>

//! Helper class for forwarding progress events in VTK (where the observer pattern is used) to an iAProgress object.
class iAvtkCommand : public vtkCommand
{
public:
	static iAvtkCommand * New()
	{
		return new iAvtkCommand();
	}
	void setProgress(iAProgress const * progress)
	{
		m_progress = progress;
	}
	void Execute(vtkObject* caller, unsigned long, void*) override
	{
		m_progress->emitProgress((dynamic_cast<vtkAlgorithm*>(caller))->GetProgress() * 100);
	}
private:
	iAProgress const * m_progress = nullptr;
};

#include <itkMacro.h>

void iAitkCommand::setProgress(iAProgress const* progress)
{
	m_progress = progress;
}
void iAitkCommand::Execute(itk::Object* caller, const itk::EventObject& event)
{
	if (typeid(event) != typeid(itk::ProgressEvent))
	{
		return;
	}
	auto process = dynamic_cast<itk::ProcessObject*>(caller);
	m_progress->emitProgress(process->GetProgress() * 100.0);
}

void iAitkCommand::Execute(const itk::Object* caller, const itk::EventObject& event)
{
	if (typeid(event) != typeid(itk::ProgressEvent))
	{
		return;
	}
	auto process = dynamic_cast<const itk::ProcessObject*>(caller);
	m_progress->emitProgress(process->GetProgress() * 10.0);
}

void iAProgress::observe( itk::Object *caller ) const
{
	if (!m_itkCommand)
	{
		m_itkCommand = iAitkCommand::New();
		m_itkCommand->setProgress(this);
	}
	caller->AddObserver(  itk::ProgressEvent(), m_itkCommand.GetPointer() );
}

void iAProgress::observe(vtkAlgorithm* caller) const
{
	if (!m_vtkCommand)
	{
		m_vtkCommand = vtkSmartPointer<iAvtkCommand>::New();
		m_vtkCommand->setProgress(this);
	}
	caller->AddObserver(vtkCommand::ProgressEvent, m_vtkCommand);
}

void iAProgress::emitProgress(double p) const
{
	emit progress(p);
}

void iAProgress::setStatus(QString const & status) const
{
	emit statusChanged(status);
}

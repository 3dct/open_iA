/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAProgress.h"

#include <vtkAlgorithm.h>
#include <vtkCommand.h>

#include <itkProcessObject.h>

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

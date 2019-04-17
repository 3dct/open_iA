/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
	void SetProgress(iAProgress* progress)
	{
		m_progress = progress;
	}
	void Execute(vtkObject* caller, unsigned long, void*)
	{
		m_progress->EmitProgress((dynamic_cast<vtkAlgorithm*>(caller))->GetProgress() * 100);
	}
private:
	iAProgress* m_progress;
};

void iAProgress::ProcessEvent( itk::Object * caller, const itk::EventObject & event )
{
	if (typeid(event) != typeid(itk::ProgressEvent))
		return;
	auto process = dynamic_cast<itk::ProcessObject *>(caller);
	EmitProgress(static_cast<int>(process->GetProgress() * 100));
}

void iAProgress::ConstProcessEvent(const itk::Object * caller, const itk::EventObject & event)
{
	if (typeid(event) != typeid(itk::ProgressEvent))
		return;
	auto process = dynamic_cast<const itk::ProcessObject *>(caller);
	EmitProgress(static_cast<int>(process->GetProgress() * 100));
}

void iAProgress::Observe( itk::Object *caller )
{
	if (!m_itkCommand)
	{
		m_itkCommand = CommandType::New();
		m_itkCommand->SetCallbackFunction(this, &iAProgress::ProcessEvent);
		m_itkCommand->SetCallbackFunction(this, &iAProgress::ConstProcessEvent);
	}
	caller->AddObserver(  itk::ProgressEvent(), m_itkCommand.GetPointer() );
}

void iAProgress::Observe(vtkAlgorithm* caller)
{
	if (!m_vtkCommand)
	{
		m_vtkCommand = vtkSmartPointer<iAvtkCommand>::New();
		m_vtkCommand->SetProgress(this);
	}
	caller->AddObserver(vtkCommand::ProgressEvent, m_vtkCommand);
}

void iAProgress::EmitProgress(int i)
{
	emit progress(i);
}

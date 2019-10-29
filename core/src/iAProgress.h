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
#pragma once

#include "open_iA_Core_export.h"

#include <itkCommand.h>

#include <vtkSmartPointer.h>

#include <QObject>

class iAvtkCommand;

class vtkAlgorithm;

//! Connects computation with progress listeners through signals.
//! Can be used to track progress of vtk and itk filters,
//! and provides an interface for manual progress tracking.
class open_iA_Core_API iAProgress : public QObject
{
	Q_OBJECT
public:
	typedef itk::MemberCommand< iAProgress >  CommandType;
	//! @{
	//! Event handlers for ITK progress events
	void processEvent(itk::Object * caller, const itk::EventObject & event );
	void constProcessEvent(const itk::Object * caller, const itk::EventObject & event );
	//! @}
	//! observe an ITK algorithm (and pass on its progress report)
	//! @param caller the ITK algorithm to observe
	void observe( itk::Object *caller );
	//! observe a VTK algorithm (and pass on its progress report)
	//! @param caller the VTK algorithm to observe
	void observe( vtkAlgorithm* caller );
	//! Trigger a progress event manually.
	//! @param p the current percentage of progress (number between 0 and 100)
	void emitProgress(int p);
	//! Set additional status information.
	//! @param status the new status to report to the user
	void setStatus(QString const & status);
Q_SIGNALS:
	//! Signal emitted whenever the progress has changed.
	//! Connect this to a method that updates the indication of the current progression to the user.
	//! @param p the current percentage of progress (number between 0 and 100)
	void progress(int p);
	//! Signal emitted whenever the status has changed.
	//! Connect this to a method that updates the output of the status to the user.
	//! @param status the new status.
	void statusChanged(QString const & status);
private:
	CommandType::Pointer m_itkCommand;
	vtkSmartPointer<iAvtkCommand> m_vtkCommand;
};

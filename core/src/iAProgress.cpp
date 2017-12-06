/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
 
#include "pch.h"
#include "iAProgress.h"

#include <itkProcessObject.h>

iAProgress::iAProgress( )
{
	m_Command = CommandType::New();
	m_Command->SetCallbackFunction( this, &iAProgress::ProcessEvent );
	m_Command->SetCallbackFunction( this, &iAProgress::ConstProcessEvent );
}


void iAProgress::ProcessEvent( itk::Object * caller, const itk::EventObject & event )
{
	if( typeid( itk::ProgressEvent )   ==  typeid( event ) )
	{
		::itk::ProcessObject::Pointer  process = dynamic_cast< itk::ProcessObject *>( caller );
		const int value = static_cast<int>( process->GetProgress() * 100 );
		emit pprogress( value );
	}
}


void iAProgress::ConstProcessEvent( const itk::Object * caller, const itk::EventObject & event )
{
	if( typeid( itk::ProgressEvent )   ==  typeid( event ) ) 
	{
		itk::ProcessObject::ConstPointer  process = dynamic_cast< const itk::ProcessObject *>( caller );
		const int value = static_cast<int>( process->GetProgress() * 100 );
		emit pprogress( value );
	}
}

void iAProgress::Observe( itk::Object *caller )
{
	caller->AddObserver(  itk::ProgressEvent(), m_Command.GetPointer() );
}

void iAProgress::ManualProgress(int i)
{
	emit pprogress(i);
}

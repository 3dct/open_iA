// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVisModule.h"

#include <vtkRenderer.h>

iAVisModule::iAVisModule( )
	: m_enabled( false )
{ /* not implemented */ }

bool iAVisModule::isAttached( )
{
	return m_renderer != nullptr;
}

void iAVisModule::attachTo( vtkRenderer* renderer )
{
	if( renderer == nullptr ) return;
	m_renderer = renderer;
}

void iAVisModule::detach( )
{
	m_renderer = nullptr;
}

void iAVisModule::reset( )
{ /* not implemented */ }

void iAVisModule::enable( )
{
	if( !m_enabled && isAttached( ) )
	{
		show( );
		m_enabled = true;
	}
}

void iAVisModule::disable( )
{
	if( m_enabled && isAttached( ) )
	{
		hide( );
		m_enabled = false;
	}
}

bool iAVisModule::isEnabled( )
{
	return m_enabled;
}

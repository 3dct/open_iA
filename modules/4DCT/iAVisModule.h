// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class vtkRenderer;

class iAVisModule
{
public:
					iAVisModule( );
	void			attachTo( vtkRenderer* renderer );
	void			detach( );
	bool			isAttached( );
	bool			isEnabled( );
	virtual void	reset( );
	virtual void	enable( );
	virtual void	disable( );

protected:
	vtkRenderer *	m_renderer;

private:
	virtual void	show( ) = 0;
	virtual void	hide( ) = 0;
	bool			m_enabled;
};

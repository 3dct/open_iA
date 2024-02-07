// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAGUIModuleInterface.h"

class vtkImageData;
class dlg_densityMap;
class iA4DCTMainWin;

class iA4DCTModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT

public:
						iA4DCTModuleInterface( );
						~iA4DCTModuleInterface( );
	void				Initialize( ) override;

private slots:
	void				openProj( );
	void				newProj( );
	void				saveProj( );
	void				extractFeaturesToFile();
	void				defectClassification();

private:

	vtkImageData*		m_fracture;
	int					m_labeImageChildInd;
	dlg_densityMap*		m_densityMap;
};

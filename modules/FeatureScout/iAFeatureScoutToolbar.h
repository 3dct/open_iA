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
#pragma once

#include "FeatureScout_export.h"

#include <QToolBar>

#include <memory>

class iAMainWindow;
class iAMdiChild;
class Ui_FeatureScoutToolBar;

//! Manages the FeatureScout toolbar.
class FeatureScout_API iAFeatureScoutToolbar : public QToolBar
{
	Q_OBJECT
public:
	//! add child for which a FeatureScout toolbar should be available.
	static void addForChild(iAMainWindow* mainWnd, iAMdiChild* child);
	//! @{ disable copying/moving.
	void operator=(const iAFeatureScoutToolbar&) = delete;
	iAFeatureScoutToolbar(const iAFeatureScoutToolbar&) = delete;
	//! @}
private slots:
	void buttonClicked();
	void childClosed();
	void childChanged();
private:
	explicit iAFeatureScoutToolbar(iAMainWindow* mainWnd);
	~iAFeatureScoutToolbar() override; // required for std::unique_ptr
	static iAFeatureScoutToolbar* tlbFeatureScout;
	std::unique_ptr<Ui_FeatureScoutToolBar> m_ui;
	iAMainWindow* m_mainWnd;
};

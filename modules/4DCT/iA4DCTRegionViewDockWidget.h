/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/

#ifndef IA4DCTREGIONVIEWDOCKWIDGET_H
#define IA4DCTREGIONVIEWDOCKWIDGET_H
// Ui
#include "ui_iA4DCTRegionViewDockWidget.h"
// Qt
#include <QDockWidget>

class iARegionVisModule;

class iA4DCTRegionViewDockWidget : public QDockWidget, public Ui::RegionViewDockWidget
{
	Q_OBJECT
public:
				iA4DCTRegionViewDockWidget( QWidget * parent );
	void		attachTo( iARegionVisModule * visModule );

signals:
	void		updateRenderWindow( );

private:
	iARegionVisModule *		m_visModule;

private slots:
	void		onSilhoetteWidthChanged( int val );
	void		onSilhoetteOpacityChanged( int val );
	void		onSurfaceOpacityChanged( int val );
	void		onSilhoetteColorChanged( const QColor & col );
	void		onSurfaceColorChanged( const QColor & col );
};

#endif // IA4DCTREGIONVIEWDOCKWIDGET_H
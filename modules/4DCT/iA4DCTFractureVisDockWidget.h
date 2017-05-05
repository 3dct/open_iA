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

#ifndef IA4DCTFRACTUREVISDOCKWIDGET_H
#define IA4DCTFRACTUREVISDOCKWIDGET_H
// Ui
#include "ui_iA4DCTFractureVisDockWidget.h"
// iA
#include "iA4DCTData.h"
// QT
#include <QDockWidget>
#include <QColor>

class iAFractureVisModule;

class iA4DCTFractureVisDockWidget : public QDockWidget, public Ui::FractureVisDockWidget
{
	Q_OBJECT
public:
				iA4DCTFractureVisDockWidget( QWidget * parent );
	void		attachTo( iAFractureVisModule * visModule );
	void		setData( iA4DCTData * data );

private slots:
	void		onSaveButtonClicked( );
	void		onColorizeButtonClicked( );
	void		onLowIntensityColorChanged( const QColor & color );
	void		onHighIntensityColorChanged( const QColor & color );
	void		onColorChanged( const QColor & color );
	void		onAmbientValueChanged( int val );
	void		onOpacityValueChanged( int val );

signals:
	void		updateRenderWindow( );

private:
	iAFractureVisModule *		m_visModule;
	iA4DCTData *				m_data;
};

#endif // IA4DCTFRACTUREVISDOCKWIDGET_H
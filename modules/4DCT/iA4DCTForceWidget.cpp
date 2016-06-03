/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iA4DCTForceWidget.h"
// Ui
#include "ui_iA4DCTForceDialog.h"
// Qt
#include <QDialog>

iA4DCTForceWidget::iA4DCTForceWidget( QWidget * parent )
	: QLabel( parent )
{ /* not implemented */ }

iA4DCTForceWidget::~iA4DCTForceWidget()
{ /* not implemented */ }

void iA4DCTForceWidget::setValue( int val )
{
	m_value = val;
	this->setText( QString::number( m_value ) );
}

int iA4DCTForceWidget::getValue()
{
	return m_value;
}

void iA4DCTForceWidget::mouseDoubleClickEvent( QMouseEvent * event )
{
	QDialog * dialog = new QDialog ( this );
	Ui::ForceDialog dialogUi;
	dialogUi.setupUi( dialog );
	dialogUi.spinBox->setValue( m_value );
	if( dialog->exec() == QDialog::Accepted )
	{
		int newValue = dialogUi.spinBox->value( );
		setValue( newValue );
		emit valueChanged( newValue );
	}
	delete dialog;
}

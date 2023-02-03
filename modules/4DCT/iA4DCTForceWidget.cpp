// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTForceWidget.h"

#include "ui_iA4DCTForceDialog.h"

#include <QDialog>

iA4DCTForceWidget::iA4DCTForceWidget( QWidget * parent )
	: QLabel( parent )
{ /* not implemented */ }

void iA4DCTForceWidget::setValue( int val )
{
	m_value = val;
	this->setText( QString::number( m_value ) );
}

int iA4DCTForceWidget::getValue( )
{
	return m_value;
}

void iA4DCTForceWidget::mouseDoubleClickEvent(QMouseEvent * /*event*/)
{
	QDialog dialog( this );
	Ui::ForceDialog dialogUi;
	dialogUi.setupUi( &dialog );
	dialogUi.spinBox->setValue( m_value );
	if( dialog.exec( ) == QDialog::Accepted )
	{
		int newValue = dialogUi.spinBox->value( );
		setValue( newValue );
		emit valueChanged( newValue );
	}
}

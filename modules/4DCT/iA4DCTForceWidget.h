// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QLabel>

class iA4DCTForceWidget : public QLabel
{
	Q_OBJECT
public:
	iA4DCTForceWidget( QWidget * parent );
	void setValue( int val );
	int getValue( );

signals:
	void valueChanged( int val );

protected:
	void mouseDoubleClickEvent( QMouseEvent * event ) override;

private:
	int m_value;
};

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QColor>
#include <QColorDialog>

class iAColorBox : public QWidget
{
	Q_OBJECT
public:
					iAColorBox( QWidget * parent );
	void			setColor( QColor col );
	QColor			getColor( );

private:
	QColorDialog	m_dialog;
	QColor			m_color;

signals:
	void			colorChanged( const QColor & col );

protected:
	void			mouseDoubleClickEvent( QMouseEvent * event ) override;
	void			paintEvent( QPaintEvent * ) override;

private slots:
	void			onCurrentColorChanged( const QColor & col );
};

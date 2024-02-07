// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QLabel>

class iAPreview : public QLabel
{
	Q_OBJECT

public:
				iAPreview( QWidget* parent = 0 );
				~iAPreview( );
	QLabel*		getBigPreview( );

protected:
	void		mousePressEvent( QMouseEvent* event );
	void		mouseReleaseEvent( QMouseEvent* event );

private:
	QLabel*		m_smallPreview;
	QLabel*		m_bigPreview;
};

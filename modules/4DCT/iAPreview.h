// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QLabel>

class iAPreview : public QLabel
{
	Q_OBJECT

public:
				iAPreview( QWidget* parent = nullptr );
				~iAPreview( );
	QLabel*		getBigPreview( );

protected:
	void		mousePressEvent( QMouseEvent* event ) override;
	void		mouseReleaseEvent( QMouseEvent* event ) override;

private:
	QLabel*		m_smallPreview;
	QLabel*		m_bigPreview;
};

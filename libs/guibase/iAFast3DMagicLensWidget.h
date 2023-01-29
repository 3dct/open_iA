// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "iAAbstractMagicLensWidget.h"

#include "iAguibase_export.h"

class iAguibase_API iAFast3DMagicLensWidget : public iAAbstractMagicLensWidget
{
	Q_OBJECT
public:
	iAFast3DMagicLensWidget( QWidget * parent = 0 );
	~iAFast3DMagicLensWidget( );
protected:
	void updateLens( ) override;
	void resizeEvent( QResizeEvent * event ) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
private:
	double m_viewAngle;
	double calculateZ( double viewAngle );
signals:
	void rightButtonReleasedSignal();
	void leftButtonReleasedSignal();
};

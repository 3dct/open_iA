// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDockWidget>

class iAMdiChild;
class iATripleModalityWidget;
class iABimodalWidget;

class dlg_tf_2mod : public QDockWidget
{
	Q_OBJECT

public:
	dlg_tf_2mod(iAMdiChild* parent);

private:
	iAMdiChild *m_mdiChild;
	iABimodalWidget *m_bimodalWidget;
};

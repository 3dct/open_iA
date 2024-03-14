// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QDialog>
#include <QString>

class iACommonInput : public QDialog
{
public:
				iACommonInput(QWidget* parent);
				~iACommonInput();
	QString		openFile(QString caption, QString filter);
	QString		saveFile(QString caption, QString filter);

protected:
	QString		m_lastDir;
};

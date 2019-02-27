/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#pragma once

#include "open_iA_Core_export.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

class open_iA_Core_API iAFileChooserWidget : public QWidget
{
	Q_OBJECT
signals:
	void fileNameChanged(QString const & fileName);
public:
	enum ChoiceType
	{
		FileNameSave,
		FileNameOpen,
		FileNamesOpen,
		Folder
	};
	iAFileChooserWidget(QWidget* parent, ChoiceType type):
		m_textEdit(new QLineEdit()),
		m_browseButton(new QPushButton("...")),
		m_choiceType(type)
	{
		setLayout(new QHBoxLayout);
		layout()->setSpacing(0);
		setContentsMargins(0, 0, 0, 0);
		layout()->setContentsMargins(0, 0, 0, 0);
		layout()->addWidget(m_textEdit);
		layout()->addWidget(m_browseButton);
		m_browseButton->setFixedHeight(16);
		connect(m_browseButton, &QPushButton::clicked, this, &iAFileChooserWidget::BrowseClicked);
		connect(m_textEdit, &QLineEdit::editingFinished, this, &iAFileChooserWidget::emitChangedFileName);
	}
	void BrowseClicked()
	{
		QString choice;
		switch (m_choiceType)
		{
			case FileNameOpen: choice  = QFileDialog::getOpenFileName(this, "Open File", QFileInfo(m_textEdit->text()).absolutePath());	break;
			case FileNamesOpen: choice = "\""+QFileDialog::getOpenFileNames(this, "Open File", QFileInfo(m_textEdit->text().split(" ")[0]).absolutePath()).join("\" \"")+"\"";	break;
			case FileNameSave: choice  = QFileDialog::getSaveFileName(this, "Save File", QFileInfo(m_textEdit->text()).absolutePath());	break;
			default:
			case Folder: choice = QFileDialog::getExistingDirectory(this, "Choose Folder", m_textEdit->text());	break;
		}
		if (!choice.isEmpty())
		{
			m_textEdit->setText(choice);
			emit fileNameChanged(choice);
		}
	}
	QString text() const
	{
		return m_textEdit->text();
	}
	void setText(QString const & text)
	{
		m_textEdit->setText(text);
	}
	QLineEdit* m_textEdit;
	QPushButton* m_browseButton;
	ChoiceType m_choiceType;
private slots:
	void emitChangedFileName()
	{
		emit fileNameChanged(m_textEdit->text());
	}
};

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QWidget>

//! A widget for selection of input/output file(s), or folder, combining a text input and a browse button.
class iAguibase_API iAFileChooserWidget : public QWidget
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
		QWidget(parent),
		m_textEdit(new QLineEdit()),
		m_browseButton(new QToolButton()),
		m_choiceType(type)
	{
		m_browseButton->setText("...");
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		setLayout(new QHBoxLayout);
		layout()->setSpacing(0);
		setContentsMargins(0, 0, 0, 0);
		layout()->setContentsMargins(0, 0, 0, 0);
		layout()->setAlignment(Qt::AlignCenter);
		layout()->addWidget(m_textEdit);
		layout()->addWidget(m_browseButton);
		m_browseButton->setStyleSheet("min-height: 1.2em;");
		connect(m_browseButton, &QToolButton::clicked, this, &iAFileChooserWidget::BrowseClicked);
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
	QToolButton* m_browseButton;
	ChoiceType m_choiceType;
private slots:
	void emitChangedFileName()
	{
		emit fileNameChanged(m_textEdit->text());
	}
};

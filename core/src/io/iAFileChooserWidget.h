#pragma once

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

class iAFileChooserWidget : public QWidget
{
	Q_OBJECT
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
			m_textEdit->setText(choice);
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
};

#include "iASetPathWidget.h"

#include <QFileDialog>
#include <QDir>
#include <QSettings>

iASetPathWidget::iASetPathWidget(QWidget* parent/* = 0*/)
	: QWidget(parent)
{
	ui.setupUi(this);
	connect(ui.Browse, SIGNAL(clicked()), SLOT(onBrowseButtonClicked()));
}

iASetPathWidget::~iASetPathWidget()
{ /* not implemented yet */ }

void iASetPathWidget::setOptions(Mode mode, QString caption, QString filter, QString uniqueKey)
{
	m_mode = mode;
	m_caption = caption;
	m_filter = filter;
	m_uniqueKey = uniqueKey;

	QSettings settings;
	ui.Path->setText(settings.value(m_uniqueKey + m_valPostfix).toString());
}

void iASetPathWidget::onBrowseButtonClicked()
{
	QSettings settings;
	QString path;

	switch (m_mode)
	{
	case Mode::openFile:
		path = QFileDialog::getOpenFileName(this, m_caption, settings.value(m_uniqueKey + m_dirPostfix).toString(), m_filter);
		break;
	case Mode::saveFile:
		path = QFileDialog::getSaveFileName(this, m_caption, settings.value(m_uniqueKey + m_dirPostfix).toString(), m_filter);
		break;
	case Mode::directory:
		path = QFileDialog::getExistingDirectory(this, m_caption, settings.value(m_uniqueKey + m_dirPostfix).toString());
	default:
		break;
	}

	if (!path.isNull())
	{
		QDir dir(path);
		settings.setValue(m_uniqueKey + m_dirPostfix, dir.path());
		settings.setValue(m_uniqueKey + m_valPostfix, path);
	}

	ui.Path->setText(path);
}
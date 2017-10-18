#include "iACommonInput.h"
#include <QFileDialog>
#include <QDir>
#include <QSettings>

iACommonInput::iACommonInput(QWidget* parent /*= 0*/)
	: QDialog(parent)
{ /* not implemented */ }

iACommonInput::~iACommonInput()
{ /* not implemented */ }

QString iACommonInput::openFile( QString caption, QString filter )
{
	QSettings settings;
	QString path = QFileDialog::getOpenFileName(this, caption, settings.value(m_lastDir).toString(), filter);
	if(!path.isNull())
	{
		QDir dir(path);
		settings.setValue(m_lastDir, dir.path());
	}
	return path;
}

QString iACommonInput::saveFile( QString caption, QString filter )
{
	QSettings settings;
	QString path = QFileDialog::getSaveFileName(this, caption, settings.value(m_lastDir).toString(), filter);
	if(!path.isNull())
	{
		QDir dir(path);
		settings.setValue(m_lastDir, dir.path());
	}
	return path;
}

#ifndef COMMONINPUT_H
#define COMMONINPUT_H

// Qt
#include <QDialog>
#include <QString>

class iACommonInput : public QDialog
{
public:
				iACommonInput(QWidget* parent = 0);
				~iACommonInput();
	QString		openFile(QString caption, QString filter);
	QString		saveFile(QString caption, QString filter);

protected:
	QString		m_lastDir;
};

#endif // COMMONINPUT_H
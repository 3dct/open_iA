#ifndef SETPATH_H
#define SETPATH_H

#include "ui_iASetPathWidget.h"
#include <QWidget>

class iASetPathWidget : public QWidget
{
	Q_OBJECT
public:
	enum Mode { openFile, saveFile, directory };

				iASetPathWidget(QWidget* parent = 0);
				~iASetPathWidget();
	void		setOptions(Mode mode, QString caption, QString filter, QString uniqueKey);

	Ui::SetPathWidget	ui;

protected:
	Mode		m_mode;
	QString		m_caption;
	QString		m_filter;
	QString		m_uniqueKey;
	const QString m_dirPostfix = QString("_dir");
	const QString m_valPostfix = QString("_val");

private slots:
	void		onBrowseButtonClicked();
};

#endif // SETPATH_H
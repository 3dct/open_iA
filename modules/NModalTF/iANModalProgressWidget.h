// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMap>
#include <QThread>
#include <QWidget>

class QGridLayout;
class QProgressBar;
class QLabel;
class QLayout;

class iANModalProgressWidget : public QWidget
{
	Q_OBJECT

public:
	iANModalProgressWidget(QWidget* parent = nullptr);

	int addProgressBar(int max, QString title, bool autoUpdateText = true, QString name = QString());
	int addProgressBar(int max, QLabel* label, bool autoUpdateText = false, QString name = QString());

	void addWidget(QWidget* widget, QString title);
	void addWidget(QWidget* widget, QLabel* label = nullptr, int rowStretch = 1);

	void addSeparator();

	bool isFinished();
	bool isCanceled();
	void setCanceled(bool);
	bool exists(int pbid);

private:
	QGridLayout* m_layout;
	QList<QProgressBar*> m_bars;
	QList<QLabel*> m_barLabels;
	QList<QString> m_barTexts;
	QList<bool> m_barAutoUpdateText;
	QMap<QString, int> m_name2id;
	int m_nextRow = 0;

	bool m_canceled = false;

public slots:
	void showDialog(QWidget* parent = nullptr);
	void update(int pbid);
	void finish();
	void cancel();

	void setFirstValue(int value);

	void setValue(int pbid, int value);
	void setMax(int pbid, int max);
	void setAutoUpdateText(int pbid, bool autoUpdateText);
	void setText(int pbid, QString text);

	void setValue(QString name, int value);
	void setMax(QString name, int max);
	void setAutoUpdateText(QString name, bool autoUpdateText);
	void setText(QString name, QString text);

signals:
	void updated(int pbid, int value, int max);
	void finished();
	void canceled();
};

class iANModalProgressUpdater : public QThread
{
	Q_OBJECT

public:
	iANModalProgressUpdater(iANModalProgressWidget*);

private:
	static const auto CONNECTION_TYPE = Qt::BlockingQueuedConnection;

signals:
	void finish();
	void cancel();

	void showDialog(QWidget* parent = nullptr);

	void setFirstValue(int value);
	void setValue(int pbid, int value);
	void setMax(int pbid, int max);
	void setAutoUpdateText(int pbid, bool autoUpdateText);
	void setText(int pbid, QString text);

	void setValue(QString name, int value);
	void setMax(QString name, int max);
	void setAutoUpdateText(QString name, bool autoUpdateText);
	void setText(QString name, QString text);
};

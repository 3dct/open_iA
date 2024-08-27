// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAQWidgetSettingsMapper.h"

#include <iAFileChooserWidget.h>
#include <iALog.h>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

void loadSettings(QVariantMap const& settings, iAQWidgetMap const& settingsWidgetMap)
{
	for (QString key : settingsWidgetMap.keys())
	{
		if (settings.contains(key))
		{
			QObject* w = settingsWidgetMap[key];
			if (qobject_cast<QComboBox*>(w))
			{
				int idx = qobject_cast<QComboBox*>(w)->findText(settings.value(key).toString());
				if (idx != -1)
				{
					qobject_cast<QComboBox*>(w)->setCurrentIndex(idx);
				}
				else
				{	// warn, but not if the combobox is empty and loaded value is an empty string
					if (!settings.value(key).toString().isEmpty() || qobject_cast<QComboBox*>(w)->count() > 0)
					{
						LOG(lvlWarn,
							QString("Invalid value '%1' for input '%2'").arg(settings.value(key).toString()).arg(key));
					}
				}
			}
			else if (qobject_cast<QCheckBox*>(w))
			{
				qobject_cast<QCheckBox*>(w)->setChecked(settings.value(key).toBool());
			}
			else if (qobject_cast<QSlider*>(w))
			{
				bool ok;
				int value = settings.value(key).toInt(&ok);
				if (!ok)
				{
					LOG(lvlWarn, QString("Invalid value '%1' for input '%2': cannot convert to int!").arg(settings.value(key).toString()).arg(key));
					continue;
				}
				qobject_cast<QSlider*>(w)->setValue(value);
			}
			else if (qobject_cast<QDoubleSpinBox*>(w))
			{
				bool ok;
				double value = settings.value(key).toDouble(&ok);
				if (!ok)
				{
					LOG(lvlWarn, QString("Invalid value '%1' for input '%2': cannot convert to double!").arg(settings.value(key).toString()).arg(key));
					continue;
				}
				qobject_cast<QDoubleSpinBox*>(w)->setValue(value);
			}
			else if (qobject_cast<QSpinBox*>(w))
			{
				bool ok;
				int value = settings.value(key).toInt(&ok);
				if (!ok)
				{
					LOG(lvlWarn, QString("Invalid value '%1' for input '%2': cannot convert to int!").arg(settings.value(key).toString()).arg(key));
					continue;
				}
				qobject_cast<QSpinBox*>(w)->setValue(value);
			}
			else if (qobject_cast<QLineEdit*>(w))
			{
				qobject_cast<QLineEdit*>(w)->setText(settings[key].toString());
			}
			else if (qobject_cast<QPushButton*>(w))
			{
				qobject_cast<QPushButton*>(w)->setText(settings[key].toString());
			}
			else if (qobject_cast<iAFileChooserWidget*>(w))
			{
				// error check?
				qobject_cast<iAFileChooserWidget*>(w)->setText(settings[key].toString());
			}
			else if (dynamic_cast<iAQLineEditVector*>(w))
			{
				auto& lineEditVector = *dynamic_cast<iAQLineEditVector*>(w);
				QStringList values = settings.value(key).toString().split(",");
				if (values.size() != lineEditVector.size())
				{
					LOG(lvlWarn, QString("Invalid value '%1' for key=%2 - should be able to split that into %3 values, but encountered %4")
						.arg(settings.value(key).toString())
						.arg(key)
						.arg(lineEditVector.size())
						.arg(values.size()));
				}
				for (int i = 0; i < lineEditVector.size() && i < values.size(); ++i)
					lineEditVector[i]->setText(values[i]);
			}
			else if (dynamic_cast<iAQCheckBoxVector*>(w))
			{
				auto& checkBoxVector = *dynamic_cast<iAQCheckBoxVector*>(w);
				QStringList values = settings.value(key).toString().split(",");
				// first uncheck all entries:
				for (auto checkbox : checkBoxVector)
				{
					checkbox->setChecked(false);
				}
				QString fullStr = settings.value(key).toString();
				if (fullStr.isEmpty())
				{
					continue;
				}
				// then check those mentioned in settings:
				for (QString v : values)
				{
					bool ok;
					int idx = v.toInt(&ok);
					if (!ok || idx < 0 || idx > checkBoxVector.size())
					{
						LOG(lvlWarn, QString("Invalid value '%1' for key=%2; entry %3 is either not convertible to int or outside of valid range 0..%4.")
							.arg(settings.value(key).toString())
							.arg(key)
							.arg(idx)
							.arg(checkBoxVector.size()));
					}
					else
					{
						checkBoxVector[idx]->setChecked(true);
					}
				}
			}
			else if (dynamic_cast<iAQRadioButtonVector*>(w))
			{
				auto& radioButtonVector = *dynamic_cast<iAQRadioButtonVector*>(w);
				QString value = settings.value(key).toString();
				for (auto radioButton : radioButtonVector)
				{
					radioButton->setChecked(radioButton->text() == value);
				}
			}
			else
			{
				LOG(lvlWarn, QString("Widget type for key=%1 unknown!").arg(key));
			}
		}
		else
		{
			LOG(lvlWarn, QString("No value found for key=%1 in settings.").arg(key));
		}
	}
}

//! Interface for setting (key, value) pairs in a data collection.
class iAInternalSettingsWrapper
{
public:
	virtual void setValue(QString const& key, QVariant const& value) = 0;
};

//! Wrapper for setting (key, value) pairs in a QSettings object using the iAInternalSettingsWrapper interface.
class iAQSettingsWrapper: public iAInternalSettingsWrapper
{
public:
	iAQSettingsWrapper(QSettings& qs):
		m_qs(qs)
	{}
	void setValue(QString const& key, QVariant const& value) override
	{
		m_qs.setValue(key, value);
	}
private:
	QSettings & m_qs;
};

//! Wrapper for setting (key, value) pairs in a QVariantMap using the iAInternalSettingsWrapper interface.
class iASettingsWrapper : public iAInternalSettingsWrapper
{
public:
	iASettingsWrapper(QVariantMap& s):
		m_s(s)
	{}
	void setValue(QString const& key, QVariant const& value) override
	{
		m_s[key] = value;
	}
private:
	QVariantMap& m_s;
};

void internalSaveSettings(iAInternalSettingsWrapper& settings, iAQWidgetMap const& settingsWidgetMap)
{
	for (QString key : settingsWidgetMap.keys())
	{
		QObject* w = settingsWidgetMap[key];
		if (qobject_cast<QComboBox*>(w))
		{
			settings.setValue(key, qobject_cast<QComboBox*>(w)->currentText());
		}
		else if (qobject_cast<QCheckBox*>(w))
		{
			settings.setValue(key, qobject_cast<QCheckBox*>(w)->isChecked());
		}
		else if (qobject_cast<QSlider*>(w))
		{
			settings.setValue(key, qobject_cast<QSlider*>(w)->value());
		}
		else if (qobject_cast<QDoubleSpinBox*>(w))
		{
			settings.setValue(key, qobject_cast<QDoubleSpinBox*>(w)->value());
		}
		else if (qobject_cast<QSpinBox*>(w))
		{
			settings.setValue(key, qobject_cast<QSpinBox*>(w)->value());
		}
		else if (qobject_cast<QLineEdit*>(w))
		{
			settings.setValue(key, qobject_cast<QLineEdit*>(w)->text());
		}
		else if (qobject_cast<QPushButton*>(w))
		{
			settings.setValue(key, qobject_cast<QPushButton*>(w)->text());
		}
		else if (qobject_cast<iAFileChooserWidget*>(w))
		{
			settings.setValue(key, qobject_cast<iAFileChooserWidget*>(w)->text());
		}
		else if (dynamic_cast<iAQLineEditVector*>(w))
		{
			QStringList values;
			auto& list = *dynamic_cast<iAQLineEditVector*>(w);
			for (auto edit : list)
			{
				values << edit->text();
			}
			settings.setValue(key, values.join(","));
		}
		else if (dynamic_cast<iAQCheckBoxVector*>(w))
		{
			auto& list = *dynamic_cast<iAQCheckBoxVector*>(w);
			QStringList values;
			for (int i = 0; i < list.size(); ++i)
			{
				if (list[i]->isChecked())
				{
					values << QString::number(i);
				}
			}
			settings.setValue(key, values.join(","));
		}
		else if (dynamic_cast<iAQRadioButtonVector*>(w))
		{
			auto& list = *dynamic_cast<iAQRadioButtonVector*>(w);
			QString value;
			for (auto rb: list)
			{
				if (rb->isChecked())
				{
					value = rb->text();
					break;
				}
			}
			settings.setValue(key, value);
		}
		else
		{
			LOG(lvlWarn, QString("Widget type for key=%1 unknown!").arg(key));
		}
	}
}

void saveSettings(QSettings& settings, iAQWidgetMap const& settingsWidgetMap)
{
	iAQSettingsWrapper settingsWrapper(settings);
	internalSaveSettings(settingsWrapper, settingsWidgetMap);
}

void saveSettings(QVariantMap& settings, iAQWidgetMap const& settingsWidgetMap)
{
	iASettingsWrapper settingsWrapper(settings);
	internalSaveSettings(settingsWrapper, settingsWidgetMap);
}

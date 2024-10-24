// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAQDockTitleWidget.h"

#include <iAQWidgetHelper.h>

#include <QDesktopServices>
#include <QDockWidget>
#include <QFrame>
#include <QLabel>
#include <QPainter>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

// iAVerticalLabel

//! a label with vertical text orientation (from bottom to top)
//! currently only used in iAQDockTitleWidget
class iAVerticalLabel : public QLabel
{
public:
	explicit iAVerticalLabel(const QString& text);

protected:
	void paintEvent(QPaintEvent*) override;
	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;
};

iAVerticalLabel::iAVerticalLabel(const QString& text) : QLabel(text)
{
}

void iAVerticalLabel::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.translate(0, height());
	painter.rotate(270);
	QFont f(painter.font());
	f.setBold(true);
	painter.setFont(f);
#ifdef __linux__
	const int Offset = 2;   // for some reason, on Linux, text is placed too far to the right and the width is a little to small (text is cut off at bottom) -> use "manual" correction offset
#else
	const int Offset = 0;
#endif
	painter.drawText(0, -Offset, height(), width()+Offset, Qt::AlignTop, text());
}

QSize iAVerticalLabel::minimumSizeHint() const
{
	QSize s = QLabel::minimumSizeHint();
	return QSize(s.height(), 0);
}

QSize iAVerticalLabel::sizeHint() const
{
	QSize s = QLabel::sizeHint();
	return QSize(s.height(), s.width());
}

// iAQDockTitleWidget

iAQDockTitleWidget::iAQDockTitleWidget(QDockWidget* parent, QString infoLink) : QFrame(parent)
{
	setProperty("qssClass", "iAQDockTitleWidget");
	int mw = parent->style()->pixelMetric(QStyle::PM_DockWidgetTitleMargin, nullptr, parent);
	setLayout(createLayout<QVBoxLayout>(2, mw));
	auto closeButton = new QToolButton();
	closeButton->setProperty("qssClass", "dockwidget-close");
	closeButton->setMinimumSize(0, 10);
	connect(closeButton, &QToolButton::clicked, this, [this]()
	{
		QDockWidget* q = qobject_cast<QDockWidget*>(parentWidget());
		q->close();
	});
	auto floatButton = new QToolButton();
	floatButton->setProperty("qssClass", "dockwidget-float");
	floatButton->setMinimumSize(0, 10);
	connect(floatButton, &QToolButton::clicked, this, [this]()
	{
		QDockWidget* q = qobject_cast<QDockWidget*>(parentWidget());
		q->setFloating(!q->isFloating());
	});
	auto infoButton = new QToolButton();
	infoButton->setProperty("qssClass", "dockwidget-info");
	infoButton->setMinimumSize(0, 10);
	connect(infoButton, &QToolButton::clicked, this, [infoLink]() { QDesktopServices::openUrl(QUrl(infoLink)); });
	layout()->addWidget(closeButton);
	layout()->addWidget(floatButton);
	layout()->addWidget(infoButton);
	layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
	layout()->addWidget(new iAVerticalLabel(parent->windowTitle()));
}

QSize iAQDockTitleWidget::sizeHint() const
{
	QDockWidget* q = qobject_cast<QDockWidget*>(parentWidget());
	QFontMetrics titleFontMetrics = q->fontMetrics();
	int mw = q->style()->pixelMetric(QStyle::PM_DockWidgetTitleMargin, nullptr, q);
	auto h = geometry().height();
	auto w = titleFontMetrics.height() + 2 * mw;
	return QSize(w, h);
}

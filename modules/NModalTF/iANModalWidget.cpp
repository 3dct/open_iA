#include "iANModalWidget.h"

#include "iAConsole.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <QHBoxLayout>


// Module interface and Attachment --------------------------------------------------------

iANModalWidgetAttachment::iANModalWidgetAttachment(MainWindow * mainWnd, MdiChild *child) :
	iAModuleAttachmentToChild(mainWnd, child),
	m_nModalWidget(nullptr)
{
	// Do nothing
}

iANModalWidgetAttachment* iANModalWidgetAttachment::create(MainWindow * mainWnd, MdiChild *child) {
	auto newAttachment = new iANModalWidgetAttachment(mainWnd, child);
	return newAttachment;
}

void iANModalWidgetAttachment::start() {
	if (!m_nModalWidget) {
		m_nModalWidget = new iANModalWidget(m_child);
		m_child->tabifyDockWidget(m_child->logDockWidget(), m_nModalWidget);
	}
	m_nModalWidget->show();
	m_nModalWidget->raise();
}

void iANModalWidgetModuleInterface::Initialize() {
	if (!m_mainWnd) // if m_mainWnd is not set, we are running in command line mode
		return;     // in that case, we do not do anything as we can not add a menu entry there
	QMenu *toolsMenu = m_mainWnd->toolsMenu();
	QMenu *menuMultiModalChannel = getMenuWithTitle(toolsMenu, QString("Multi-Modal/-Channel Images"), false);

	QAction *action = new QAction(m_mainWnd);
	action->setText(QApplication::translate("MainWindow", "n-Modal Transfer Function", 0));
	AddActionToMenuAlphabeticallySorted(menuMultiModalChannel, action, true);
	connect(action, SIGNAL(triggered()), this, SLOT(onMenuItemSelected()));
}

iAModuleAttachmentToChild* iANModalWidgetModuleInterface::CreateAttachment(MainWindow* mainWnd, MdiChild *childData) {
	return iANModalWidgetAttachment::create(mainWnd, childData);
}

void iANModalWidgetModuleInterface::onMenuItemSelected() {
	PrepareActiveChild();
	auto attach = GetAttachment<iANModalWidgetAttachment>();
	if (!attach)
	{
		AttachToMdiChild(m_mdiChild);
		attach = GetAttachment<iANModalWidgetAttachment>();
		if (!attach)
		{
			DEBUG_LOG("Attaching failed!");
			return;
		}
	}
	attach->start();
}


// n-Modal Widget -------------------------------------------------------------------------

iANModalWidget::iANModalWidget(MdiChild *mdiChild):
	QDockWidget("n-Modal Transfer Function", mdiChild)
{
	QHBoxLayout *layout = new QHBoxLayout(this);


}
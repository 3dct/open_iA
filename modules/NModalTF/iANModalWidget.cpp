#include "iANModalWidget.h"

#include "iAConsole.h"
#include "mainwindow.h"
#include "mdichild.h"


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
	auto attach = GetAttachment<iANModalWidgetAttachment>(m_mdiChild);
	if (!attach)
	{
		AttachToMdiChild(m_mdiChild);
		attach = GetAttachment<iANModalWidgetAttachment>(m_mdiChild);
		if (!attach)
		{
			DEBUG_LOG("Attaching failed!");
			return;
		}
	}
	attach->start();
}


// n-Modal Widget -------------------------------------------------------------------------

#include "iAModality.h"
#include "iAModalityTransfer.h"

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSharedPointer>

// TEMPORARY
#include <QStandardItemModel>
#include <QObjectList>
#include <QColor>
#include <QStandardItem>

iANModalWidget::iANModalWidget(MdiChild *mdiChild):
	QDockWidget("n-Modal Transfer Function", mdiChild)
{
	m_mdiChild = mdiChild;
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetVerticalTitleBar);

	m_label = new QLabel("n-Modal Transfer Function");

	QPushButton *button = new QPushButton("Click me!");

	QHBoxLayout *layout = new QHBoxLayout();
	layout->addWidget(m_label);
	layout->addWidget(button);

	QWidget *widget = new QWidget();
	widget->setLayout(layout);

	setWidget(widget);

	// Connect
	connect(button, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
}

void iANModalWidget::onButtonClicked() {
	//m_label->setText("Clicked");
	adjustTf();
}

void iANModalWidget::adjustTf() {

	QSharedPointer<iAModality> modality = m_mdiChild->modality(0);
	vtkSmartPointer<vtkImageData> image = modality->image();
	QSharedPointer<iAModalityTransfer> tf = modality->transfer();

	double range[2];
	image->GetScalarRange(range);
	double min = range[0];
	double max = range[1];

	QList<LabeledVoxel> voxels;

	{
		// Find with string "labels": TERRIBLE IDEA! TODO: improve
		//dlg_labels *labeling = m_mdiChild->findChild<dlg_labels*>("labels"); // Doesn't work... dunno why
		QObject *obj = m_mdiChild->findChild<QObject*>("labels");
		dlg_labels* labeling = static_cast<dlg_labels*>(obj);

		QString text = QString();

		QStandardItemModel *items = labeling->m_itemModel;
		for (int row = 0; row < items->rowCount(); row++) {
			QStandardItem *item = items->item(row, 0);

			if (row == 0) {
				item->setText("Remover");
				item->setData(QColor(0, 0, 0), Qt::DecorationRole);
			}

			QColor color = qvariant_cast<QColor>(item->data(Qt::DecorationRole));
			int count = items->item(row, 1)->text().toInt();
			for (int childRow = 0; childRow < item->rowCount(); childRow++) {
				QString t = item->child(childRow, 0)->text();
				QString nums = t.mid(1, t.size() - 2); // Remove parentheses
				QString nospace = nums.replace(" ", ""); // Remove spaces
				QStringList coords = nospace.split(","); // Separate by comma
				int x = coords[0].toInt();
				int y = coords[1].toInt();
				int z = coords[2].toInt();

				double scalar = image->GetScalarComponentAsDouble(x, y, z, 0);

				auto v = LabeledVoxel();
				v.x = x;
				v.y = y;
				v.z = z;
				v.scalar = scalar;
				v.r = color.redF();
				v.g = color.greenF();
				v.b = color.blueF();
				v.remover = (row == 0);

				voxels.append(v);

				text += v.text() + "\n";
			}
		}

		m_label->setText(text);
	}

	tf->colorTF()->RemoveAllPoints();
	tf->colorTF()->AddRGBPoint(min, 0.0, 0.0, 0.0);
	tf->colorTF()->AddRGBPoint(max, 0.0, 0.0, 0.0);

	tf->opacityTF()->RemoveAllPoints();
	tf->opacityTF()->AddPoint(min, 0.0);
	tf->opacityTF()->AddPoint(max, 0.0);

	for (int i = 0; i < voxels.size(); i++) {
		LabeledVoxel v = voxels[i];

		double opacity = v.remover ? 0.0 : (1.0 / voxels.size());

		tf->colorTF()->AddRGBPoint(v.scalar, v.r, v.g, v.b);
		tf->opacityTF()->AddPoint(v.scalar, opacity);
	}
}

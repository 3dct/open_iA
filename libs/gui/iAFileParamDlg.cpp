// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <iAFileParamDlg.h>

#include <iAFileUtils.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iAMathUtility.h>
#include <iASettings.h>
#include <iAValueTypeVectorHelpers.h>

#include <iAFileStackParams.h>
#include <iAImageStackFileIO.h>
#include <iARawFileIO.h>

#include <iAParameterDlg.h>
#include <iARawFileParamDlg.h>
#include <iAThemeHelper.h>

#include <vtkImageData.h>

#include <QRegularExpression>

namespace
{
	QString settingName(iAFileIO::Operation op, QString name)
	{
		return QString("File%1/%2")
			.arg(op == iAFileIO::Load ? "Load" : "Save")
			.arg(name.remove(QRegularExpression("[^a-zA-Z\\d]")));
	}
}

iAFileParamDlg::~iAFileParamDlg()
{}

bool iAFileParamDlg::askForParameters(QWidget* parent, iAAttributes const& parameters, QString const & ioName, QVariantMap& values, QString const& fileName, iADataSet const * dataSet) const
{
	Q_UNUSED(fileName);
	Q_UNUSED(dataSet);
	auto dlgParams = combineAttributesWithValues(parameters, values);
	auto it = std::find_if(dlgParams.begin(), dlgParams.end(), [](auto a) { return a->name() == iADataSet::FileNameKey; });
	if (it != dlgParams.end())
	{
		it->get()->setDefaultValue(fileName);
	}

	iAParameterDlg dlg(parent, QString("%1 parameters").arg(ioName), dlgParams);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}
	values = dlg.parameterValues();
	return true;
}

iAFileParamDlg* iAFileParamDlg::get(QString const& ioName)
{
	if (m_dialogs.contains(ioName))
	{
		return m_dialogs[ioName].get();
	}
	static auto defaultDialog = std::make_shared<iAFileParamDlg>();
	return defaultDialog.get();
}

void iAFileParamDlg::add(QString const& ioName, std::shared_ptr<iAFileParamDlg> dlg)
{
	m_dialogs.insert(ioName, dlg);
}

QMap<QString, std::shared_ptr<iAFileParamDlg>> iAFileParamDlg::m_dialogs;

bool iAFileParamDlg::getParameters(QWidget* parent, iAFileIO const* io, iAFileIO::Operation op, QString const& fileName, QVariantMap & paramValues, iADataSet const * dataSet)
{
	if (io->parameter(op).size() > 1 || (io->parameter(op).size() == 1 && io->parameter(op)[0]->name() != iADataSet::FileNameKey))
	{
		auto settingPath = settingName(op, io->name());
		paramValues = ::loadSettings(settingPath, extractValues(io->parameter(op)));
		auto paramDlg = iAFileParamDlg::get(settingPath);
		if (!paramDlg->askForParameters(parent, io->parameter(op), io->name(), paramValues, fileName, dataSet))
		{
			return false;
		}
		::storeSettings(settingPath, paramValues);
	}
	return true;
}

//! Dialog for retrieving parameters for the raw file I/O.
class iARawFileLoadParamDlg : public iAFileParamDlg
{
public:
	bool askForParameters(QWidget* parent, iAAttributes const& parameters, QString const& ioName, QVariantMap& values, QString const& fileName, iADataSet const * dataSet) const override
	{
		Q_UNUSED(parameters);    // iARawFileParamDlg knows which parameters to get
		Q_UNUSED(ioName);
		Q_UNUSED(dataSet);
		iARawFileParamDlg dlg(fileName, parent, "Raw file parameters", values, iAThemeHelper::brightMode());
		if (!dlg.accepted())
		{
			return false;
		}
		values = dlg.parameterValues();
		return true;
	}
};

//! Dialog for retrieving parameters for loading an image stack.
class iAImageStackLoadParamDlg: public iAFileParamDlg
{

public:
	bool askForParameters(QWidget* parent, iAAttributes const& parameters, QString const & ioName, QVariantMap& values, QString const& fileName, iADataSet const * dataSet) const override
	{
		Q_UNUSED(dataSet);
		QString base, suffix;
		int range[2];
		int digits;
		determineStackParameters(fileName, base, suffix, range, digits);
		values[iAImageStackFileIO::LoadTypeStr] = (digits == 0) ? iAImageStackFileIO::SingleImageOption : iAImageStackFileIO::ImageStackOption;
		values[iAFileStackParams::FileNameBase] = base;
		values[iAFileStackParams::Extension] = suffix;
		values[iAFileStackParams::NumDigits] = digits;
		values[iAFileStackParams::MinimumIndex] = range[0];
		values[iAFileStackParams::MaximumIndex] = range[1];
		return iAFileParamDlg::askForParameters(parent, parameters, ioName, values, fileName);
	}
};

//! Dialog for retrieving parameters for loading an image stack.
class iAImageStackSaveParamDlg : public iAFileParamDlg
{

public:
	bool askForParameters(QWidget* parent, iAAttributes const& parameters, QString const& ioName, QVariantMap& values, QString const& fileName, iADataSet const * dataSet) const override
	{
		auto imgDS = dynamic_cast<iAImageData const *>(dataSet);
		if (!imgDS)
		{
			return false;
		}
		auto extent = imgDS->vtkImage()->GetExtent();
		int axisIdx = iAImageStackFileIO::axisName2Idx(values[iAImageStackFileIO::AxisOption].toString());
		values[iAFileStackParams::MinimumIndex] = clamp(extent[axisIdx * 2], extent[axisIdx * 2 + 1], values[iAFileStackParams::MinimumIndex].toInt());
		values[iAFileStackParams::MaximumIndex] = clamp(extent[axisIdx * 2], extent[axisIdx * 2 + 1], values[iAFileStackParams::MaximumIndex].toInt());
		return iAFileParamDlg::askForParameters(parent, parameters, ioName, values, fileName);
	}
};

#ifdef USE_HDF5

#include <iAHDF5IO.h>

#include <QFileInfo>
#include <QStandardItem>
#include <iAWidgetAnimationDecorator.h>

namespace
{

	struct opdata {
		QStandardItem* item;
		unsigned       recurs;         /* Recursion level.  0=root */
		struct opdata* prev;          /* Pointer to previous opdata */
		haddr_t        addr;           /* Group address */
	};

	const int DATASET = 0;
	const int GROUP = 1;
	const int OTHER = 2;

	//! This function recursively searches the linked list of opdata structures
	//! for one whose address matches target_addr.  Returns 1 if a match is
	//! found, and 0 otherwise.
	int group_check(struct opdata* od, haddr_t target_addr)
	{
		if (od->addr == target_addr)
		{
			return 1;       /* Addresses match */
		}
		else if (!od->recurs)
		{
			return 0;       /* Root group reached with no matches */
		}
		else
		{
			return group_check(od->prev, target_addr);
		}
		/* Recursively examine the next node */
	}

	herr_t op_func(hid_t loc_id, const char* name, const H5L_info_t* /*info*/,
		void* operator_data)
	{
		herr_t          status, return_val = 0;
		H5O_info_t      infobuf;
		struct opdata* od = (struct opdata*)operator_data;
		status = H5Oget_info_by_name(loc_id, name, &infobuf, H5P_DEFAULT);
		if (status < 0)
		{
			LOG(lvlWarn, QString("H5Oget_info_by_name failed with code %1!").arg(status));
		}
		QString caption;
		int vtkType = -1;
		int rank = 0;
		switch (infobuf.type)
		{
		case H5O_TYPE_GROUP:
			caption = QString("Group: %1").arg(name);
			break;
		case H5O_TYPE_DATASET:
		{
			hid_t dset = H5Dopen(loc_id, name, H5P_DEFAULT);
			if (dset == -1)
			{
				caption = QString("Dataset %1; unable to open.").arg(name);
				break;
			}
			hid_t space = H5Dget_space(dset);
			rank = H5Sget_simple_extent_ndims(space);
			hsize_t* dims = new hsize_t[rank];
			hsize_t* maxdims = new hsize_t[rank];
			status = H5Sget_simple_extent_dims(space, dims, maxdims);
			hid_t type_id = H5Dget_type(dset);
			H5T_class_t hdf5Type = H5Tget_class(type_id);
			size_t numBytes = H5Tget_size(type_id);
			H5T_order_t order = H5Tget_order(type_id);
			H5T_sign_t sign = H5Tget_sign(type_id);
			vtkType = hdf5GetNumericVTKTypeFromHDF5Type(hdf5Type, numBytes, sign);
			H5Tclose(type_id);
			caption = QString("Dataset: %1; type=%2 (%3 bytes, order %4, %5); rank=%6; ")
				.arg(name)
				.arg(hdf5MapTypeToString(hdf5Type))
				.arg(numBytes)
				.arg((order == H5T_ORDER_LE) ? "LE" : "BE")
				.arg((sign == H5T_SGN_NONE) ? "unsigned" : "signed")
				.arg(rank);
			for (int i = 0; i < rank; ++i)
			{
				caption += QString("%1%2").arg(dims[i]).arg((dims[i] != maxdims[i]) ? QString("%1").arg(maxdims[i]) : QString());
				if (i < rank - 1) caption += " x ";
			}
			status = H5Sclose(space);
			status = H5Dclose(dset);
			break;
		}
		case H5O_TYPE_NAMED_DATATYPE:
			caption = QString("Datatype: %1").arg(name);
			break;
		default:
			caption = QString("Unknown: %1").arg(name);
		}
		QStandardItem* newItem = new QStandardItem(caption);
		od->item->appendRow(newItem);

		switch (infobuf.type) {
		case H5O_TYPE_GROUP:
			if (group_check(od, infobuf.addr))
			{
				caption += QString(" (Warning: Loop detected!)");
			}
			else
			{
				struct opdata nextod;
				nextod.item = newItem;
				nextod.recurs = od->recurs + 1;
				nextod.prev = od;
				nextod.addr = infobuf.addr;
				return_val = H5Literate_by_name(loc_id, name, H5_INDEX_NAME,
					H5_ITER_NATIVE, nullptr, op_func, (void*)&nextod,
					H5P_DEFAULT);
			}
			newItem->setData(GROUP, Qt::UserRole + 1);
			break;
		case H5O_TYPE_DATASET:
			newItem->setData(DATASET, Qt::UserRole + 1);
			break;
		default:
			newItem->setData(OTHER, Qt::UserRole + 1);
			break;
		}
		newItem->setData(QString("%1").arg(name), Qt::UserRole + 2);
		newItem->setData(vtkType, Qt::UserRole + 3);
		newItem->setData(rank, Qt::UserRole + 4);
		return return_val;
	}
}

#include "iAQTtoUIConnector.h"
#include "ui_OpenHDF5.h"

using OpenHDF5Dlg = iAQTtoUIConnector<QDialog, Ui_dlgOpenHDF5>;

class iAHDF5FileLoadParamDlg: public iAFileParamDlg
{
	bool askForParameters(QWidget* parent, iAAttributes const& parameters, QString const& ioName, QVariantMap& values, QString const& fileName, iADataSet const * dataSet) const override
	{
		Q_UNUSED(parameters); // we know which parameters we have
		Q_UNUSED(ioName);
		Q_UNUSED(dataSet);
		hid_t file_id = H5Fopen(fileName.toStdString().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
		if (file_id < 0)
		{
			hdf5PrintErrorsToConsole();
			LOG(lvlError, "H5open returned value < 0!");
			return false;
		}
		if (hdf5IsITKImage(file_id))
		{
			H5Fclose(file_id);
			return true;
		}

		QStandardItemModel* model = new QStandardItemModel();
		model->setHorizontalHeaderLabels(QStringList() << "HDF5 Structure");
		QStandardItem* rootItem = new QStandardItem(QFileInfo(fileName).fileName() + "/");
		rootItem->setData(GROUP, Qt::UserRole + 1);
		rootItem->setData(fileName, Qt::UserRole + 2);
		model->appendRow(rootItem);

		H5O_info_t infobuf;
		H5Oget_info(file_id, &infobuf);
		struct opdata   od;
		od.item = rootItem;
		od.recurs = 0;
		od.prev = nullptr;
		od.addr = infobuf.addr;
		H5Literate(file_id, H5_INDEX_NAME, H5_ITER_NATIVE, nullptr, op_func, (void*)&od);
		H5Fclose(file_id);

		// check if maybe only one dataset is contained in the file anyway:
		QStandardItem* curItem = rootItem;
		while (curItem)
		{
			if (curItem->rowCount() > 1)
			{
				curItem = nullptr;
				break;
			}
			curItem = curItem->child(0);
			assert(curItem);
			if (curItem->data(Qt::UserRole + 1) == DATASET)
			{
				break;
			}
		}
		QModelIndex idx;
		QVector<double> hdf5Spacing{ 1.0, 1.0, 1.0 };
		if (curItem && curItem->data(Qt::UserRole + 1) == DATASET)
		{
			LOG(lvlInfo, "File only contains one dataset, loading that with default spacing of 1,1,1!");
			idx = curItem->index();
		}
		else
		{
			OpenHDF5Dlg dlg(parent);
			dlg.setWindowTitle(QString("Open HDF5").arg(fileName));
			dlg.tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
			dlg.tree->setModel(model);
			QObject::connect(dlg.buttonBox, &QDialogButtonBox::accepted, &dlg, [&dlg]()
			{
				QString msg;
				auto idx = dlg.tree->currentIndex();
				if (idx.data(Qt::UserRole + 1) != DATASET)
				{
					msg = "You have to select a dataset! ";
				}
				else if (idx.data(Qt::UserRole + 3).toInt() == -1)
				{
					msg = "Can't read datasets of this data type! ";
				}
				else if (idx.data(Qt::UserRole + 4).toInt() < 1 || idx.data(Qt::UserRole + 4).toInt() > 3)
				{
					msg += QString("The rank (number of dimensions) of the dataset must be between 1 and 3 (was %1). ").arg(idx.data(Qt::UserRole + 4).toInt());
				}
				bool okX, okY, okZ;
				dlg.edSpacingX->text().toDouble(&okX);
				dlg.edSpacingY->text().toDouble(&okY);
				dlg.edSpacingZ->text().toDouble(&okZ);
				if (!(okX && okY && okZ))
				{
					msg += "One of the spacing values is invalid (these have to be valid floating point numbers)!";

				}
				if (msg.isEmpty())
				{
					dlg.accept();
				}
				else
				{
					LOG(lvlWarn, msg);
					dlg.lbErrorMessage->setText(msg);
					iAWidgetAnimationDecorator::animate(dlg.lbErrorMessage);
					// two animations might overlap and cause "flickering" (since possibly existing animation is not stopped
					// before a new one is started); but this doesn't seem to cause problems.
				}
			});
			if (dlg.exec() != QDialog::Accepted)
			{
				LOG(lvlInfo, "Dataset selection aborted.");
				return false;
			}
			idx = dlg.tree->currentIndex();
			hdf5Spacing[0] = dlg.edSpacingX->text().toDouble();
			hdf5Spacing[1] = dlg.edSpacingY->text().toDouble();
			hdf5Spacing[2] = dlg.edSpacingZ->text().toDouble();
		}
		QStringList hdf5Path;
		while (idx.parent() != QModelIndex())    // don't insert root element (filename)
		{
			hdf5Path.prepend(idx.data(Qt::UserRole + 2).toString());
			idx = idx.parent();
		}
		auto fullPath = hdf5Path.join("/");
		LOG(lvlInfo, QString("Selected path (length %1): %2").arg(hdf5Path.size()).arg(fullPath));
		values[iAHDF5IO::DataSetPathStr] = fullPath;
		values[iAHDF5IO::SpacingStr] = variantVector(hdf5Spacing);
		return true;
	}
};
#endif

void iAFileParamDlg::setupDefaultFileParamDlgs()
{
	add(settingName(iAFileIO::Load, iARawFileIO::Name), std::make_shared<iARawFileLoadParamDlg>());
	add(settingName(iAFileIO::Load, iAImageStackFileIO::Name), std::make_shared<iAImageStackLoadParamDlg>());
	add(settingName(iAFileIO::Save, iAImageStackFileIO::Name), std::make_shared<iAImageStackSaveParamDlg>());
#ifdef USE_HDF5
	add(settingName(iAFileIO::Load, iAHDF5IO::Name), std::make_shared<iAHDF5FileLoadParamDlg>());
#endif
}

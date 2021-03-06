/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of FastoNoSQL.

    FastoNoSQL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoNoSQL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoNoSQL.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gui/widgets/type_delegate.h"

#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QTableView>
#include <QHeaderView>

#include <common/convert2string.h>
#include <common/qt/convert2string.h>
#include <common/qt/utils_qt.h>

#include "gui/widgets/list_type_widget.h"
#include "gui/widgets/hash_type_widget.h"

#include "gui/fasto_common_item.h"

Q_DECLARE_METATYPE(fastonosql::core::NValue)

namespace fastonosql {
namespace gui {

TypeDelegate::TypeDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QSize TypeDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
  UNUSED(option);
  UNUSED(index);
  return QSize(row_height, row_height);
}

QWidget* TypeDelegate::createEditor(QWidget* parent,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const {
  FastoCommonItem* node = common::qt::item<common::qt::gui::TreeItem*, FastoCommonItem*>(index);
  if (!node) {
    return QStyledItemDelegate::createEditor(parent, option, index);
  }

  common::Value::Type t = node->type();
  if (t == common::Value::TYPE_INTEGER || t == common::Value::TYPE_UINTEGER) {
    QSpinBox* editor = new QSpinBox(parent);
    editor->setRange(INT32_MIN, INT32_MAX);
    return editor;
  } else if (t == common::Value::TYPE_BOOLEAN) {
    QComboBox* editor = new QComboBox(parent);
    editor->addItem("true");
    editor->addItem("false");
    return editor;
  } else if (t == common::Value::TYPE_STRING) {
    QLineEdit* editor = new QLineEdit(parent);
    return editor;
  } else if (t == common::Value::TYPE_ARRAY || t == common::Value::TYPE_SET) {
    ListTypeWidget* editor = new ListTypeWidget(parent);
    editor->horizontalHeader()->hide();
    editor->verticalHeader()->hide();
    return editor;
  } else if (t == common::Value::TYPE_ZSET || t == common::Value::TYPE_HASH) {
    HashTypeWidget* editor = new HashTypeWidget(parent);
    editor->horizontalHeader()->hide();
    editor->verticalHeader()->hide();
    return editor;
  } else {
    return QStyledItemDelegate::createEditor(parent, option, index);
  }
}

void TypeDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
  FastoCommonItem* node = common::qt::item<common::qt::gui::TreeItem*, FastoCommonItem*>(index);
  if (!node) {
    return;
  }

  core::NDbKValue dbv = node->dbv();
  core::NValue val = dbv.Value();
  common::Value::Type t = node->type();
  if (t == common::Value::TYPE_INTEGER) {
    int value = 0;
    if (val->getAsInteger(&value)) {
      QSpinBox* spinBox = static_cast<QSpinBox*>(editor);
      spinBox->setValue(value);
    }
  } else if (t == common::Value::TYPE_UINTEGER) {
    unsigned int value = 0;
    if (val->getAsUInteger(&value)) {
      QSpinBox* spinBox = static_cast<QSpinBox*>(editor);
      spinBox->setValue(value);
    }
  } else if (t == common::Value::TYPE_BOOLEAN) {
    bool value;
    if (val->getAsBoolean(&value)) {
      QComboBox* combobox = static_cast<QComboBox*>(editor);
      combobox->setCurrentIndex(value ? 0 : 1);
    }
  } else if (t == common::Value::TYPE_STRING) {
    std::string value;
    if (val->getAsString(&value)) {
      QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
      lineedit->setText(common::ConvertFromString<QString>(value));
    }
  } else if (t == common::Value::TYPE_ARRAY) {
    common::ArrayValue* arr = nullptr;
    if (val->getAsList(&arr)) {
      ListTypeWidget* listwidget = static_cast<ListTypeWidget*>(editor);
      for (auto it = arr->begin(); it != arr->end(); ++it) {
        std::string val = (*it)->toString();
        if (val.empty()) {
          continue;
        }

        listwidget->insertRow(common::ConvertFromString<QString>(val));
      }
    }
  } else if (t == common::Value::TYPE_SET) {
    common::SetValue* set = nullptr;
    if (val->getAsSet(&set)) {
      ListTypeWidget* listwidget = static_cast<ListTypeWidget*>(editor);
      for (auto it = set->begin(); it != set->end(); ++it) {
        std::string val = (*it)->toString();
        if (val.empty()) {
          continue;
        }

        listwidget->insertRow(common::ConvertFromString<QString>(val));
      }
    }
  } else if (t == common::Value::TYPE_ZSET) {
    common::ZSetValue* zset = nullptr;
    if (val->getAsZSet(&zset)) {
      HashTypeWidget* hashwidget = static_cast<HashTypeWidget*>(editor);
      for (auto it = zset->begin(); it != zset->end(); ++it) {
        auto element = (*it);
        common::Value* key = element.first;
        common::Value* value = element.second;
        QString ftext = common::ConvertFromString<QString>(key->toString());
        QString stext = common::ConvertFromString<QString>(value->toString());

        hashwidget->insertRow(ftext, stext);
      }
    }
  } else if (t == common::Value::TYPE_HASH) {
    common::HashValue* hash = nullptr;
    if (val->getAsHash(&hash)) {
      HashTypeWidget* hashwidget = static_cast<HashTypeWidget*>(editor);
      for (auto it = hash->begin(); it != hash->end(); ++it) {
        auto element = (*it);
        common::Value* key = element.first;
        common::Value* value = element.second;
        QString ftext = common::ConvertFromString<QString>(key->toString());
        QString stext = common::ConvertFromString<QString>(value->toString());

        hashwidget->insertRow(ftext, stext);
      }
    }
  } else {
    QStyledItemDelegate::setEditorData(editor, index);
  }
}

void TypeDelegate::setModelData(QWidget* editor,
                                QAbstractItemModel* model,
                                const QModelIndex& index) const {
  FastoCommonItem* node = common::qt::item<common::qt::gui::TreeItem*, FastoCommonItem*>(index);
  if (!node) {
    return;
  }

  common::Value::Type t = node->type();
  if (t == common::Value::TYPE_INTEGER) {
    QSpinBox* spinBox = static_cast<QSpinBox*>(editor);
    int value = spinBox->value();
    core::NValue val(common::Value::createIntegerValue(value));
    QVariant var = QVariant::fromValue(val);
    model->setData(index, var, Qt::EditRole);
  } else if (t == common::Value::TYPE_UINTEGER) {
    QSpinBox* spinBox = static_cast<QSpinBox*>(editor);
    int value = spinBox->value();
    core::NValue val(common::Value::createUIntegerValue(value));
    QVariant var = QVariant::fromValue(val);
    model->setData(index, var, Qt::EditRole);
  } else if (t == common::Value::TYPE_BOOLEAN) {
    QComboBox* combobox = static_cast<QComboBox*>(editor);
    int cindex = combobox->currentIndex();
    core::NValue val(common::Value::createBooleanValue(cindex == 0));
    QVariant var = QVariant::fromValue(val);
    model->setData(index, var, Qt::EditRole);
  } else if (t == common::Value::TYPE_STRING) {
    QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
    QString text = lineedit->text();
    if (text.isEmpty()) {
      return;
    }

    common::StringValue* string = common::Value::createStringValue(common::ConvertToString(text));
    QVariant var = QVariant::fromValue(core::NValue(string));
    model->setData(index, var, Qt::EditRole);
  } else if (t == common::Value::TYPE_ARRAY) {
    ListTypeWidget* listwidget = static_cast<ListTypeWidget*>(editor);
    common::ArrayValue* arr = listwidget->arrayValue();
    if (!arr) {
      return;
    }

    QVariant var = QVariant::fromValue(core::NValue(arr));
    model->setData(index, var, Qt::EditRole);
  } else if (t == common::Value::TYPE_SET) {
    ListTypeWidget* listwidget = static_cast<ListTypeWidget*>(editor);
    common::SetValue* set = listwidget->setValue();
    if (!set) {
      return;
    }

    QVariant var = QVariant::fromValue(core::NValue(set));
    model->setData(index, var, Qt::EditRole);
  } else if (t == common::Value::TYPE_ZSET) {
    HashTypeWidget* hashwidget = static_cast<HashTypeWidget*>(editor);
    common::ZSetValue* zset = hashwidget->zsetValue();
    if (!zset) {
      return;
    }

    QVariant var = QVariant::fromValue(core::NValue(zset));
    model->setData(index, var, Qt::EditRole);
  } else if (t == common::Value::TYPE_HASH) {
    HashTypeWidget* hashwidget = static_cast<HashTypeWidget*>(editor);
    common::HashValue* hash = hashwidget->hashValue();
    if (!hash) {
      return;
    }

    QVariant var = QVariant::fromValue(core::NValue(hash));
    model->setData(index, var, Qt::EditRole);
  } else {
    QStyledItemDelegate::setModelData(editor, model, index);
  }
}

void TypeDelegate::updateEditorGeometry(QWidget* editor,
                                        const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const {
  UNUSED(index);

  editor->setGeometry(option.rect);
}

}  // namespace gui
}  // namespace fastonosql

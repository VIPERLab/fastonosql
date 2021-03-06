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

#pragma once

#include <QTableView>

#include <common/value.h>

namespace fastonosql {
namespace gui {

class HashTableModel;

class HashTypeWidget : public QTableView {
  Q_OBJECT
 public:
  explicit HashTypeWidget(QWidget* parent = Q_NULLPTR);
  virtual ~HashTypeWidget();

  void insertRow(const QString& first, const QString& second);
  void clear();

  common::ZSetValue* zsetValue() const;  // alocate memory
  common::HashValue* hashValue() const;  // alocate memory

 private Q_SLOTS:
  void addRow(const QModelIndex& index);
  void removeRow(const QModelIndex& index);

 private:
  HashTableModel* model_;
};

}  // namespace gui
}  // namespace fastonosql

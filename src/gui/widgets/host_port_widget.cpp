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

#include "gui/widgets/host_port_widget.h"

#include <QHBoxLayout>
#include <QEvent>
#include <QLineEdit>
#include <QLabel>
#include <QRegExpValidator>

#include <common/convert2string.h>
#include <common/qt/convert2string.h>

namespace fastonosql {
namespace gui {

HostPortWidget::HostPortWidget(QWidget* parent) : QWidget(parent) {
  QHBoxLayout* hostAndPasswordLayout = new QHBoxLayout;
  hostName_ = new QLineEdit;
  hostPort_ = new QLineEdit;
  hostPort_->setFixedWidth(80);
  QRegExp rx("\\d+");  // (0-65554)
  hostPort_->setValidator(new QRegExpValidator(rx, this));
  hostAndPasswordLayout->addWidget(hostName_);
  hostAndPasswordLayout->addWidget(new QLabel(":"));
  hostAndPasswordLayout->addWidget(hostPort_);
  setLayout(hostAndPasswordLayout);

  retranslateUi();
}

common::net::HostAndPort HostPortWidget::host() const {
  return common::net::HostAndPort(common::ConvertToString(hostName_->text()),
                                  hostPort_->text().toInt());
}

void HostPortWidget::setHost(const common::net::HostAndPort& host) {
  hostName_->setText(common::ConvertFromString<QString>(host.host));
  hostPort_->setText(QString::number(host.port));
}

bool HostPortWidget::isValidHost() const {
  common::net::HostAndPort hs = host();
  return hs.isValid();
}

void HostPortWidget::retranslateUi() {}

void HostPortWidget::changeEvent(QEvent* ev) {
  if (ev->type() == QEvent::LanguageChange) {
    retranslateUi();
  }

  QWidget::changeEvent(ev);
}

}  // namespace gui
}  // namespace fastonosql

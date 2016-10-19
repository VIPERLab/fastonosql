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

#include "shell/shell_widget.h"

#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint32_t

#include <memory>  // for __shared_ptr
#include <string>  // for string
#include <vector>  // for vector

#include <QAction>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QSplitter>
#include <QToolBar>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSpinBox>

#include <common/convert2string.h>  // for ConvertFromString
#include <common/error.h>           // for Error
#include <common/macros.h>          // for VERIFY, UNUSED, CHECK, etc
#include <common/value.h>           // for ErrorValue

#include <common/qt/convert2string.h>  // for ConvertToString
#include <common/qt/gui/icon_label.h>  // for IconLabel
#include <common/qt/gui/shortcuts.h>   // for FastoQKeySequence
#include <common/qt/utils_qt.h>        // for SaveToFileText, etc

#include "core/command/command_info.h"  // for UNDEFINED_SINCE, etc
#include "core/events/events_info.h"    // for DiscoveryInfoResponce, etc
#include "core/server/iserver.h"        // for IServer
#include "core/server/iserver_local.h"
#include "core/server/iserver_remote.h"
#include "core/settings_manager.h"  // for SettingsManager

#include "gui/gui_factory.h"  // for GuiFactory
#include "gui/shortcuts.h"    // for executeKey

#include "shell/base_shell.h"  // for BaseShell

#include "translations/global.h"  // for trError, trSaveAs, etc

namespace {
const QSize iconSize = QSize(24, 24);
const QString trSupportedCommandsCountTemplate_1S = QObject::tr("Supported commands count: %1");
const QString trCommandsVersion = QObject::tr("Command version:");
const QString trCantReadTemplate_2S = QObject::tr(PROJECT_NAME_TITLE " can't read from %1:\n%2.");
const QString trCantSaveTemplate_2S = QObject::tr(PROJECT_NAME_TITLE " can't save to %1:\n%2.");
const QString trAdvancedOptions = QObject::tr("Advanced options");
const QString trCalculating = QObject::tr("Calculate...");
}

namespace fastonosql {
namespace shell {

namespace {
BaseShell* makeBaseShell(core::connectionTypes type, QWidget* parent) {
  BaseShell* shell =
      BaseShell::createFromType(type, core::SettingsManager::instance().autoCompletion());
  parent->setToolTip(
      QObject::tr("Based on <b>%1</b> version: <b>%2</b>").arg(shell->basedOn(), shell->version()));
  shell->setContextMenuPolicy(Qt::CustomContextMenu);
  return shell;
}
}
BaseShellWidget::BaseShellWidget(core::IServerSPtr server, const QString& filePath, QWidget* parent)
    : QWidget(parent), server_(server), input_(nullptr), filePath_(filePath) {
  CHECK(server_);

  VERIFY(
      connect(server_.get(), &core::IServer::startedConnect, this, &BaseShellWidget::startConnect));
  VERIFY(connect(server_.get(), &core::IServer::finishedConnect, this,
                 &BaseShellWidget::finishConnect));
  VERIFY(connect(server_.get(), &core::IServer::startedDisconnect, this,
                 &BaseShellWidget::startDisconnect));
  VERIFY(connect(server_.get(), &core::IServer::finishedDisconnect, this,
                 &BaseShellWidget::finishDisconnect));
  VERIFY(connect(server_.get(), &core::IServer::progressChanged, this,
                 &BaseShellWidget::progressChange));

  VERIFY(connect(server_.get(), &core::IServer::startedSetDefaultDatabase, this,
                 &BaseShellWidget::startSetDefaultDatabase));
  VERIFY(connect(server_.get(), &core::IServer::finishedSetDefaultDatabase, this,
                 &BaseShellWidget::finishSetDefaultDatabase));

  VERIFY(connect(server_.get(), &core::IServer::enteredMode, this, &BaseShellWidget::enterMode));
  VERIFY(connect(server_.get(), &core::IServer::leavedMode, this, &BaseShellWidget::leaveMode));

  VERIFY(connect(server_.get(), &core::IServer::startedLoadDiscoveryInfo, this,
                 &BaseShellWidget::startLoadDiscoveryInfo));
  VERIFY(connect(server_.get(), &core::IServer::finishedLoadDiscoveryInfo, this,
                 &BaseShellWidget::finishLoadDiscoveryInfo));

  VERIFY(connect(server_.get(), &core::IServer::startedExecute, this,
                 &BaseShellWidget::startExecute, Qt::DirectConnection));
  VERIFY(connect(server_.get(), &core::IServer::finishedExecute, this,
                 &BaseShellWidget::finishExecute, Qt::DirectConnection));

  QVBoxLayout* mainlayout = new QVBoxLayout;
  QHBoxLayout* hlayout = new QHBoxLayout;

  QToolBar* savebar = new QToolBar;

  loadAction_ = new QAction(gui::GuiFactory::instance().loadIcon(), translations::trLoad, savebar);
  typedef void (BaseShellWidget::*lf)();
  VERIFY(connect(loadAction_, &QAction::triggered, this,
                 static_cast<lf>(&BaseShellWidget::loadFromFile)));
  savebar->addAction(loadAction_);

  saveAction_ = new QAction(gui::GuiFactory::instance().saveIcon(), translations::trSave, savebar);
  VERIFY(connect(saveAction_, &QAction::triggered, this, &BaseShellWidget::saveToFile));
  savebar->addAction(saveAction_);

  saveAsAction_ =
      new QAction(gui::GuiFactory::instance().saveAsIcon(), translations::trSaveAs, savebar);
  VERIFY(connect(saveAsAction_, &QAction::triggered, this, &BaseShellWidget::saveToFileAs));
  savebar->addAction(saveAsAction_);

  connectAction_ =
      new QAction(gui::GuiFactory::instance().connectIcon(), translations::trConnect, savebar);
  VERIFY(connect(connectAction_, &QAction::triggered, this, &BaseShellWidget::connectToServer));
  savebar->addAction(connectAction_);

  disConnectAction_ = new QAction(gui::GuiFactory::instance().disConnectIcon(),
                                  translations::trDisconnect, savebar);
  VERIFY(connect(disConnectAction_, &QAction::triggered, this,
                 &BaseShellWidget::disconnectFromServer));
  savebar->addAction(disConnectAction_);

  executeAction_ =
      new QAction(gui::GuiFactory::instance().executeIcon(), translations::trExecute, savebar);
  executeAction_->setShortcut(gui::executeKey);
  VERIFY(connect(executeAction_, &QAction::triggered, this, &BaseShellWidget::execute));
  savebar->addAction(executeAction_);

  stopAction_ = new QAction(gui::GuiFactory::instance().stopIcon(), translations::trStop, savebar);
  VERIFY(connect(stopAction_, &QAction::triggered, this, &BaseShellWidget::stop));
  savebar->addAction(stopAction_);

  core::ConnectionMode mode = core::InteractiveMode;
  connectionMode_ = new common::qt::gui::IconLabel(
      gui::GuiFactory::instance().modeIcon(mode),
      common::ConvertFromString<QString>(common::ConvertToString(mode)), iconSize);

  hlayout->addWidget(savebar);
  hlayout->addWidget(new QSplitter(Qt::Horizontal));

  hlayout->addWidget(connectionMode_);
  workProgressBar_ = new QProgressBar;
  workProgressBar_->setTextVisible(true);
  hlayout->addWidget(workProgressBar_);
  mainlayout->addLayout(hlayout);

  advancedOptions_ = new QCheckBox;
  advancedOptions_->setText(trAdvancedOptions);
  VERIFY(connect(advancedOptions_, &QCheckBox::stateChanged, this,
                 &BaseShellWidget::advancedOptionsChange));

  input_ = makeBaseShell(server->type(), this);

  advancedOptionsWidget_ = new QWidget;
  advancedOptionsWidget_->setVisible(false);
  QVBoxLayout* advOptLayout = new QVBoxLayout;

  QHBoxLayout* repeatLayout = new QHBoxLayout;
  QLabel* repeatLabel = new QLabel(translations::trRepeat);
  repeatCount_ = new QSpinBox;
  repeatCount_->setRange(0, INT32_MAX);
  repeatCount_->setSingleStep(1);
  repeatLayout->addWidget(repeatCount_);
  repeatLayout->addWidget(repeatLabel);

  QHBoxLayout* intervalLayout = new QHBoxLayout;
  QLabel* intervalLabel = new QLabel(translations::trInterval);
  intervalMsec_ = new QSpinBox;
  intervalMsec_->setRange(0, INT32_MAX);
  intervalMsec_->setSingleStep(1000);
  intervalLayout->addWidget(intervalMsec_);
  intervalLayout->addWidget(intervalLabel);

  historyCall_ = new QCheckBox(translations::trHistory);
  historyCall_->setChecked(true);
  advOptLayout->addLayout(repeatLayout);
  advOptLayout->addLayout(intervalLayout);
  advOptLayout->addWidget(historyCall_);
  advancedOptionsWidget_->setLayout(advOptLayout);

  QHBoxLayout* hlayout2 = new QHBoxLayout;
  core::connectionTypes ct = server_->type();
  serverName_ =
      new common::qt::gui::IconLabel(gui::GuiFactory::instance().icon(ct), trCalculating, iconSize);
  serverName_->setElideMode(Qt::ElideRight);
  hlayout2->addWidget(serverName_);
  dbName_ = new common::qt::gui::IconLabel(gui::GuiFactory::instance().databaseIcon(),
                                           trCalculating, iconSize);
  hlayout2->addWidget(dbName_);
  hlayout2->addWidget(new QSplitter(Qt::Horizontal));
  hlayout2->addWidget(advancedOptions_);
  mainlayout->addLayout(hlayout2);

  QHBoxLayout* inputLayout = new QHBoxLayout;
  inputLayout->addWidget(input_);
  inputLayout->addWidget(advancedOptionsWidget_);
  mainlayout->addLayout(inputLayout);

  QHBoxLayout* apilayout = new QHBoxLayout;
  apilayout->addWidget(
      new QLabel(trSupportedCommandsCountTemplate_1S.arg(input_->commandsCount())));
  apilayout->addWidget(new QSplitter(Qt::Horizontal));

  commandsVersionApi_ = new QComboBox;
  typedef void (QComboBox::*curc)(int);
  VERIFY(connect(commandsVersionApi_, static_cast<curc>(&QComboBox::currentIndexChanged), this,
                 &BaseShellWidget::changeVersionApi));

  std::vector<uint32_t> versions = input_->supportedVersions();
  for (size_t i = 0; i < versions.size(); ++i) {
    uint32_t cur = versions[i];
    std::string curVers = core::convertVersionNumberToReadableString(cur);
    commandsVersionApi_->addItem(gui::GuiFactory::instance().unknownIcon(),
                                 common::ConvertFromString<QString>(curVers), cur);
    commandsVersionApi_->setCurrentIndex(i);
  }
  apilayout->addWidget(new QLabel(trCommandsVersion));
  apilayout->addWidget(commandsVersionApi_);
  mainlayout->addLayout(apilayout);

  setLayout(mainlayout);

  syncConnectionActions();
  updateServerInfo(server_->serverInfo());
  updateDefaultDatabase(server_->currentDatabaseInfo());
}

void BaseShellWidget::advancedOptionsChange(int state) {
  advancedOptionsWidget_->setVisible(state);
}

BaseShellWidget::~BaseShellWidget() {}

QString BaseShellWidget::text() const {
  return input_->text();
}

void BaseShellWidget::setText(const QString& text) {
  input_->setText(text);
}

void BaseShellWidget::executeText(const QString& text) {
  input_->setText(text);
  execute();
}

void BaseShellWidget::execute() {
  QString selected = input_->selectedText();
  if (selected.isEmpty()) {
    selected = input_->text();
  }

  int repeat = repeatCount_->value();
  int interval = intervalMsec_->value();
  bool history = historyCall_->isChecked();
  executeArgs(selected, repeat, interval, history);
}

void BaseShellWidget::executeArgs(const QString& text, int repeat, int interval, bool history) {
  core::events_info::ExecuteInfoRequest req(this, common::ConvertToString(text), repeat, interval,
                                            history);
  server_->execute(req);
}

void BaseShellWidget::stop() {
  server_->stopCurrentEvent();
}

void BaseShellWidget::connectToServer() {
  core::events_info::ConnectInfoRequest req(this);
  server_->connect(req);
}

void BaseShellWidget::disconnectFromServer() {
  core::events_info::DisConnectInfoRequest req(this);
  server_->disconnect(req);
}

void BaseShellWidget::loadFromFile() {
  loadFromFile(filePath_);
}

bool BaseShellWidget::loadFromFile(const QString& path) {
  QString filepath =
      QFileDialog::getOpenFileName(this, path, QString(), translations::trfilterForScripts);
  if (!filepath.isEmpty()) {
    QString out;
    common::Error err = common::qt::LoadFromFileText(filepath, &out);
    if (err && err->isError()) {
      QMessageBox::critical(this, translations::trError,
                            trCantReadTemplate_2S.arg(
                                filepath, common::ConvertFromString<QString>(err->description())));
      return false;
    }

    setText(out);
    filePath_ = filepath;
    return true;
  }

  return false;
}

void BaseShellWidget::saveToFileAs() {
  QString filepath = QFileDialog::getSaveFileName(this, translations::trSaveAs, filePath_,
                                                  translations::trfilterForScripts);
  if (filepath.isEmpty()) {
    return;
  }

  common::Error err = common::qt::SaveToFileText(filepath, text());
  if (err && err->isError()) {
    QMessageBox::critical(this, translations::trError,
                          trCantSaveTemplate_2S.arg(
                              filepath, common::ConvertFromString<QString>(err->description())));
    return;
  }

  filePath_ = filepath;
}

void BaseShellWidget::changeVersionApi(int index) {
  if (index == -1) {
    return;
  }

  QVariant var = commandsVersionApi_->itemData(index);
  uint32_t version = qvariant_cast<uint32_t>(var);
  input_->setFilteredVersion(version);
}

void BaseShellWidget::saveToFile() {
  if (filePath_.isEmpty()) {
    saveToFileAs();
  } else {
    common::Error err = common::qt::SaveToFileText(filePath_, text());
    if (err && err->isError()) {
      QMessageBox::critical(this, translations::trError,
                            trCantSaveTemplate_2S.arg(
                                filePath_, common::ConvertFromString<QString>(err->description())));
    }
  }
}

void BaseShellWidget::startConnect(const core::events_info::ConnectInfoRequest& req) {
  UNUSED(req);

  syncConnectionActions();
}

void BaseShellWidget::finishConnect(const core::events_info::ConnectInfoResponce& res) {
  UNUSED(res);

  syncConnectionActions();
}

void BaseShellWidget::startDisconnect(const core::events_info::DisConnectInfoRequest& req) {
  UNUSED(req);

  syncConnectionActions();
}

void BaseShellWidget::finishDisconnect(const core::events_info::DisConnectInfoResponce& res) {
  UNUSED(res);

  syncConnectionActions();
  updateServerInfo(core::IServerInfoSPtr());
  updateDefaultDatabase(core::IDataBaseInfoSPtr());
}

void BaseShellWidget::startSetDefaultDatabase(
    const core::events_info::SetDefaultDatabaseRequest& req) {
  UNUSED(req);
}

void BaseShellWidget::finishSetDefaultDatabase(
    const core::events_info::SetDefaultDatabaseResponce& res) {
  common::Error er = res.errorInfo();
  if (er && er->isError()) {
    return;
  }

  core::IServer* serv = qobject_cast<core::IServer*>(sender());
  if (!serv) {
    DNOTREACHED();
    return;
  }

  core::IDataBaseInfoSPtr db = res.inf;
  updateDefaultDatabase(db);
}

void BaseShellWidget::progressChange(const core::events_info::ProgressInfoResponce& res) {
  workProgressBar_->setValue(res.progress);
}

void BaseShellWidget::enterMode(const core::events_info::EnterModeInfo& res) {
  core::ConnectionMode mode = res.mode;
  connectionMode_->setIcon(gui::GuiFactory::instance().modeIcon(mode), iconSize);
  std::string modeText = common::ConvertToString(mode);
  connectionMode_->setText(common::ConvertFromString<QString>(modeText));
}

void BaseShellWidget::leaveMode(const core::events_info::LeaveModeInfo& res) {
  UNUSED(res);
}

void BaseShellWidget::startLoadDiscoveryInfo(const core::events_info::DiscoveryInfoRequest& res) {
  UNUSED(res);
}

void BaseShellWidget::finishLoadDiscoveryInfo(const core::events_info::DiscoveryInfoResponce& res) {
  common::Error err = res.errorInfo();
  if (err && err->isError()) {
    return;
  }

  updateServerInfo(res.sinfo);
  updateDefaultDatabase(res.dbinfo);
}

void BaseShellWidget::startExecute(const core::events_info::ExecuteInfoRequest& req) {
  UNUSED(req);

  repeatCount_->setEnabled(false);
  intervalMsec_->setEnabled(false);
  historyCall_->setEnabled(false);
  executeAction_->setEnabled(false);
  stopAction_->setEnabled(true);
}
void BaseShellWidget::finishExecute(const core::events_info::ExecuteInfoResponce& res) {
  UNUSED(res);

  repeatCount_->setEnabled(true);
  intervalMsec_->setEnabled(true);
  historyCall_->setEnabled(true);
  executeAction_->setEnabled(true);
  stopAction_->setEnabled(false);
}

void BaseShellWidget::updateServerInfo(core::IServerInfoSPtr inf) {
  if (!inf) {
    serverName_->setText(trCalculating);
    for (int i = 0; i < commandsVersionApi_->count(); ++i) {
      commandsVersionApi_->setItemIcon(i, gui::GuiFactory::instance().unknownIcon());
    }
    return;
  }

  std::string server_label;
  if (server_->isCanRemote()) {
    core::IServerRemote* rserver = dynamic_cast<core::IServerRemote*>(server_.get());  // +
    server_label = common::ConvertToString(rserver->host());
  } else {
    core::IServerLocal* lserver = dynamic_cast<core::IServerLocal*>(server_.get());  // +
    server_label = lserver->path();
  }
  QString qserver_label = common::ConvertFromString<QString>(server_label);
  serverName_->setText(qserver_label);

  uint32_t servVers = inf->version();
  if (servVers == UNDEFINED_SINCE) {
    return;
  }

  bool updatedComboIndex = false;
  for (int i = 0; i < commandsVersionApi_->count(); ++i) {
    QVariant var = commandsVersionApi_->itemData(i);
    uint32_t version = qvariant_cast<uint32_t>(var);
    if (version == UNDEFINED_SINCE) {
      commandsVersionApi_->setItemIcon(i, gui::GuiFactory::instance().unknownIcon());
      continue;
    }

    if (version >= servVers) {
      if (!updatedComboIndex) {
        updatedComboIndex = true;
        commandsVersionApi_->setCurrentIndex(i);
        commandsVersionApi_->setItemIcon(i, gui::GuiFactory::instance().successIcon());
      } else {
        commandsVersionApi_->setItemIcon(i, gui::GuiFactory::instance().failIcon());
      }
    } else {
      commandsVersionApi_->setItemIcon(i, gui::GuiFactory::instance().successIcon());
    }
  }
}

void BaseShellWidget::updateDefaultDatabase(core::IDataBaseInfoSPtr dbs) {
  if (!dbs) {
    dbName_->setText(trCalculating);
    return;
  }

  std::string name = dbs->name();
  dbName_->setText(common::ConvertFromString<QString>(name));
}

void BaseShellWidget::syncConnectionActions() {
  connectAction_->setVisible(!server_->isConnected());
  disConnectAction_->setVisible(server_->isConnected());
  executeAction_->setEnabled(server_->isConnected());
  stopAction_->setEnabled(!executeAction_->isEnabled());
}

}  // namespace shell
}  // namespace fastonosql

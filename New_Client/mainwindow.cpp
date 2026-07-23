#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "resetdialog.h"
#include "tcpmgr.h"
#include <QLayout>
#include <QIcon>

static bool g_devSkipLogin = false;

void MainWindow_SetDevSkipLogin(bool skip)
{
    g_devSkipLogin = skip;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _login_dlg(nullptr),
    _reg_dlg(nullptr),
    _reset_dlg(nullptr),
    _chat_dlg(nullptr),
    _devSkipLogin(g_devSkipLogin)
{
    ui->setupUi(this);

    if (_devSkipLogin) {
        enterChatUi();
        connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);
        return;
    }

    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);
    FitToCentralWidget();

    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::enterChatUi()
{
    // setCentralWidget 会接管并删除原先的 central（若有），避免手动 deleteLater 与旧指针错乱
    ChatDialog *dlg = new ChatDialog(this);
    dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(dlg);
    _chat_dlg = dlg;
    _chat_dlg->show();
    setMinimumSize(QSize(1050, 900));
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    resize(QSize(1050, 900));
}

void MainWindow::FitToCentralWidget()
{
    QWidget *cw = centralWidget();
    if (!cw) {
        return;
    }

    cw->adjustSize();
    QSize target = cw->size();
    if (!target.isValid() || target.isEmpty()) {
        target = cw->sizeHint();
    }

    if (target.isValid() && !target.isEmpty()) {
        setMinimumSize(target);
        setMaximumSize(target);
        resize(target);
    }
}

void MainWindow::SlotSwitchReg()
{
    _reg_dlg = new RegisterDialog(this);
    _reg_dlg->hide();
    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);

    connect(_reg_dlg, &RegisterDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    setCentralWidget(_reg_dlg);
    FitToCentralWidget();
    if (_login_dlg) {
        _login_dlg->hide();
    }
    _reg_dlg->show();
}

void MainWindow::SlotSwitchLogin()
{
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);
    FitToCentralWidget();

    if (_reg_dlg) {
        _reg_dlg->hide();
    }
    _login_dlg->show();
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
}

void MainWindow::SlotSwitchReset()
{
    _reset_dlg = new ResetDialog(this);
    _reset_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_reset_dlg);
    FitToCentralWidget();

    if (_login_dlg) {
        _login_dlg->hide();
    }
    _reset_dlg->show();
    connect(_reset_dlg, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLogin2);
}

void MainWindow::SlotSwitchLogin2()
{
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);
    FitToCentralWidget();

    if (_reset_dlg) {
        _reset_dlg->hide();
    }
    _login_dlg->show();
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
}

void MainWindow::SlotSwitchChat()
{
    enterChatUi();
    if (_login_dlg) {
        _login_dlg->hide();
    }
}

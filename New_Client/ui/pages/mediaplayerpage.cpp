#include "mediaplayerpage.h"
#include "ui_mediaplayerpage.h"

mediaplayerpage::mediaplayerpage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::mediaplayerpage)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Widget);
}

mediaplayerpage::~mediaplayerpage()
{
    delete ui;
}

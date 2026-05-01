#ifndef MEDIAPLAYERPAGE_H
#define MEDIAPLAYERPAGE_H

#include <QDialog>

namespace Ui {
class mediaplayerpage;
}

class mediaplayerpage : public QDialog
{
    Q_OBJECT

public:
    explicit mediaplayerpage(QWidget *parent = nullptr);
    ~mediaplayerpage();

private:
    Ui::mediaplayerpage *ui;
};

#endif // MEDIAPLAYERPAGE_H

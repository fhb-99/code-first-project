/********************************************************************************
** Form generated from reading UI file 'mediaplayerpage.ui'
**
** Created by: Qt User Interface Compiler version 5.12.10
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MEDIAPLAYERPAGE_H
#define UI_MEDIAPLAYERPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>

QT_BEGIN_NAMESPACE

class Ui_mediaplayerpage
{
public:
    QLabel *label;

    void setupUi(QDialog *mediaplayerpage)
    {
        if (mediaplayerpage->objectName().isEmpty())
            mediaplayerpage->setObjectName(QString::fromUtf8("mediaplayerpage"));
        mediaplayerpage->resize(457, 471);
        label = new QLabel(mediaplayerpage);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(170, 10, 121, 81));

        retranslateUi(mediaplayerpage);

        QMetaObject::connectSlotsByName(mediaplayerpage);
    } // setupUi

    void retranslateUi(QDialog *mediaplayerpage)
    {
        mediaplayerpage->setWindowTitle(QApplication::translate("mediaplayerpage", "Dialog", nullptr));
        label->setText(QApplication::translate("mediaplayerpage", "\346\222\255\346\224\276\345\231\250", nullptr));
    } // retranslateUi

};

namespace Ui {
    class mediaplayerpage: public Ui_mediaplayerpage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MEDIAPLAYERPAGE_H

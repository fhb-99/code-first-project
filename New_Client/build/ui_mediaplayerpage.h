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
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_mediaplayerpage
{
public:
    QVBoxLayout *verticalLayout_root;
    QFrame *frame_header;
    QHBoxLayout *horizontalLayout_header;
    QLabel *lb_page_title;
    QSpacerItem *horizontalSpacer_header;
    QLabel *lb_session;
    QFrame *frame_video_container;
    QVBoxLayout *verticalLayout_video;
    QWidget *video_render_host;
    QVBoxLayout *verticalLayout_video_placeholder;
    QLabel *lb_video_placeholder;
    QFrame *frame_controls;
    QVBoxLayout *verticalLayout_controls;
    QHBoxLayout *horizontalLayout_controls_top;
    QPushButton *btn_play;
    QPushButton *btn_stop;
    QSpacerItem *horizontalSpacer_controls_mid;
    QLabel *lb_volume;
    QSlider *slider_volume;
    QHBoxLayout *horizontalLayout_progress;
    QLabel *lb_time_current;
    QSlider *slider_progress;
    QLabel *lb_time_total;
    QFrame *frame_status_bar;
    QHBoxLayout *horizontalLayout_status;
    QLabel *lb_status_text;
    QSpacerItem *horizontalSpacer_status;
    QLabel *lb_stream_text;

    void setupUi(QDialog *mediaplayerpage)
    {
        if (mediaplayerpage->objectName().isEmpty())
            mediaplayerpage->setObjectName(QString::fromUtf8("mediaplayerpage"));
        mediaplayerpage->resize(578, 641);
        mediaplayerpage->setStyleSheet(QString::fromUtf8("QDialog#mediaplayerpage {\n"
"  background: #eef1f5;\n"
"}\n"
"QFrame#frame_header,\n"
"QFrame#frame_controls,\n"
"QFrame#frame_status_bar {\n"
"  background: #ffffff;\n"
"  border: 1px solid #d8dde6;\n"
"  border-radius: 8px;\n"
"}\n"
"QFrame#frame_video_container {\n"
"  background: #151a22;\n"
"  border: 1px solid #2b3240;\n"
"  border-radius: 10px;\n"
"}\n"
"QWidget#video_render_host {\n"
"  background: #10151d;\n"
"}\n"
"QLabel#lb_video_placeholder {\n"
"  color: #7f8ca1;\n"
"  font-size: 16px;\n"
"}\n"
"QPushButton {\n"
"  background: #f6f8fb;\n"
"  border: 1px solid #cfd6e2;\n"
"  border-radius: 6px;\n"
"  padding: 4px 12px;\n"
"  color: #1f2937;\n"
"}\n"
"QPushButton:hover {\n"
"  background: #e3ebfa;\n"
"  border: 1px solid #9fb4d9;\n"
"}\n"
"QPushButton:pressed {\n"
"  background: #d2def5;\n"
"  border: 1px solid #6f8fc4;\n"
"  padding-top: 5px;\n"
"  padding-left: 13px;\n"
"}\n"
"QPushButton#btn_play {\n"
"  min-width: 96px;\n"
"  font-weight: 600;\n"
"  background: #1f9d55;\n"
"  border: 1px solid "
                        "#178045;\n"
"  color: white;\n"
"}\n"
"QPushButton#btn_play:hover {\n"
"  background: #25b25f;\n"
"  border: 1px solid #1c9650;\n"
"}\n"
"QPushButton#btn_play:pressed {\n"
"  background: #178045;\n"
"  border: 1px solid #126c3c;\n"
"}\n"
"QPushButton#btn_stop {\n"
"  background: #e25555;\n"
"  border: 1px solid #c74747;\n"
"  color: white;\n"
"}\n"
"QPushButton#btn_stop:hover {\n"
"  background: #f06a6a;\n"
"  border: 1px solid #d75858;\n"
"}\n"
"QPushButton#btn_stop:pressed {\n"
"  background: #c74747;\n"
"  border: 1px solid #a93a3a;\n"
"}\n"
"QSlider::groove:horizontal {\n"
"  border: 1px solid #c9d1de;\n"
"  height: 6px;\n"
"  border-radius: 3px;\n"
"  background: #e9edf4;\n"
"}\n"
"QSlider::handle:horizontal {\n"
"  background: #5b78b3;\n"
"  border: 1px solid #4b669a;\n"
"  width: 14px;\n"
"  margin: -5px 0;\n"
"  border-radius: 7px;\n"
"}"));
        verticalLayout_root = new QVBoxLayout(mediaplayerpage);
        verticalLayout_root->setSpacing(8);
        verticalLayout_root->setObjectName(QString::fromUtf8("verticalLayout_root"));
        verticalLayout_root->setContentsMargins(10, 10, 10, 10);
        frame_header = new QFrame(mediaplayerpage);
        frame_header->setObjectName(QString::fromUtf8("frame_header"));
        frame_header->setFrameShape(QFrame::StyledPanel);
        frame_header->setFrameShadow(QFrame::Raised);
        horizontalLayout_header = new QHBoxLayout(frame_header);
        horizontalLayout_header->setObjectName(QString::fromUtf8("horizontalLayout_header"));
        horizontalLayout_header->setContentsMargins(10, 6, 10, 6);
        lb_page_title = new QLabel(frame_header);
        lb_page_title->setObjectName(QString::fromUtf8("lb_page_title"));
        lb_page_title->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        horizontalLayout_header->addWidget(lb_page_title);

        horizontalSpacer_header = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_header->addItem(horizontalSpacer_header);

        lb_session = new QLabel(frame_header);
        lb_session->setObjectName(QString::fromUtf8("lb_session"));

        horizontalLayout_header->addWidget(lb_session);


        verticalLayout_root->addWidget(frame_header);

        frame_video_container = new QFrame(mediaplayerpage);
        frame_video_container->setObjectName(QString::fromUtf8("frame_video_container"));
        frame_video_container->setFrameShape(QFrame::StyledPanel);
        frame_video_container->setFrameShadow(QFrame::Sunken);
        verticalLayout_video = new QVBoxLayout(frame_video_container);
        verticalLayout_video->setObjectName(QString::fromUtf8("verticalLayout_video"));
        verticalLayout_video->setContentsMargins(0, 0, 0, 0);
        video_render_host = new QWidget(frame_video_container);
        video_render_host->setObjectName(QString::fromUtf8("video_render_host"));
        video_render_host->setMinimumSize(QSize(0, 340));
        verticalLayout_video_placeholder = new QVBoxLayout(video_render_host);
        verticalLayout_video_placeholder->setObjectName(QString::fromUtf8("verticalLayout_video_placeholder"));
        verticalLayout_video_placeholder->setContentsMargins(0, 0, 0, 0);
        lb_video_placeholder = new QLabel(video_render_host);
        lb_video_placeholder->setObjectName(QString::fromUtf8("lb_video_placeholder"));
        lb_video_placeholder->setAlignment(Qt::AlignCenter);

        verticalLayout_video_placeholder->addWidget(lb_video_placeholder);


        verticalLayout_video->addWidget(video_render_host);


        verticalLayout_root->addWidget(frame_video_container);

        frame_controls = new QFrame(mediaplayerpage);
        frame_controls->setObjectName(QString::fromUtf8("frame_controls"));
        frame_controls->setFrameShape(QFrame::StyledPanel);
        frame_controls->setFrameShadow(QFrame::Raised);
        verticalLayout_controls = new QVBoxLayout(frame_controls);
        verticalLayout_controls->setObjectName(QString::fromUtf8("verticalLayout_controls"));
        verticalLayout_controls->setContentsMargins(10, 8, 10, 8);
        horizontalLayout_controls_top = new QHBoxLayout();
        horizontalLayout_controls_top->setObjectName(QString::fromUtf8("horizontalLayout_controls_top"));
        btn_play = new QPushButton(frame_controls);
        btn_play->setObjectName(QString::fromUtf8("btn_play"));
        btn_play->setMinimumSize(QSize(96, 32));

        horizontalLayout_controls_top->addWidget(btn_play);

        btn_stop = new QPushButton(frame_controls);
        btn_stop->setObjectName(QString::fromUtf8("btn_stop"));
        btn_stop->setMinimumSize(QSize(72, 28));

        horizontalLayout_controls_top->addWidget(btn_stop);

        horizontalSpacer_controls_mid = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_controls_top->addItem(horizontalSpacer_controls_mid);

        lb_volume = new QLabel(frame_controls);
        lb_volume->setObjectName(QString::fromUtf8("lb_volume"));

        horizontalLayout_controls_top->addWidget(lb_volume);

        slider_volume = new QSlider(frame_controls);
        slider_volume->setObjectName(QString::fromUtf8("slider_volume"));
        slider_volume->setMinimumSize(QSize(120, 0));
        slider_volume->setMaximum(100);
        slider_volume->setValue(60);
        slider_volume->setOrientation(Qt::Horizontal);

        horizontalLayout_controls_top->addWidget(slider_volume);


        verticalLayout_controls->addLayout(horizontalLayout_controls_top);

        horizontalLayout_progress = new QHBoxLayout();
        horizontalLayout_progress->setObjectName(QString::fromUtf8("horizontalLayout_progress"));
        lb_time_current = new QLabel(frame_controls);
        lb_time_current->setObjectName(QString::fromUtf8("lb_time_current"));

        horizontalLayout_progress->addWidget(lb_time_current);

        slider_progress = new QSlider(frame_controls);
        slider_progress->setObjectName(QString::fromUtf8("slider_progress"));
        slider_progress->setTracking(true);
        slider_progress->setMinimum(0);
        slider_progress->setMaximum(1);
        slider_progress->setPageStep(1);
        slider_progress->setOrientation(Qt::Horizontal);

        horizontalLayout_progress->addWidget(slider_progress);

        lb_time_total = new QLabel(frame_controls);
        lb_time_total->setObjectName(QString::fromUtf8("lb_time_total"));

        horizontalLayout_progress->addWidget(lb_time_total);


        verticalLayout_controls->addLayout(horizontalLayout_progress);


        verticalLayout_root->addWidget(frame_controls);

        frame_status_bar = new QFrame(mediaplayerpage);
        frame_status_bar->setObjectName(QString::fromUtf8("frame_status_bar"));
        frame_status_bar->setMinimumSize(QSize(0, 30));
        frame_status_bar->setMaximumSize(QSize(16777215, 30));
        frame_status_bar->setFrameShape(QFrame::StyledPanel);
        frame_status_bar->setFrameShadow(QFrame::Raised);
        horizontalLayout_status = new QHBoxLayout(frame_status_bar);
        horizontalLayout_status->setObjectName(QString::fromUtf8("horizontalLayout_status"));
        horizontalLayout_status->setContentsMargins(10, 4, 10, 4);
        lb_status_text = new QLabel(frame_status_bar);
        lb_status_text->setObjectName(QString::fromUtf8("lb_status_text"));

        horizontalLayout_status->addWidget(lb_status_text);

        horizontalSpacer_status = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_status->addItem(horizontalSpacer_status);

        lb_stream_text = new QLabel(frame_status_bar);
        lb_stream_text->setObjectName(QString::fromUtf8("lb_stream_text"));

        horizontalLayout_status->addWidget(lb_stream_text);


        verticalLayout_root->addWidget(frame_status_bar);


        retranslateUi(mediaplayerpage);

        QMetaObject::connectSlotsByName(mediaplayerpage);
    } // setupUi

    void retranslateUi(QDialog *mediaplayerpage)
    {
        mediaplayerpage->setWindowTitle(QApplication::translate("mediaplayerpage", "Media Player", nullptr));
        lb_page_title->setText(QApplication::translate("mediaplayerpage", "Player Workspace", nullptr));
        lb_session->setText(QApplication::translate("mediaplayerpage", "Session: room_default", nullptr));
        lb_video_placeholder->setText(QApplication::translate("mediaplayerpage", "Video Render Area", nullptr));
        btn_play->setText(QApplication::translate("mediaplayerpage", "\342\226\266 \346\222\255\346\224\276", nullptr));
        btn_stop->setText(QApplication::translate("mediaplayerpage", "Stop", nullptr));
        lb_volume->setText(QApplication::translate("mediaplayerpage", "Volume", nullptr));
        lb_time_current->setText(QApplication::translate("mediaplayerpage", "00:00:00", nullptr));
        lb_time_total->setText(QApplication::translate("mediaplayerpage", "00:00:00", nullptr));
        lb_status_text->setText(QApplication::translate("mediaplayerpage", "Status: Idle", nullptr));
        lb_stream_text->setText(QApplication::translate("mediaplayerpage", "Stream: -", nullptr));
    } // retranslateUi

};

namespace Ui {
    class mediaplayerpage: public Ui_mediaplayerpage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MEDIAPLAYERPAGE_H

/********************************************************************************
** Form generated from reading UI file 'chatdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.10
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CHATDIALOG_H
#define UI_CHATDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <applyfriendpage.h>
#include <chatpage.h>
#include <contactuserlist.h>
#include <friendinfopage.h>
#include <mediaplayerpage.h>
#include <statewidget.h>
#include "chatuserlist.h"
#include "clickedbtn.h"
#include "customizeedit.h"
#include "searchlist.h"

QT_BEGIN_NAMESPACE

class Ui_ChatDialog
{
public:
    QHBoxLayout *horizontalLayout;
    QWidget *side_bar;
    QVBoxLayout *verticalLayout_4;
    QWidget *widget_2;
    QVBoxLayout *verticalLayout_5;
    QLabel *side_head_lb;
    StateWidget *side_chat_lb;
    StateWidget *side_player_lb;
    StateWidget *side_contact_lb;
    QWidget *widget;
    QSpacerItem *verticalSpacer;
    QWidget *chat_user_wid;
    QVBoxLayout *verticalLayout;
    QStackedWidget *stackedWidget_2;
    QWidget *page;
    QVBoxLayout *verticalLayout_2;
    QWidget *search_wid;
    QHBoxLayout *horizontalLayout_2;
    CustomizeEdit *search_edit;
    QSpacerItem *horizontalSpacer;
    ClickedBtn *add_btn;
    SearchList *search_list;
    ChatUserList *chat_user_list;
    ContactUserList *con_user_list;
    QWidget *page_2;
    QVBoxLayout *verticalLayout_media_left;
    QFrame *frame_stream_search;
    QHBoxLayout *horizontalLayout_stream_search;
    QLineEdit *edit_stream_search;
    QPushButton *btn_stream_search;
    QLabel *lb_stream_list_title;
    QListWidget *list_streams;
    QFrame *frame_session_bar;
    QHBoxLayout *horizontalLayout_session_bar;
    QPushButton *btn_create_session;
    QPushButton *btn_join_session;
    QLabel *lb_session_list_title;
    QListWidget *list_sessions;
    QFrame *frame_collab_info;
    QVBoxLayout *verticalLayout_collab_info;
    QLabel *lb_collab_title;
    QLabel *lb_collab_master;
    QLabel *lb_collab_mode;
    QStackedWidget *stackedWidget;
    ChatPage *chat_page;
    ApplyFriendPage *friend_apply_page;
    FriendInfoPage *friend_info_page;
    mediaplayerpage *media_player_page;

    void setupUi(QDialog *ChatDialog)
    {
        if (ChatDialog->objectName().isEmpty())
            ChatDialog->setObjectName(QString::fromUtf8("ChatDialog"));
        ChatDialog->resize(594, 690);
        horizontalLayout = new QHBoxLayout(ChatDialog);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        side_bar = new QWidget(ChatDialog);
        side_bar->setObjectName(QString::fromUtf8("side_bar"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(side_bar->sizePolicy().hasHeightForWidth());
        side_bar->setSizePolicy(sizePolicy);
        side_bar->setMinimumSize(QSize(56, 0));
        side_bar->setMaximumSize(QSize(56, 16777215));
        verticalLayout_4 = new QVBoxLayout(side_bar);
        verticalLayout_4->setSpacing(30);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        verticalLayout_4->setContentsMargins(10, 30, 0, 0);
        widget_2 = new QWidget(side_bar);
        widget_2->setObjectName(QString::fromUtf8("widget_2"));
        widget_2->setMinimumSize(QSize(29, 29));
        verticalLayout_5 = new QVBoxLayout(widget_2);
        verticalLayout_5->setSpacing(30);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        verticalLayout_5->setContentsMargins(0, 0, 0, 0);
        side_head_lb = new QLabel(widget_2);
        side_head_lb->setObjectName(QString::fromUtf8("side_head_lb"));
        side_head_lb->setMinimumSize(QSize(35, 35));
        side_head_lb->setMaximumSize(QSize(35, 35));

        verticalLayout_5->addWidget(side_head_lb);

        side_chat_lb = new StateWidget(widget_2);
        side_chat_lb->setObjectName(QString::fromUtf8("side_chat_lb"));
        side_chat_lb->setMinimumSize(QSize(30, 30));
        side_chat_lb->setMaximumSize(QSize(30, 30));

        verticalLayout_5->addWidget(side_chat_lb);

        side_player_lb = new StateWidget(widget_2);
        side_player_lb->setObjectName(QString::fromUtf8("side_player_lb"));
        side_player_lb->setMinimumSize(QSize(30, 30));
        side_player_lb->setMaximumSize(QSize(30, 30));

        verticalLayout_5->addWidget(side_player_lb);

        side_contact_lb = new StateWidget(widget_2);
        side_contact_lb->setObjectName(QString::fromUtf8("side_contact_lb"));
        side_contact_lb->setMinimumSize(QSize(30, 30));
        side_contact_lb->setMaximumSize(QSize(30, 30));

        verticalLayout_5->addWidget(side_contact_lb);


        verticalLayout_4->addWidget(widget_2);

        widget = new QWidget(side_bar);
        widget->setObjectName(QString::fromUtf8("widget"));

        verticalLayout_4->addWidget(widget);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_4->addItem(verticalSpacer);


        horizontalLayout->addWidget(side_bar);

        chat_user_wid = new QWidget(ChatDialog);
        chat_user_wid->setObjectName(QString::fromUtf8("chat_user_wid"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(chat_user_wid->sizePolicy().hasHeightForWidth());
        chat_user_wid->setSizePolicy(sizePolicy1);
        chat_user_wid->setMinimumSize(QSize(250, 0));
        chat_user_wid->setMaximumSize(QSize(250, 16777215));
        verticalLayout = new QVBoxLayout(chat_user_wid);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        stackedWidget_2 = new QStackedWidget(chat_user_wid);
        stackedWidget_2->setObjectName(QString::fromUtf8("stackedWidget_2"));
        stackedWidget_2->setMinimumSize(QSize(250, 0));
        page = new QWidget();
        page->setObjectName(QString::fromUtf8("page"));
        verticalLayout_2 = new QVBoxLayout(page);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        search_wid = new QWidget(page);
        search_wid->setObjectName(QString::fromUtf8("search_wid"));
        search_wid->setMinimumSize(QSize(0, 60));
        search_wid->setMaximumSize(QSize(16777215, 60));
        horizontalLayout_2 = new QHBoxLayout(search_wid);
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(9, 9, 9, 9);
        search_edit = new CustomizeEdit(search_wid);
        search_edit->setObjectName(QString::fromUtf8("search_edit"));
        search_edit->setMinimumSize(QSize(0, 24));
        search_edit->setMaximumSize(QSize(16777215, 24));

        horizontalLayout_2->addWidget(search_edit);

        horizontalSpacer = new QSpacerItem(5, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        add_btn = new ClickedBtn(search_wid);
        add_btn->setObjectName(QString::fromUtf8("add_btn"));
        add_btn->setMinimumSize(QSize(24, 24));
        add_btn->setMaximumSize(QSize(24, 24));

        horizontalLayout_2->addWidget(add_btn);


        verticalLayout_2->addWidget(search_wid);

        search_list = new SearchList(page);
        search_list->setObjectName(QString::fromUtf8("search_list"));

        verticalLayout_2->addWidget(search_list);

        chat_user_list = new ChatUserList(page);
        chat_user_list->setObjectName(QString::fromUtf8("chat_user_list"));
        chat_user_list->setMinimumSize(QSize(250, 0));
        chat_user_list->setMaximumSize(QSize(250, 16777215));

        verticalLayout_2->addWidget(chat_user_list);

        con_user_list = new ContactUserList(page);
        con_user_list->setObjectName(QString::fromUtf8("con_user_list"));

        verticalLayout_2->addWidget(con_user_list);

        stackedWidget_2->addWidget(page);
        page_2 = new QWidget();
        page_2->setObjectName(QString::fromUtf8("page_2"));
        verticalLayout_media_left = new QVBoxLayout(page_2);
        verticalLayout_media_left->setSpacing(8);
        verticalLayout_media_left->setObjectName(QString::fromUtf8("verticalLayout_media_left"));
        verticalLayout_media_left->setContentsMargins(8, 8, 8, 8);
        frame_stream_search = new QFrame(page_2);
        frame_stream_search->setObjectName(QString::fromUtf8("frame_stream_search"));
        frame_stream_search->setFrameShape(QFrame::StyledPanel);
        horizontalLayout_stream_search = new QHBoxLayout(frame_stream_search);
        horizontalLayout_stream_search->setObjectName(QString::fromUtf8("horizontalLayout_stream_search"));
        horizontalLayout_stream_search->setContentsMargins(6, 6, 6, 6);
        edit_stream_search = new QLineEdit(frame_stream_search);
        edit_stream_search->setObjectName(QString::fromUtf8("edit_stream_search"));

        horizontalLayout_stream_search->addWidget(edit_stream_search);

        btn_stream_search = new QPushButton(frame_stream_search);
        btn_stream_search->setObjectName(QString::fromUtf8("btn_stream_search"));
        btn_stream_search->setMinimumSize(QSize(56, 26));

        horizontalLayout_stream_search->addWidget(btn_stream_search);


        verticalLayout_media_left->addWidget(frame_stream_search);

        lb_stream_list_title = new QLabel(page_2);
        lb_stream_list_title->setObjectName(QString::fromUtf8("lb_stream_list_title"));

        verticalLayout_media_left->addWidget(lb_stream_list_title);

        list_streams = new QListWidget(page_2);
        list_streams->setObjectName(QString::fromUtf8("list_streams"));

        verticalLayout_media_left->addWidget(list_streams);

        frame_session_bar = new QFrame(page_2);
        frame_session_bar->setObjectName(QString::fromUtf8("frame_session_bar"));
        frame_session_bar->setFrameShape(QFrame::StyledPanel);
        horizontalLayout_session_bar = new QHBoxLayout(frame_session_bar);
        horizontalLayout_session_bar->setObjectName(QString::fromUtf8("horizontalLayout_session_bar"));
        horizontalLayout_session_bar->setContentsMargins(6, 6, 6, 6);
        btn_create_session = new QPushButton(frame_session_bar);
        btn_create_session->setObjectName(QString::fromUtf8("btn_create_session"));

        horizontalLayout_session_bar->addWidget(btn_create_session);

        btn_join_session = new QPushButton(frame_session_bar);
        btn_join_session->setObjectName(QString::fromUtf8("btn_join_session"));

        horizontalLayout_session_bar->addWidget(btn_join_session);


        verticalLayout_media_left->addWidget(frame_session_bar);

        lb_session_list_title = new QLabel(page_2);
        lb_session_list_title->setObjectName(QString::fromUtf8("lb_session_list_title"));

        verticalLayout_media_left->addWidget(lb_session_list_title);

        list_sessions = new QListWidget(page_2);
        list_sessions->setObjectName(QString::fromUtf8("list_sessions"));
        list_sessions->setMinimumSize(QSize(0, 120));

        verticalLayout_media_left->addWidget(list_sessions);

        frame_collab_info = new QFrame(page_2);
        frame_collab_info->setObjectName(QString::fromUtf8("frame_collab_info"));
        frame_collab_info->setFrameShape(QFrame::StyledPanel);
        verticalLayout_collab_info = new QVBoxLayout(frame_collab_info);
        verticalLayout_collab_info->setObjectName(QString::fromUtf8("verticalLayout_collab_info"));
        verticalLayout_collab_info->setContentsMargins(8, 6, 8, 6);
        lb_collab_title = new QLabel(frame_collab_info);
        lb_collab_title->setObjectName(QString::fromUtf8("lb_collab_title"));

        verticalLayout_collab_info->addWidget(lb_collab_title);

        lb_collab_master = new QLabel(frame_collab_info);
        lb_collab_master->setObjectName(QString::fromUtf8("lb_collab_master"));

        verticalLayout_collab_info->addWidget(lb_collab_master);

        lb_collab_mode = new QLabel(frame_collab_info);
        lb_collab_mode->setObjectName(QString::fromUtf8("lb_collab_mode"));

        verticalLayout_collab_info->addWidget(lb_collab_mode);


        verticalLayout_media_left->addWidget(frame_collab_info);

        stackedWidget_2->addWidget(page_2);

        verticalLayout->addWidget(stackedWidget_2);


        horizontalLayout->addWidget(chat_user_wid);

        stackedWidget = new QStackedWidget(ChatDialog);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setMinimumSize(QSize(50, 0));
        chat_page = new ChatPage();
        chat_page->setObjectName(QString::fromUtf8("chat_page"));
        stackedWidget->addWidget(chat_page);
        friend_apply_page = new ApplyFriendPage();
        friend_apply_page->setObjectName(QString::fromUtf8("friend_apply_page"));
        stackedWidget->addWidget(friend_apply_page);
        friend_info_page = new FriendInfoPage();
        friend_info_page->setObjectName(QString::fromUtf8("friend_info_page"));
        stackedWidget->addWidget(friend_info_page);
        media_player_page = new mediaplayerpage();
        media_player_page->setObjectName(QString::fromUtf8("media_player_page"));
        stackedWidget->addWidget(media_player_page);

        horizontalLayout->addWidget(stackedWidget);


        retranslateUi(ChatDialog);

        stackedWidget_2->setCurrentIndex(0);
        stackedWidget->setCurrentIndex(2);


        QMetaObject::connectSlotsByName(ChatDialog);
    } // setupUi

    void retranslateUi(QDialog *ChatDialog)
    {
        ChatDialog->setWindowTitle(QApplication::translate("ChatDialog", "Dialog", nullptr));
        side_head_lb->setText(QString());
        add_btn->setText(QString());
        edit_stream_search->setPlaceholderText(QApplication::translate("ChatDialog", "???ID / ??", nullptr));
        btn_stream_search->setText(QApplication::translate("ChatDialog", "??", nullptr));
        lb_stream_list_title->setText(QApplication::translate("ChatDialog", "??????", nullptr));
        btn_create_session->setText(QApplication::translate("ChatDialog", "????", nullptr));
        btn_join_session->setText(QApplication::translate("ChatDialog", "????", nullptr));
        lb_session_list_title->setText(QApplication::translate("ChatDialog", "????", nullptr));
        lb_collab_title->setText(QApplication::translate("ChatDialog", "????", nullptr));
        lb_collab_master->setText(QApplication::translate("ChatDialog", "??: -", nullptr));
        lb_collab_mode->setText(QApplication::translate("ChatDialog", "??: ???", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ChatDialog: public Ui_ChatDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CHATDIALOG_H

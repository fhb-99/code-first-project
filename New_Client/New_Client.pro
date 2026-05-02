QT       += core gui network opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
RC_ICONS = icon.ico

INCLUDEPATH += \
    $$PWD/core \
    $$PWD/data \
    $$PWD/media \
    $$PWD/net \
    $$PWD/ui \
    $$PWD/ui/dialogs \
    $$PWD/ui/items \
    $$PWD/ui/pages \
    $$PWD/ui/widgets

# FFmpeg 头文件（与 hplayer-master/3rd 目录结构一致：<libavcodec/...>）
INCLUDEPATH += $$PWD/3rd/include

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    core/global.cpp \
    data/userdata.cpp \
    data/usermgr.cpp \
    main.cpp \
    mainwindow.cpp \
    media/decodepipeline.cpp \
    media/mediapipeline.cpp \
    media/streamcontroller.cpp \
    net/httpmgr.cpp \
    net/tcpmgr.cpp \
    ui/dialogs/chatdialog.cpp \
    ui/dialogs/findfaildlg.cpp \
    ui/dialogs/findsuccessdlg.cpp \
    ui/dialogs/loadingdlg.cpp \
    ui/dialogs/logindialog.cpp \
    ui/dialogs/registerdialog.cpp \
    ui/dialogs/resetdialog.cpp \
    ui/items/ChatItemBase.cpp \
    ui/items/adduseritem.cpp \
    ui/items/applyfrienditem.cpp \
    ui/items/conuseritem.cpp \
    ui/items/grouptipitem.cpp \
    ui/items/invaliditem.cpp \
    ui/items/lineitem.cpp \
    ui/items/listitembase.cpp \
    ui/pages/applyfriend.cpp \
    ui/pages/applyfriendlist.cpp \
    ui/pages/applyfriendpage.cpp \
    ui/pages/authenfriend.cpp \
    ui/pages/chatpage.cpp \
    ui/pages/friendinfopage.cpp \
    ui/pages/mediaplayerpage.cpp \
    ui/widgets/BubbleFrame.cpp \
    ui/widgets/ChatView.cpp \
    ui/widgets/MessageTextEdit.cpp \
    ui/widgets/PictureBubble.cpp \
    ui/widgets/TextBubble.cpp \
    ui/widgets/chatuserlist.cpp \
    ui/widgets/chatuserwid.cpp \
    ui/widgets/clickedbtn.cpp \
    ui/widgets/clickedlabel.cpp \
    ui/widgets/clickedoncelabel.cpp \
    ui/widgets/contactuserlist.cpp \
    ui/widgets/customizeedit.cpp \
    ui/widgets/customizetextedit.cpp \
    ui/widgets/friendlabel.cpp \
    ui/widgets/searchlist.cpp \
    ui/widgets/statelabel.cpp \
    ui/widgets/statewidget.cpp \
    ui/widgets/timerbtn.cpp

HEADERS += \
    core/ffmpeg_util.h \
    core/global.h \
    core/singleton.h \
    data/userdata.h \
    data/usermgr.h \
    mainwindow.h \
    media/decodepipeline.h \
    media/mediapipeline.h \
    media/streamcontroller.h \
    net/httpmgr.h \
    net/tcpmgr.h \
    ui/dialogs/chatdialog.h \
    ui/dialogs/findfaildlg.h \
    ui/dialogs/findsuccessdlg.h \
    ui/dialogs/loadingdlg.h \
    ui/dialogs/logindialog.h \
    ui/dialogs/registerdialog.h \
    ui/dialogs/resetdialog.h \
    ui/items/ChatItemBase.h \
    ui/items/adduseritem.h \
    ui/items/applyfrienditem.h \
    ui/items/conuseritem.h \
    ui/items/grouptipitem.h \
    ui/items/invaliditem.h \
    ui/items/lineitem.h \
    ui/items/listitembase.h \
    ui/pages/applyfriend.h \
    ui/pages/applyfriendlist.h \
    ui/pages/applyfriendpage.h \
    ui/pages/authenfriend.h \
    ui/pages/chatpage.h \
    ui/pages/friendinfopage.h \
    ui/pages/mediaplayerpage.h \
    ui/widgets/BubbleFrame.h \
    ui/widgets/ChatView.h \
    ui/widgets/MessageTextEdit.h \
    ui/widgets/PictureBubble.h \
    ui/widgets/TextBubble.h \
    ui/widgets/chatuserlist.h \
    ui/widgets/chatuserwid.h \
    ui/widgets/clickedbtn.h \
    ui/widgets/clickedlabel.h \
    ui/widgets/clickedoncelabel.h \
    ui/widgets/contactuserlist.h \
    ui/widgets/customizeedit.h \
    ui/widgets/customizetextedit.h \
    ui/widgets/friendlabel.h \
    ui/widgets/searchlist.h \
    ui/widgets/statelabel.h \
    ui/widgets/statewidget.h \
    ui/widgets/timerbtn.h

FORMS += \
    mainwindow.ui \
    ui/dialogs/chatdialog.ui \
    ui/dialogs/findfaildlg.ui \
    ui/dialogs/findsuccessdlg.ui \
    ui/dialogs/loadingdlg.ui \
    ui/dialogs/logindialog.ui \
    ui/dialogs/registerdialog.ui \
    ui/dialogs/resetdialog.ui \
    ui/items/adduseritem.ui \
    ui/items/applyfrienditem.ui \
    ui/items/conuseritem.ui \
    ui/items/grouptipitem.ui \
    ui/items/lineitem.ui \
    ui/pages/applyfriend.ui \
    ui/pages/applyfriendpage.ui \
    ui/pages/authenfriend.ui \
    ui/pages/chatpage.ui \
    ui/pages/friendinfopage.ui \
    ui/pages/mediaplayerpage.ui \
    ui/widgets/chatuserwid.ui \
    ui/widgets/friendlabel.ui

RESOURCES += \
    rc.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    config/config.ini \
    res/add_friend.png \
    res/add_friend_hover.png \
    res/add_friend_normal.png \
    res/addtip.png \
    res/arowdown.png \
    res/chat_icon.png \
    res/chat_icon_hover.png \
    res/chat_icon_press.png \
    res/chat_icon_select_hover.png \
    res/chat_icon_select_press.png \
    res/close_search.png \
    res/close_transparent.png \
    res/contact_list.png \
    res/contact_list_hover.png \
    res/contact_list_press.png \
    res/female.png \
    res/filedir.png \
    res/filedir_hover.png \
    res/filedir_press.png \
    res/head_1.jpg \
    res/head_2.jpg \
    res/head_3.jpg \
    res/head_4.jpg \
    res/head_5.jpg \
    res/ice.png \
    res/loading.gif \
    res/male.png \
    res/msg_chat_hover.png \
    res/msg_chat_normal.png \
    res/msg_chat_press.png \
    res/red_point.png \
    res/right_tip.png \
    res/search.png \
    res/smile.png \
    res/smile_hover.png \
    res/smile_press.png \
    res/tipclose.png \
    res/unvisible.png \
    res/unvisible_hover.png \
    res/video_chat_hover.png \
    res/video_chat_normal.png \
    res/video_chat_press.png \
    res/visible.png \
    res/visible_hover.png \
    res/voice_chat_hover.png \
    res/voice_chat_normal.png \
    res/voice_chat_press.png


CONFIG(debug, debug|release) {
        #debug
    message("debug mode")
    #指定要拷贝的文件目录为工程目录下release目录下的所有dll、lib文件，例如工程目录在D:\QT\Test
    #PWD就为D:/QT/Test，DllFile = D:/QT/Test/release/*.dll
    TargetConfig = $${PWD}/config/config.ini
    #将输入目录中的"/"替换为"\"
    TargetConfig = $$replace(TargetConfig, /, \\)
    #将输出目录中的"/"替换为"\"
    OutputDir =  $${OUT_PWD}/$${DESTDIR}
    OutputDir = $$replace(OutputDir, /, \\)
    //执行copy命令
    QMAKE_POST_LINK += copy /Y \"$$TargetConfig\" \"$$OutputDir\" &

    # 首先，定义static文件夹的路径
    StaticDir = $${PWD}/static
    # 将路径中的"/"替换为"\"
    StaticDir = $$replace(StaticDir, /, \\)
    #message($${StaticDir})
    # 使用xcopy命令拷贝文件夹，/E表示拷贝子目录及其内容，包括空目录。/I表示如果目标不存在则创建目录。/Y表示覆盖现有文件而不提示。
     QMAKE_POST_LINK += xcopy /Y /E /I \"$$StaticDir\" \"$$OutputDir\\static\\\"
}else{
      #release
    message("release mode")
    #指定要拷贝的文件目录为工程目录下release目录下的所有dll、lib文件，例如工程目录在D:\QT\Test
    #PWD就为D:/QT/Test，DllFile = D:/QT/Test/release/*.dll
    TargetConfig = $${PWD}/config/config.ini
    #将输入目录中的"/"替换为"\"
    TargetConfig = $$replace(TargetConfig, /, \\)
    #将输出目录中的"/"替换为"\"
    OutputDir =  $${OUT_PWD}/$${DESTDIR}
    OutputDir = $$replace(OutputDir, /, \\)
    //执行copy命令
    QMAKE_POST_LINK += copy /Y \"$$TargetConfig\" \"$$OutputDir\" &

    # 首先，定义static文件夹的路径
    StaticDir = $${PWD}/static
    # 将路径中的"/"替换为"\"
    StaticDir = $$replace(StaticDir, /, \\)
    #message($${StaticDir})
    # 使用xcopy命令拷贝文件夹，/E表示拷贝子目录及其内容，包括空目录。/I表示如果目标不存在则创建目录。/Y表示覆盖现有文件而不提示。
     QMAKE_POST_LINK += xcopy /Y /E /I \"$$StaticDir\" \"$$OutputDir\\static\\\"
}

win32-msvc* {
    QMAKE_CXXFLAGS += /wd"4819" /utf-8
    # FFmpeg 导入库（与 HPlayer 相同：msvc2015_x64，一般可被 VS2017 链接）
    LIBS += -L$$PWD/3rd/lib/msvc2015_x64
    LIBS += -lavformat -lavdevice -lavcodec -lswresample -lswscale -lavutil
    # SDL2（头文件在 $$PWD/3rd/include/SDL2，运行时请将 SDL2.dll 放入 ffmpeg_runtime 一并拷出）
    LIBS += -lSDL2
    LIBS += -lopengl32 -lglu32
    # 将 FFmpeg 运行库拷到 exe 同级：把下列 DLL 放入本目录下 ffmpeg_runtime/（见说明）
    exists($$PWD/ffmpeg_runtime) {
        FFMPEG_RT = $$PWD/ffmpeg_runtime
        FFMPEG_RT = $$replace(FFMPEG_RT, /, \\)
        OutDllDir = $$OUT_PWD/$$DESTDIR
        OutDllDir = $$replace(OutDllDir, /, \\)
        QMAKE_POST_LINK += xcopy /Y /Q \"$$FFMPEG_RT\\*.dll\" \"$$OutDllDir\\\" &
    }
}

/***************************************************************************
                          kgpgeditor.h  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
    email                : bj@altern.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KGPGEDITOR_H__
#define __KGPGEDITOR_H__

#include <QStringList>

#include <kmainwindow.h>
#include <kurl.h>

class QCloseEvent;

class KToggleAction;
class KAction;

class KgpgInterface;
class KgpgView;

class KgpgEditor : public KMainWindow
{
    Q_OBJECT
    friend class KgpgView;

public:
    KgpgEditor(QWidget *parent = 0, const char *name = 0, Qt::WFlags f = 0, KShortcut goHome = QKeySequence(Qt::CTRL + Qt::Key_Home), bool mainWindow = false);
    ~KgpgEditor();

    void openDocumentFile(const KURL& url, QString encoding = QString::null);
    void openEncryptedDocumentFile(const KURL& url);

    KgpgView *view;

signals:
    void refreshImported(QStringList);
    void openChangeFont();
    void openConfigDialog();
    void encryptFiles(KURL::List fileList);

public slots:
    void slotSetFont(QFont myFont);
    void closeWindow();

protected:
    void readOptions(bool doresize = true);
    void saveOptions();
    void initActions();
    void initView();
    void closeEvent(QCloseEvent *e);

private slots:
    // File menu
    void slotFileNew();
    void slotFileOpen();
    void slotFileSave();
    void slotFileSaveAs();
    void slotFilePrint();
    void slotFilePreEnc();
    void slotFilePreDec();
    void slotKeyManager();
    void slotFileQuit();

    // Edit menu
    void slotundo();
    void slotredo();
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotSelectAll();

    // Coding menu
    void slotSetCharset();
    void slotResetEncoding(bool enc);
    bool checkEncoding(QTextCodec *codec);

    // Signing menu
    void slotPreSignFile();
    void slotSignFile(KURL url);
    void slotSignFileFin(KgpgInterface *interface);
    void slotPreVerifyFile();
    void slotVerifyFile(KURL url);
    void slotCheckMd5();
    void importSignatureKey(QString id);

    // Options menu
    void slotOptions();

    void modified();
    void newText();

private:
    QStringList m_customdecrypt;
    QString m_textencoding;

    KToggleAction *m_encodingaction;
    KAction *m_filesave;
    KAction *m_editundo;
    KAction *m_editredo;
    KShortcut goDefaultKey;
    KURL m_docname;

    bool m_ismainwindow;
};

#endif // __KGPGEDITOR_H__

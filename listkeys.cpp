/***************************************************************************
                          listkeys.cpp  -  description
                             -------------------
    begin                : Thu Jul 4 2002
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

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <QDragMoveEvent>
#include <QDesktopWidget>
#include <QRadioButton>
#include <QKeySequence>
#include <QTextStream>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QToolButton>
#include <QClipboard>
#include <QTextCodec>
#include <QTabWidget>
#include <QDropEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QVariant>
#include <QPainter>
#include <QToolTip>
#include <QString>
#include <QPixmap>
#include <QLayout>
#include <QRegExp>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QEvent>
#include <QFlags>
#include <QMovie>
#include <QList>
#include <QFile>
#include <QDir>

#include <Q3TextDrag>

#include <kabc/addresseedialog.h>
#include <kabc/stdaddressbook.h>
#include <ktoolinvocation.h>
#include <kurlrequester.h>
#include <kio/netaccess.h>
#include <kstandarddirs.h>
#include <kpassivepopup.h>
#include <kdesktopfile.h>
#include <kapplication.h>
#include <kinputdialog.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfinddialog.h>
#include <kstatusbar.h>
#include <dcopclient.h>
#include <ktempfile.h>
#include <klineedit.h>
#include <kmimetype.h>
#include <kshortcut.h>
#include <kstdaccel.h>
#include <kprocess.h>
#include <kpassdlg.h>
#include <kprinter.h>
#include <klocale.h>
#include <dcopref.h>
#include <kprocio.h>
#include <kaction.h>
#include <kdebug.h>
#include <kfind.h>
#include <kmenu.h>
#include <kurl.h>
#include <ktip.h>
#include <krun.h>
#include <kwin.h>

#include "selectsecretkey.h"
#include "newkey.h"
#include "kgpg.h"
#include "kgpgeditor.h"
#include "kgpgview.h"
#include "listkeys.h"
#include "keyexport.h"
#include "sourceselect.h"
#include "adduid.h"
#include "groupedit.h"
#include "kgpgrevokewidget.h"
#include "keyservers.h"
#include "keyserver.h"
#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "keygener.h"
#include "kgpgoptions.h"
#include "keyinfowidget.h"
#include "kgpglibrary.h"

class UpdateViewItem : public KListViewItem
{
public:
    enum ItemType
    {
        Group = 1,
        Public = 2,
        Secret = 4,
        Sub = 8,
        Uid = 16,
        Uat = 32,
        Rev = 64,
        Sign = 128,
    };

    UpdateViewItem(KListView *parent = 0, QString name = QString::null, QString email = QString::null, QString trust = QString::null, QString expiration = QString::null, QString size = QString::null, QString creation = QString::null, QString id = QString::null, bool isdefault = false, bool isexpired = false, ItemType type = Public);
    UpdateViewItem(KListViewItem *parent = 0, QString name = QString::null, QString email = QString::null, QString trust = QString::null, QString expiration = QString::null, QString size = QString::null, QString creation = QString::null, QString id = QString::null, bool isdefault = false, bool isexpired = false, ItemType type = Public);

    void setItemType(const ItemType &type);
    ItemType itemType() const;

    void setDefault(const bool &def);
    bool isDefault() const;

    void setExpired(const bool &exp);
    bool isExpired() const;

    virtual void paintCell(QPainter *p, const QColorGroup &cg, int col, int width, int align);
    virtual int compare(Q3ListViewItem *item, int c, bool ascending) const;
    virtual QString key(int column, bool) const;

    bool m_def;
    bool m_exp;
    ItemType m_type;
};

UpdateViewItem::UpdateViewItem(KListView *parent, QString name, QString email, QString trust, QString expiration, QString size, QString creation, QString id, bool isdefault, bool isexpired, ItemType type)
              : KListViewItem(parent)
{
    m_def = isdefault;
    m_exp = isexpired;
    m_type = type;
    setText(0, name);
    setText(1, email);
    setText(2, trust);
    setText(3, expiration);
    setText(4, size);
    setText(5, creation);
    setText(6, id);
}

UpdateViewItem::UpdateViewItem(KListViewItem *parent, QString name, QString email, QString trust, QString expiration, QString size, QString creation, QString id, bool isdefault, bool isexpired, ItemType type)
              : KListViewItem(parent)
{
    m_def = isdefault;
    m_exp = isexpired;
    m_type = type;
    setText(0, name);
    setText(1, email);
    setText(2, trust);
    setText(3, expiration);
    setText(4, size);
    setText(5, creation);
    setText(6, id);
}

void UpdateViewItem::setItemType(const ItemType &type)
{
    m_type = type;
}

UpdateViewItem::ItemType UpdateViewItem::itemType() const
{
    return m_type;
}

void UpdateViewItem::setDefault(const bool &def)
{
    m_def = def;
}

bool UpdateViewItem::isDefault() const
{
    return m_def;
}

void UpdateViewItem::setExpired(const bool &exp)
{
    m_exp = exp;
}

bool UpdateViewItem::isExpired() const
{
    return m_exp;
}

void UpdateViewItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment)
{
    QColorGroup _cg(cg);

    if (itemType() & Group)
        return;

    if (itemType() & Public)
    {
        if (m_def && (column < 2))
        {
            QFont font(p->font());
            font.setBold(true);
            p->setFont(font);
        }
        else
        if (m_exp && (column == 3))
            _cg.setColor(QColorGroup::Text, Qt::red);
    }
    else
    if (column < 2)
    {
        QFont font(p->font());
        font.setItalic(true);
        p->setFont(font);
    }

    KListViewItem::paintCell(p, _cg, column, width, alignment);
}

int UpdateViewItem::compare(Q3ListViewItem *item2, int c, bool ascending) const
{
    KListViewItem *item = static_cast<KListViewItem*>(item2);

    if (c == 0)
    {
        ItemType item1 = itemType();
        ItemType item2 = static_cast<UpdateViewItem*>(item)->itemType();

        if (item1 < item2) return -1;
        if (item1 > item2) return 1;

        return KListViewItem::compare(item, c, ascending);
    }

    if ((c == 3) || (c == 5))  // by (3) expiration date or (5) creation date
    {
        QDate d = KGlobal::locale()->readDate(text(c));
        QDate itemDate = KGlobal::locale()->readDate(item->text(c));

        bool thisDateValid = d.isValid();
        bool itemDateValid = itemDate.isValid();

        if (thisDateValid)
        {
            if (itemDateValid)
            {
                if (d < itemDate) return -1;
                if (d > itemDate) return  1;
            }
            else
                return -1;
        }
        else
        if (itemDateValid)
            return 1;

        return 0;
    }

    if (c == 2)  // sorting by pixmap
    {
        const QPixmap* pix = pixmap(c);
        const QPixmap* itemPix = item->pixmap(c);

        int serial;
        int itemSerial;

        if (!pix)
            serial = 0;
        else
            serial = pix->serialNumber();

        if (!itemPix)
            itemSerial = 0;
        else
            itemSerial = itemPix->serialNumber();

        if (serial < itemSerial) return -1;
        if (serial > itemSerial) return  1;
        return 0;
    }

    return KListViewItem::compare(item, c, ascending);
}

QString UpdateViewItem::key(int column, bool) const
{
    return text(column).toLower();
}

KeyView::KeyView(QWidget *parent)
       : KListView(parent)
{
    setRootIsDecorated(true);
    addColumn(i18n("Name"), 200);
    addColumn(i18n("Email"), 200);
    addColumn(i18n("Trust"), 60);
    addColumn(i18n("Expiration"), 100);
    addColumn(i18n("Size"), 100);
    addColumn(i18n("Creation"), 100);
    addColumn(i18n("Id"), 100);
    setShowSortIndicator(true);
    setAllColumnsShowFocus(true);
    setFullWidth(true);
    setAcceptDrops(true) ;
    setSelectionModeExt(Extended);

    KIconLoader *loader = KGlobal::iconLoader();
    pixkeyOrphan = loader->loadIcon("kgpg_key4", KIcon::Small, 20);
    pixkeyGroup = loader->loadIcon("kgpg_key3", KIcon::Small, 20);
    pixkeyPair = loader->loadIcon("kgpg_key2", KIcon::Small, 20);
    pixkeySingle = loader->loadIcon("kgpg_key1", KIcon::Small, 20);
    pixsignature = loader->loadIcon("signature", KIcon::Small, 20);
    pixuserid = loader->loadIcon("kgpg_identity", KIcon::Small, 20);
    pixuserphoto = loader->loadIcon("kgpg_photo", KIcon::Small, 20);
    pixRevoke = loader->loadIcon("stop", KIcon::Small, 20);

    QPixmap blankFrame;
    blankFrame.load(locate("appdata", "pics/kgpg_blank.png"));

    trustunknown.load(locate("appdata", "pics/kgpg_fill.png"));
    trustunknown.fill(KGpgSettings::colorUnknown());
    bitBlt(&trustunknown, 0, 0, &blankFrame, 0, 0, 50, 15);

    trustbad.load(locate("appdata", "pics/kgpg_fill.png"));
    trustbad.fill(KGpgSettings::colorBad());
    bitBlt(&trustbad, 0, 0, &blankFrame, 0, 0, 50, 15);

    trustrevoked.load(locate("appdata", "pics/kgpg_fill.png"));
    trustrevoked.fill(KGpgSettings::colorRev());
    bitBlt(&trustrevoked, 0, 0, &blankFrame, 0, 0, 50, 15);

    trustgood.load(locate("appdata", "pics/kgpg_fill.png"));
    trustgood.fill(KGpgSettings::colorGood());
    bitBlt(&trustgood, 0, 0, &blankFrame, 0, 0, 50, 15);

    connect(this, SIGNAL(expanded(Q3ListViewItem*)), this, SLOT(expandKey(Q3ListViewItem *)));

    header()->setMovingEnabled(false);
    setAcceptDrops(true);
    setDragEnabled(true);
}

void KeyView::setPreviewSize(const int &size)
{
    m_previewsize = size;
}

int KeyView::previewSize() const
{
    return m_previewsize;
}

void KeyView::setDisplayPhoto(const bool &display)
{
    m_displayphoto = display;
}

bool KeyView::displayPhoto() const
{
    return m_displayphoto;
}

void KeyView::slotAddColumn(const int &c)
{
    header()->setResizeEnabled(true, c);
    adjustColumn(c);
}

void KeyView::slotRemoveColumn(const int &c)
{
    hideColumn(c);
    header()->setResizeEnabled(false, c);
    header()->setStretchEnabled(true, 6);
}

void KeyView::contentsDragMoveEvent(QDragMoveEvent *e)
{
    e->accept(KURL::List::canDecode(e->mimeData()));
}

void  KeyView::contentsDropEvent(QDropEvent *o)
{
    KURL::List uriList = KURL::List::fromMimeData(o->mimeData());
    if (!uriList.isEmpty())
        droppedfile(uriList.first());
}

void KeyView::startDrag()
{
    QString keyid = currentItem()->text(6);
    if (!keyid.startsWith("0x"))
        return;

    KgpgInterface *interface = new KgpgInterface();
    QString keytxt = interface->getKeys(true, true, QStringList(keyid));
    delete interface;

    Q3DragObject *d = new Q3TextDrag(keytxt, this);
    d->dragCopy();
    // do NOT delete d.
}

void  KeyView::droppedfile(const KURL &url)
{
    if (KMessageBox::questionYesNo(this, i18n("<p>Do you want to import file <b>%1</b> into your key ring?</p>").arg(url.path()), QString::null, i18n("Import"), i18n("Do Not Import")) != KMessageBox::Yes)
        return;

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(importKeyFinished(QStringList)), this, SLOT(slotReloadKeys(QStringList)));
    interface->importKey(url);
}

void KeyView::slotReloadKeys(const QStringList &keyids)
{
    if (keyids.isEmpty())
        return;

    if (keyids.first() == "ALL")
    {
        refreshAll();
        return;
    }

    refreshKeys(keyids);

    ensureItemVisible(this->findItem((keyids.last()).right(8).prepend("0x"), 6));
    emit statusMessage(i18n("%1 Keys, %2 Groups").arg(childCount() - groupNb).arg(groupNb), 1);
    emit statusMessage(i18n("Ready"), 0);
}

void KeyView::refreshAll()
{
    emit statusMessage(i18n("Loading Keys..."), 0, true);
    kapp->processEvents();

    // update display of keys in main management window
    kdDebug(2100) << "Refreshing All" << endl;

    // get current position.
    KListViewItem *current = static_cast<KListViewItem*>(currentItem());
    if(current != 0)
    {
        while(current->depth() > 0)
            current = static_cast<KListViewItem*>(current->parent());
        takeItem(current);
    }

    // clear the list
    clear();

    orphanList = QStringList();
    if (refreshKeys())
    {
        kdDebug(2100) << "No key found" << endl;
        emit statusMessage(i18n("Ready"), 0);
        return;
    }

    refreshGroups();

    KListViewItem *newPos = 0L;
    if(current != 0)
    {
        // select previous selected
        if (!current->text(6).isEmpty())
            newPos = static_cast<KListViewItem*>(findItem(current->text(6), 6));
        else
            newPos = static_cast<KListViewItem*>(findItem(current->text(0), 0));
        delete current;
    }

    if (newPos != 0L)
    {
        setCurrentItem(newPos);
        setSelected(newPos, true);
        ensureItemVisible(newPos);
    }
    else
    {
        setCurrentItem(firstChild());
        setSelected(firstChild(), true);
    }

    emit statusMessage(i18n("%1 Keys, %2 Groups").arg(childCount() - groupNb).arg(groupNb), 1);
    emit statusMessage(i18n("Ready"),0);
    kdDebug(2100) << "Refresh Finished" << endl;
}

bool KeyView::refreshKeys(QStringList ids)
{
    KgpgInterface *interface = new KgpgInterface();
    KgpgListKeys secretlist = interface->readSecretKeys(true);

    QStringList issec;
    for (int i = 0; i < secretlist.size(); ++i)
        issec << secretlist.at(i).id();

    KgpgListKeys publiclist = interface->readPublicKeys(true, ids);
    delete interface;

    UpdateViewItem *item = 0;
    QString defaultkey = KGpgSettings::defaultKey();
    for (int i = 0; i < publiclist.size(); ++i)
    {
        KgpgKey key = publiclist.at(i);
        bool isbold = false;
        bool isexpired = false;
        bool noid = false;

        if (key.id() == defaultkey)
            isbold = true;
        if (key.trust() == 'e')
            isexpired = true;
        if (key.name().isEmpty())
            noid = true;

        item = new UpdateViewItem(this, key.name(), key.email(), QString::null, key.expiration(), key.size(), key.creation(), key.id(), isbold, isexpired, UpdateViewItem::Public);
        item->setPixmap(2, getTrustPix(key.trust(), key.valide()));
        item->setVisible(true);
        item->setExpandable(true);

        QStringList::Iterator ite;
        ite = issec.find(key.id());
        if (ite != issec.end())
        {
            item->setPixmap(0, pixkeyPair);
            issec.remove(*ite);
        }
        else
            item->setPixmap(0, pixkeySingle);
    }

    if (!issec.isEmpty())
        insertOrphans(issec);

    if (publiclist.size() == 0)
        return 1;
    else
    {
        if (publiclist.size() == 1)
        {
            clearSelection();
            setCurrentItem(item);
        }
        return 0;
    }
}

void KeyView::refreshcurrentkey(KListViewItem *current)
{
    if (!current)
        return;

    bool keyIsOpen = false;

    QString keyUpdate = current->text(6);
    if (keyUpdate.isEmpty())
        return;
    if (current->isOpen())
        keyIsOpen = true;

    delete current;

    refreshKeys(QStringList(keyUpdate));

    if (currentItem())
        if (currentItem()->text(6) == keyUpdate)
            currentItem()->setOpen(keyIsOpen);
}

void KeyView::refreshselfkey()
{
    if (currentItem()->depth() == 0)
        refreshcurrentkey(static_cast<KListViewItem*>(currentItem()));
    else
        refreshcurrentkey(static_cast<KListViewItem*>(currentItem()->parent()));
}

void KeyView::slotReloadOrphaned()
{
    QStringList issec;

    KgpgInterface *interface = new KgpgInterface();
    KgpgListKeys listkeys;

    listkeys = interface->readSecretKeys(true);
    for (int i = 0; i < listkeys.size(); ++i)
        issec << listkeys.at(i).id();
    listkeys = interface->readPublicKeys(true);
    for (int i = 0; i < listkeys.size(); ++i)
        issec.remove(listkeys.at(i).id());

    delete interface;

    QStringList::Iterator it;
    QStringList list;
    for (it = issec.begin(); it != issec.end(); ++it)
        if (findItem(*it, 6) == 0)
            list += *it;

    if (list.size() != 0)
        insertOrphans(list);

    setSelected(findItem(*it, 6), true);
    emit statusMessage(i18n("%1 Keys, %2 Groups").arg(childCount() - groupNb).arg(groupNb), 1);
    emit statusMessage(i18n("Ready"), 0);
}

void KeyView::insertOrphans(QStringList ids)
{
    KgpgInterface *interface = new KgpgInterface();
    KgpgListKeys keys = interface->readSecretKeys(true, ids);
    delete interface;

    UpdateViewItem *item = 0;
    for (int i = 0; i < keys.count(); ++i)
    {
        KgpgKey key = keys.at(i);
        bool isbold = false;
        bool isexpired = false;

        if (key.trust() == 'e')
            isexpired = true;

        orphanList << key.id();

        item = new UpdateViewItem(this, key.name(), key.email(), QString::null, key.expiration(), key.size(), key.creation(), key.id(), isbold, isexpired, UpdateViewItem::Secret);
        item->setPixmap(0, pixkeyOrphan);
    }

    if (ids.size() == 1)
    {
        if (keys.isEmpty())
        {
            orphanList.remove(ids.at(0));
            setSelected(currentItem(), true);
        }
        else
        {
            clearSelection();
            setCurrentItem(item);
            setSelected(item, true);
        }
    }
}

void KeyView::refreshGroups()
{
    kdDebug(2100) << "Refreshing groups..." << endl;
    UpdateViewItem *item = static_cast<UpdateViewItem*>(firstChild());
    while (item)
    {
        if (item->itemType() == UpdateViewItem::Group)
        {
            UpdateViewItem *item2 = static_cast<UpdateViewItem*>(item->nextSibling());
            delete item;
            item = item2;
        }
        else
            item = static_cast<UpdateViewItem*>(item->nextSibling());
    }

    QStringList groups = KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
    groupNb = groups.count();

    for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
        if (!QString(*it).isEmpty())
        {
            item = new UpdateViewItem(this, QString(*it), "-", "-", "-", "-", "-", "-", false, false, UpdateViewItem::Group);
            item->setPixmap(0, pixkeyGroup);
            item->setExpandable(false);
        }

    emit statusMessage(i18n("%1 Keys, %2 Groups").arg(childCount() - groupNb).arg(groupNb), 1);
    emit statusMessage(i18n("Ready"), 0);
}

void KeyView::refreshTrust(int color, QColor newColor)
{
    if (!newColor.isValid())
        return;

    QPixmap blankFrame;
    QPixmap newtrust;
    int trustFinger = 0;

    blankFrame.load(locate("appdata", "pics/kgpg_blank.png"));
    newtrust.load(locate("appdata", "pics/kgpg_fill.png"));
    newtrust.fill(newColor);

    bitBlt(&newtrust, 0, 0, &blankFrame, 0, 0, 50, 15);

    switch (color)
    {
        case kgpgOptions::GoodColor:
            trustFinger = trustgood.serialNumber();
            trustgood = newtrust;
            break;

        case kgpgOptions::BadColor:
            trustFinger = trustbad.serialNumber();
            trustbad = newtrust;
            break;

        case kgpgOptions::UnknownColor:
            trustFinger = trustunknown.serialNumber();
            trustunknown = newtrust;
            break;

        case kgpgOptions::RevColor:
            trustFinger = trustrevoked.serialNumber();
            trustrevoked = newtrust;
            break;
    }

    KListViewItem *item = static_cast<KListViewItem*>(firstChild());
    while (item)
    {
        if (item->pixmap(2))
            if (item->pixmap(2)->serialNumber() == trustFinger)
                item->setPixmap(2, newtrust);
        item = static_cast<KListViewItem*>(item->nextSibling());
    }
}

void KeyView::expandKey(Q3ListViewItem *item2)
{
    KListViewItem *item = static_cast<KListViewItem*>(item2);
    if (item->childCount() != 0)
        return;   // key has already been expanded

    QString keyid = item->text(6);

    KgpgInterface *interface = new KgpgInterface();
    KgpgListKeys keys = interface->readPublicKeys(true, QStringList(keyid), true);
    KgpgKey key = keys.at(0);

    UpdateViewItem *tmpitem;


    /********* insertion of sub keys ********/
    for (int i = 0; i < key.subList()->size(); ++i)
    {
        KgpgKeySub sub = key.subList()->at(i);

        QString algo = i18n("%1 subkey").arg(KgpgKey::algorithmeToString(sub.algorithme()));
        tmpitem = new UpdateViewItem(item, algo, QString::null, QString::null, sub.expiration(), sub.size(), sub.creation(), sub.id(), false, false, UpdateViewItem::Sub);
        tmpitem->setPixmap(0, pixkeySingle);
        tmpitem->setPixmap(2, getTrustPix(sub.trust(), sub.valide()));
        insertSigns(tmpitem, sub.signList());
    }
    /****************************************/


    /********* insertion of users id ********/
    for (int i = 0; i < key.uidList()->size(); ++i)
    {
        KgpgKeyUid uid = key.uidList()->at(i);

        tmpitem = new UpdateViewItem(item, uid.name(), uid.email(), QString::null, "-", "-", "-", "-", false, false, UpdateViewItem::Uid);
        tmpitem->setPixmap(2, getTrustPix(key.trust(), key.valide()));
        tmpitem->setPixmap(0, pixuserid);
        insertSigns(tmpitem, uid.signList());
    }
    /****************************************/


    /******** insertion of photos id ********/
    QStringList photolist = key.photoList();
    for (int i = 0; i < photolist.size(); ++i)
    {
        tmpitem = new UpdateViewItem(item, i18n("Photo id"), QString::null, QString::null, "-", "-", "-", photolist.at(i), false, false, UpdateViewItem::Uat);
        tmpitem->setPixmap(2, getTrustPix(key.trust(), key.valide()));

        if (m_displayphoto)
        {
            QPixmap pixmap = interface->loadPhoto(keyid, photolist.at(i), true);
            tmpitem->setPixmap(0, pixmap.scaled(m_previewsize + 5, m_previewsize, Qt::KeepAspectRatio));
        }
        else
            tmpitem->setPixmap(0, pixuserphoto);

        KgpgKeyUat uat = key.uatList()->at(i);
        insertSigns(tmpitem, uat.signList());
    }
    /****************************************/


    /******** insertion of signature ********/
    insertSigns(item, key.signList());
    /****************************************/

    delete interface;
}

void KeyView::insertSigns(KListViewItem *item, const KgpgKeySignList &list)
{
    UpdateViewItem *newitem;
    for (int i = 0; i < list.size(); ++i)
    {
        const KgpgKeySign sign = list.at(i);

        QString tmpname = sign.name();
        if (!sign.comment().isEmpty())
            tmpname += " (" + sign.comment() + ")";

        QString tmpemail = sign.email();
        if (sign.local())
            tmpemail += i18n(" [local]");

        if (sign.revocation())
        {
            tmpemail += i18n(" [Revocation signature]");
            newitem = new UpdateViewItem(item, tmpname, tmpemail, "-", "-", "-", sign.creation(), sign.id(), false, false, UpdateViewItem::Rev);
            newitem->setPixmap(0, pixRevoke);
        }
        else
        {
            newitem = new UpdateViewItem(item, tmpname, tmpemail, "-", sign.expiration(), "-", sign.creation(), sign.id(), false, false, UpdateViewItem::Sign);
            newitem->setPixmap(0, pixsignature);
        }
    }
}

void KeyView::expandGroup(KListViewItem *item)
{
    QStringList keysGroup = KgpgInterface::getGpgGroupSetting(item->text(0), KGpgSettings::gpgConfigPath());

    kdDebug(2100) << keysGroup << endl;

    for (QStringList::Iterator it = keysGroup.begin(); it != keysGroup.end(); ++it)
    {
        UpdateViewItem *item2 = new UpdateViewItem(item, QString(*it), QString::null, QString::null, QString::null, QString::null, QString::null, QString::null);
        item2->setPixmap(0, pixkeyGroup);
        item2->setExpandable(false);
    }
}

QPixmap KeyView::getTrustPix(const QChar &c, const bool &isvalid)
{
    if (!isvalid)
        return trustbad;
    if (c == 'o')
        return trustunknown;
    if (c == 'i')
        return trustbad;
    if (c == 'd')
        return trustbad;
    if (c == 'r')
        return trustrevoked;
    if (c == 'e')
        return trustbad;
    if (c == 'q')
        return trustunknown;
    if (c == 'n')
        return trustunknown;
    if (c == 'm')
        return trustbad;
    if (c == 'f')
        return trustgood;
    if (c == 'u')
        return trustgood;
    return trustunknown;
}

mySearchLine::mySearchLine(QWidget *parent, KeyView *listView)
            : KListViewSearchLine(parent, listView)
{
    m_searchlistview = listView;
    setKeepParentsVisible(false);
}

mySearchLine::~ mySearchLine()
{
}

void mySearchLine::updateSearch(const QString& s)
{
    KListViewSearchLine::updateSearch(s);
    if (m_searchlistview->displayOnlySecret || !m_searchlistview->displayDisabled)
    {
        int disabledSerial = m_searchlistview->trustbad.serialNumber();
        KListViewItem *item = static_cast<KListViewItem*>(m_searchlistview->firstChild());
        while (item)
        {
            if (item->isVisible() && !(item->text(6).isEmpty()))
            {
                if (m_searchlistview->displayOnlySecret && m_searchlistview->secretList.find(item->text(6)) == -1)
                    item->setVisible(false);

                if (!m_searchlistview->displayDisabled && item->pixmap(2))
                    if (item->pixmap(2)->serialNumber() == disabledSerial)
                        item->setVisible(false);
            }

            item = static_cast<KListViewItem*>(item->nextSibling());
        }
    }
}

bool mySearchLine::itemMatches(const KListViewItem *item, const QString &s) const
{
    if (item->depth() != 0)
        return true;
    else
        return KListViewSearchLine::itemMatches(item, s);
}

// main window for key management
listKeys::listKeys(QWidget *parent, const char *name)
        : DCOPObject("KeyInterface"), KMainWindow(parent, name,0)
{
    setCaption(i18n("Key Management"));

    keysList2 = new KeyView(this);
    keysList2->photoKeysList = QString::null;
    keysList2->groupNb = 0;
    keyStatusBar = 0;
    readOptions();

    if (showTipOfDay)
        installEventFilter(this);

    KStdAction::quit(this, SLOT(quitApp()), actionCollection());
    KStdAction::find(this, SLOT(findKey()), actionCollection());
    KStdAction::findNext(this, SLOT(findNextKey()), actionCollection());
    KStdAction::preferences(this, SLOT(showOptions()), actionCollection(),"options_configure");

    (void) new KAction(i18n("&Key Server Dialog"), "network", 0,this, SLOT(showKeyServer()),actionCollection(),"key_server");
    (void) new KAction(i18n("Tip of the &Day"), "idea", 0,this, SLOT(slotTip()), actionCollection(),"help_tipofday");
    (void) new KAction(i18n("View GnuPG Manual"), "contents", 0,this, SLOT(slotManpage()),actionCollection(),"gpg_man");
    (void) new KAction(i18n("&Open Editor"), "edit", 0, this, SLOT(slotOpenEditor()), actionCollection(), "kgpg_editor");
    (void) new KAction(i18n("&Go to Default Key"), "gohome", QKeySequence(Qt::CTRL + Qt::Key_Home), this, SLOT(slotGotoDefaultKey()), actionCollection(),"go_default_key");
    (void) new KAction(i18n("&Refresh List"), "reload", KStdAccel::reload(), this, SLOT(refreshkey()), actionCollection(), "key_refresh");

    (void) new KToggleAction(i18n("&Show only Secret Keys"), "kgpg_show", 0,this, SLOT(slotToggleSecret()),actionCollection(),"show_secret");
    (void) new KToggleAction(i18n("&Hide Expired/Disabled Keys"),0, 0,this, SLOT(slotToggleDisabled()),actionCollection(),"hide_disabled");

    KAction *infoKey = new KAction(i18n("&Edit Key"), "kgpg_info", Qt::Key_Return, this, SLOT(listsigns()), actionCollection(), "key_info");
    KAction *editKey = new KAction(i18n("Edit Key in &Terminal"), "kgpg_term", QKeySequence(Qt::ALT+Qt::Key_Return),this, SLOT(slotedit()),actionCollection(),"key_edit");
    KAction *generateKey = new KAction(i18n("&Generate Key Pair..."), "kgpg_gen", KStdAccel::shortcut(KStdAccel::New),this, SLOT(slotgenkey()),actionCollection(),"key_gener");
    KAction *exportPublicKey = new KAction(i18n("E&xport Public Keys..."), "kgpg_export", KStdAccel::shortcut(KStdAccel::Copy), this, SLOT(slotexport()), actionCollection(), "key_export");
    KAction *importKey = new KAction(i18n("&Import Key..."), "kgpg_import", KStdAccel::shortcut(KStdAccel::Paste), this, SLOT(slotPreImportKey()), actionCollection(), "key_import");
    KAction *newContact = new KAction(i18n("&Create New Contact in Address Book"), "kaddressbook", 0, this, SLOT(addToKAB()), actionCollection(), "add_kab");
    KAction *createGroup = new KAction(i18n("&Create Group with Selected Keys..."), 0, 0, this, SLOT(createNewGroup()), actionCollection(), "create_group");
    KAction *editCurrentGroup = new KAction(i18n("&Edit Group"), 0, 0, this, SLOT(editGroup()), actionCollection(), "edit_group");
    KAction *delGroup = new KAction(i18n("&Delete Group"), 0, 0, this, SLOT(deleteGroup()), actionCollection(), "delete_group");
    KAction *deleteKey = new KAction(i18n("&Delete Keys"), "editdelete", Qt::Key_Delete, this, SLOT(confirmdeletekey()), actionCollection(), "key_delete");
    KAction *setDefaultKey = new KAction(i18n("Set as De&fault Key"), 0, 0, this, SLOT(slotSetDefKey()), actionCollection(), "key_default");
    KAction *addPhoto = new KAction(i18n("&Add Photo"), 0, 0, this, SLOT(slotAddPhoto()), actionCollection(), "add_photo");
    KAction *addUid = new KAction(i18n("&Add User Id"), 0, 0, this, SLOT(slotAddUid()), actionCollection(), "add_uid");
    KAction *exportSecretKey = new KAction(i18n("Export Secret Key..."), 0, 0,this, SLOT(slotexportsec()),actionCollection(),"key_sexport");
    KAction *deleteKeyPair = new KAction(i18n("Delete Key Pair"), 0, 0,this, SLOT(deleteseckey()),actionCollection(),"key_pdelete");
    KAction *revokeKey = new KAction(i18n("Revoke Key..."), 0, 0,this, SLOT(revokeWidget()),actionCollection(),"key_revoke");
    KAction *regeneratePublic = new KAction(i18n("&Regenerate Public Key"), 0, 0,this, SLOT(slotregenerate()),actionCollection(),"key_regener");
    KAction *delUid = new KAction(i18n("&Delete User Id"), 0, 0, this, SLOT(slotDelUid()), actionCollection(), "del_uid");
    KAction *openPhoto = new KAction(i18n("&Open Photo"), "image", 0, this, SLOT(slotShowPhoto()), actionCollection(), "key_photo");
    KAction *deletePhoto = new KAction(i18n("&Delete Photo"), "delete", 0, this, SLOT(slotDeletePhoto()), actionCollection(), "delete_photo");
    KAction *delSignKey = new KAction(i18n("Delete Sign&ature"), "editdelete", 0, this, SLOT(delsignkey()), actionCollection(), "key_delsign");

    importAllSignKeys = new KAction(i18n("Import &Missing Signatures From Keyserver"), "network", 0, this, SLOT(importallsignkey()), actionCollection(), "key_importallsign");
    refreshKey = new KAction(i18n("&Refresh Keys From Keyserver"), "reload", 0, this, SLOT(refreshKeyFromServer()), actionCollection(), "key_server_refresh");
    signKey = new KAction(i18n("&Sign Keys..."), "kgpg_sign", 0, this, SLOT(signkey()), actionCollection(), "key_sign");
    importSignatureKey = new KAction(i18n("Import Key From Keyserver"), "network", 0, this, SLOT(preimportsignkey()), actionCollection(), "key_importsign");

    sTrust = new KToggleAction(i18n("Trust"),0, 0,this, SLOT(slotShowTrust()),actionCollection(),"show_trust");
    sSize = new KToggleAction(i18n("Size"),0, 0,this, SLOT(slotShowSize()),actionCollection(),"show_size");
    sCreat = new KToggleAction(i18n("Creation"),0, 0,this, SLOT(slotShowCreat()),actionCollection(),"show_creat");
    sExpi = new KToggleAction(i18n("Expiration"),0, 0,this, SLOT(slotShowExpi()),actionCollection(),"show_expi");

    photoProps = new KSelectAction(i18n("&Photo ID's"),"kgpg_photo", actionCollection(), "photo_settings");

    // Keep the list in kgpg.kcfg in sync with this one!
    QStringList list;
    list.append(i18n("Disable"));
    list.append(i18n("Small"));
    list.append(i18n("Medium"));
    list.append(i18n("Large"));
    photoProps->setItems(list);

    keysList2->displayOnlySecret = false;
    keysList2->displayDisabled = true;

    int psize = KGpgSettings::photoProperties();
    photoProps->setCurrentItem(psize);
    slotSetPhotoSize(psize);

    // popup on a public key
    popup = new KMenu();
    exportPublicKey->plug(popup);
    deleteKey->plug(popup);
    signKey->plug(popup);
    infoKey->plug(popup);
    editKey->plug(popup);
    refreshKey->plug(popup);
    setDefaultKey->plug(popup);
    popup->insertSeparator();
    importAllSignKeys->plug(popup);

    // popup on a secret key
    popupsec = new KMenu();
    exportPublicKey->plug(popupsec);
    signKey->plug(popupsec);
    infoKey->plug(popupsec);
    editKey->plug(popupsec);
    refreshKey->plug(popupsec);
    setDefaultKey->plug(popupsec);
    popupsec->insertSeparator();
    importAllSignKeys->plug(popupsec);
    popupsec->insertSeparator();
    addPhoto->plug(popupsec);
    addUid->plug(popupsec);
    exportSecretKey->plug(popupsec);
    deleteKeyPair->plug(popupsec);
    revokeKey->plug(popupsec);

    // popup on a group
    popupgroup = new KMenu();
    editCurrentGroup->plug(popupgroup);
    delGroup->plug(popupgroup);

    popupout = new KMenu();
    importKey->plug(popupout);
    generateKey->plug(popupout);

    // popup on a signature
    popupsig = new KMenu();
    importSignatureKey->plug(popupsig);
    delSignKey->plug(popupsig);

    // popup on a photo
    popupphoto = new KMenu();
    openPhoto->plug(popupphoto);
    deletePhoto->plug(popupphoto);

    // popup on an user id
    popupuid = new KMenu();
    delUid->plug(popupuid);

    // popup on an orphan key
    popuporphan = new KMenu();
    regeneratePublic->plug(popuporphan);
    deleteKeyPair->plug(popuporphan);

    editCurrentGroup->setEnabled(false);
    delGroup->setEnabled(false);
    createGroup->setEnabled(false);
    infoKey->setEnabled(false);
    editKey->setEnabled(false);
    signKey->setEnabled(false);
    refreshKey->setEnabled(false);
    exportPublicKey->setEnabled(false);
    newContact->setEnabled(false);

    setCentralWidget(keysList2);
    keysList2->restoreLayout(KGlobal::config(), "KeyView");

    connect(keysList2, SIGNAL(returnPressed(Q3ListViewItem *)), this, SLOT(listsigns()));
    connect(keysList2, SIGNAL(doubleClicked(Q3ListViewItem *, const QPoint &, int)), this, SLOT(listsigns()));
    connect(keysList2, SIGNAL(selectionChanged ()), this, SLOT(checkList()));
    connect(keysList2, SIGNAL(contextMenuRequested(Q3ListViewItem *, const QPoint &, int)), this, SLOT(slotmenu(Q3ListViewItem *, const QPoint &, int)));
    connect(keysList2, SIGNAL(destroyed()), this, SLOT(annule()));
    connect(photoProps, SIGNAL(activated(int)), this, SLOT(slotSetPhotoSize(int)));

    // get all keys data
    keyStatusBar = statusBar();

    setupGUI(KMainWindow::Create | Save | ToolBar | StatusBar | Keys, "listkeys.rc");
    toolBar()->insertLineSeparator();

    QToolButton *clearSearch = new QToolButton(toolBar());
    clearSearch->setTextLabel(i18n("Clear Search"), true);
    clearSearch->setIconSet(SmallIconSet(QApplication::reverseLayout() ? "clear_left" : "locationbar_erase"));
    (void) new QLabel(i18n("Search: "), toolBar());
    listViewSearch = new mySearchLine(toolBar(), keysList2);
    connect(clearSearch, SIGNAL(pressed()), listViewSearch, SLOT(clear()));

    (void)new KAction(i18n("Filter Search"), Qt::Key_F6, listViewSearch, SLOT(setFocus()), actionCollection(), "search_focus");

    sTrust->setChecked(KGpgSettings::showTrust());
    sSize->setChecked(KGpgSettings::showSize());
    sCreat->setChecked(KGpgSettings::showCreat());
    sExpi->setChecked(KGpgSettings::showExpi());

    statusbarTimer = new QTimer(this);

    keyStatusBar->insertItem("", 0, 1);
    keyStatusBar->insertFixedItem(i18n("00000 Keys, 000 Groups"), 1, true);
    keyStatusBar->setItemAlignment(0, Qt::AlignLeft);
    keyStatusBar->changeItem("", 1);
    connect(keysList2, SIGNAL(statusMessage(QString, int, bool)), this, SLOT(changeMessage(QString, int, bool)));
    connect(statusbarTimer, SIGNAL(timeout()), this, SLOT(statusBarTimeout()));

    s_kgpgEditor = new KgpgEditor(parent, "editor", Qt::WType_Dialog, actionCollection()->action("go_default_key")->shortcut(), true);
    connect(s_kgpgEditor, SIGNAL(refreshImported(QStringList)), keysList2, SLOT(slotReloadKeys(QStringList)));
    connect(this, SIGNAL(fontChanged(QFont)), s_kgpgEditor, SLOT(slotSetFont(QFont)));
}

void listKeys::showKeyManager()
{
    show();
}

void listKeys::slotOpenEditor()
{
    KgpgEditor *kgpgtxtedit = new KgpgEditor(this, "editor", Qt::Window, actionCollection()->action("go_default_key")->shortcut());
    kgpgtxtedit->setAttribute(Qt::WA_DeleteOnClose);

    connect(kgpgtxtedit, SIGNAL(refreshImported(QStringList)), keysList2, SLOT(slotReloadKeys(QStringList)));
    connect(kgpgtxtedit, SIGNAL(encryptFiles(KURL::List)), this, SIGNAL(encryptFiles(KURL::List)));
    connect(this, SIGNAL(fontChanged(QFont)), kgpgtxtedit, SLOT(slotSetFont(QFont)));

    kgpgtxtedit->show();
}

void listKeys::statusBarTimeout()
{
    keyStatusBar->changeItem("", 0);
}

void listKeys::changeMessage(QString msg, int nb, bool keep)
{
    statusbarTimer->stop();
    if ((nb == 0) & (!keep))
        statusbarTimer->start(10000, true);
    keyStatusBar->changeItem(" " + msg + " ", nb);
}

void listKeys::slotShowTrust()
{
    if (sTrust->isChecked())
        keysList2->slotAddColumn(2);
    else
        keysList2->slotRemoveColumn(2);
}

void listKeys::slotShowExpi()
{
    if (sExpi->isChecked())
        keysList2->slotAddColumn(3);
    else
        keysList2->slotRemoveColumn(3);
}

void listKeys::slotShowSize()
{
    if (sSize->isChecked())
        keysList2->slotAddColumn(4);
    else
        keysList2->slotRemoveColumn(4);
}

void listKeys::slotShowCreat()
{
    if (sCreat->isChecked())
        keysList2->slotAddColumn(5);
    else
        keysList2->slotRemoveColumn(5);
}

bool listKeys::eventFilter(QObject *, QEvent *e)
{
    if ((e->type() == QEvent::Show) && (showTipOfDay))
    {
        KTipDialog::showTip(this, QString("kgpg/tips"), false);
        showTipOfDay = false;
    }

    return false;
}

void listKeys::slotToggleSecret()
{
    KListViewItem *item = static_cast<KListViewItem*>(keysList2->firstChild());
    if (!item)
        return;

    keysList2->displayOnlySecret = !keysList2->displayOnlySecret;
    listViewSearch->updateSearch(listViewSearch->text());
}

void listKeys::slotToggleDisabled()
{
    KListViewItem *item = static_cast<KListViewItem*>(keysList2->firstChild());
    if (!item)
        return;

    keysList2->displayDisabled = !keysList2->displayDisabled;
    listViewSearch->updateSearch(listViewSearch->text());
}

void listKeys::slotGotoDefaultKey()
{
    KListViewItem *myDefaulKey = static_cast<KListViewItem*>(keysList2->findItem(KGpgSettings::defaultKey(), 6));
    keysList2->clearSelection();
    keysList2->setCurrentItem(myDefaulKey);
    keysList2->setSelected(myDefaulKey, true);
    keysList2->ensureItemVisible(myDefaulKey);
}

void listKeys::refreshKeyFromServer()
{
    if (keysList2->currentItem() == NULL)
        return;

    QString keyIDS;
    keysList = keysList2->selectedItems();
    bool keyDepth = true;

    for (int i = 0; i < keysList.count(); ++i)
        if (keysList.at(i))
        {
            if ((keysList.at(i)->depth() != 0) || (keysList.at(i)->text(6).isEmpty()))
                keyDepth = false;
            else
                keyIDS += keysList.at(i)->text(6) + " ";
        }

    if (!keyDepth)
    {
        KMessageBox::sorry(this, i18n("You can only refresh primary keys. Please check your selection."));
        return;
    }

    kServer = new keyServer(0, "server_dialog", false);
    connect(kServer, SIGNAL(importFinished(QString)), this, SLOT(refreshFinished()));
    kServer->slotSetText(keyIDS);
    kServer->slotImport();
}

void listKeys::refreshFinished()
{
    if (kServer)
        kServer = 0L;

    for (int i = 0; i < keysList.count(); ++i)
        if (keysList.at(i))
            keysList2->refreshcurrentkey(static_cast<KListViewItem*>(keysList.at(i)));
}

void listKeys::slotDelUid()
{
    KListViewItem *item = static_cast<KListViewItem*>(keysList2->currentItem());
    while (item->depth()>0)
        item = static_cast<KListViewItem*>(item->parent());

    KProcess *process = new KProcess();
    KConfig *config = KGlobal::config();
    config->setGroup("General");
    *process << config->readPathEntry("TerminalApplication", "konsole");
    *process << "-e" << "gpg";
    *process << "--edit-key" << item->text(6) << "uid";
    process->start(KProcess::Block);
    keysList2->refreshselfkey();
}

void listKeys::slotregenerate()
{
    FILE *fp;
    QString tst;
    char line[300];
    QString cmd = "gpg --no-secmem-warning --export-secret-key " + keysList2->currentItem()->text(6) + " | gpgsplit --no-split --secret-to-public | gpg --import";

    fp = popen(cmd.ascii(), "r");
    while (fgets(line, sizeof(line), fp))
    {
        tst += line;
    }
    pclose(fp);
    QString regID = keysList2->currentItem()->text(6);
    keysList2->takeItem(keysList2->currentItem());
    keysList2->refreshKeys(QStringList(regID));
}

void listKeys::slotAddUid()
{
    addUidWidget = new KDialogBase(KDialogBase::Swallow, i18n("Add New User Id"),  KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, this, 0, true);
    addUidWidget->enableButtonOK(false);
    AddUid *keyUid = new AddUid();
    addUidWidget->setMainWidget(keyUid);
    //keyUid->setMinimumSize(keyUid->sizeHint());
    keyUid->setMinimumWidth(300);
    connect(keyUid->kLineEdit1, SIGNAL(textChanged(const QString & )), this, SLOT(slotAddUidEnable(const QString & )));
    if (addUidWidget->exec() != QDialog::Accepted)
        return;

    KgpgInterface *addUidProcess = new KgpgInterface();
    connect(addUidProcess,SIGNAL(addUidFinished(int, KgpgInterface*)), this ,SLOT(slotAddUidFin(int, KgpgInterface*)));
    addUidProcess->addUid(keysList2->currentItem()->text(6),keyUid->kLineEdit1->text(),keyUid->kLineEdit2->text(),keyUid->kLineEdit3->text());
}

void listKeys::slotAddUidFin(int res, KgpgInterface *interface)
{
    // TODO tester les res
    kdDebug(2100) << "Resultat : " << res << endl;
    delete interface;
    keysList2->refreshselfkey();
}

void listKeys::slotAddUidEnable(const QString & name)
{
    addUidWidget->enableButtonOK(name.length() > 4);
}

void listKeys::slotAddPhoto()
{
    KgpgLibrary *lib = new KgpgLibrary();
    connect (lib, SIGNAL(photoAdded()), this, SLOT(slotUpdatePhoto()));
    lib->addPhoto(keysList2->currentItem()->text(6));
}

void listKeys::slotDeletePhoto()
{
    QString mess = i18n("<qt>Are you sure you want to delete Photo id <b>%1</b><br>from key <b>%2 &lt;%3&gt;</b>?</qt>");
    mess = mess.arg(keysList2->currentItem()->text(6));
    mess = mess.arg(keysList2->currentItem()->parent()->text(0));
    mess = mess.arg(keysList2->currentItem()->parent()->text(1));

    /*
    if (KMessageBox::warningContinueCancel(this, mess, i18n("Warning"), KGuiItem(i18n("Delete"), "editdelete")) != KMessageBox::Continue)
        return;
    */

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(delPhotoFinished(int, KgpgInterface*)), this, SLOT(slotDelPhotoFinished(int, KgpgInterface*)));
    interface->deletePhoto(keysList2->currentItem()->parent()->text(6), keysList2->currentItem()->text(6));
}

void listKeys::slotDelPhotoFinished(int res, KgpgInterface *interface)
{
    delete interface;

    // TODO : add res == 3 (bad passphrase)

    if (res == 2)
        slotUpdatePhoto();

}

void listKeys::slotUpdatePhoto()
{
    keysList2->refreshselfkey();
}

void listKeys::slotSetPhotoSize(int size)
{
    switch(size)
    {
        case 1:
            showPhoto = true;
            keysList2->setPreviewSize(22);
            break;
        case 2:
            showPhoto = true;
            keysList2->setPreviewSize(42);
            break;
        case 3:
            showPhoto = true;
            keysList2->setPreviewSize(65);
            break;
        default:
            showPhoto = false;
            break;
    }
    keysList2->setDisplayPhoto(showPhoto);

    // refresh keys with photo id
    KListViewItem *newdef = static_cast<KListViewItem*>(keysList2->firstChild());
    while (newdef)
    {
        //if ((keysList2->photoKeysList.find(newdef->text(6))!=-1) && (newdef->childCount ()>0))
        if (newdef->childCount() > 0)
        {
            bool hasphoto = false;
            KListViewItem *newdefChild = static_cast<KListViewItem*>(newdef->firstChild());
            while (newdefChild)
            {
                if (newdefChild->text(0) == i18n("Photo id"))
                {
                    hasphoto = true;
                    break;
                }
                newdefChild = static_cast<KListViewItem*>(newdefChild->nextSibling());
            }

            if (hasphoto)
            {
                while (newdef->firstChild())
                    delete newdef->firstChild();
                keysList2->expandKey(newdef);
            }
        }
        newdef = static_cast<KListViewItem*>(newdef->nextSibling());
    }
}

void listKeys::findKey()
{
    KFindDialog fd(this, "find_dialog");
    if (fd.exec() != QDialog::Accepted)
        return;

    searchString = fd.pattern();
    searchOptions = fd.options();
    findFirstKey();
}

void listKeys::findFirstKey()
{
    if (searchString.isEmpty())
        return;

    bool foundItem = true;
    KListViewItem *item = static_cast<KListViewItem*>(keysList2->firstChild());
    if (!item)
        return;
    QString searchText = item->text(0) + " " + item->text(1) + " " + item->text(6);

    KFind *m_find = new KFind(searchString, searchOptions, this);
    m_find->setData(searchText);
    while (m_find->find() == KFind::NoMatch)
    {
        if (!item->nextSibling())
        {
            foundItem = false;
            break;
        }
        else
        {
            item = static_cast<KListViewItem*>(item->nextSibling());
            searchText = item->text(0) + " " + item->text(1) + " " + item->text(6);
            m_find->setData(searchText);
        }
    }
    delete m_find;

    if (foundItem)
    {
        keysList2->clearSelection();
        keysList2->setCurrentItem(item);
        keysList2->setSelected(item, true);
        keysList2->ensureItemVisible(item);
    }
    else
        KMessageBox::sorry(this, i18n("<qt>Search string '<b>%1</b>' not found.").arg(searchString));
}

void listKeys::findNextKey()
{
    //kdDebug(2100)<<"find next"<<endl;
    if (searchString.isEmpty())
    {
        findKey();
        return;
    }

    bool foundItem = true;
    KListViewItem *item = static_cast<KListViewItem*>(keysList2->currentItem());
    if (!item)
        return;

    while(item->depth() > 0)
        item = static_cast<KListViewItem*>(item->parent());
    item=static_cast<KListViewItem*>(item->nextSibling());

    QString searchText = item->text(0) + " " + item->text(1) + " " + item->text(6);
    //kdDebug(2100) << "Next string:" << searchText << endl;
    //kdDebug(2100) << "Search:" << searchString << endl;
    //kdDebug(2100) << "OPts:" << searchOptions << endl;
    KFind *m_find = new KFind(searchString, searchOptions, this);
    m_find->setData(searchText);
    while (m_find->find() == KFind::NoMatch)
    {
        if (!item->nextSibling())
        {
            foundItem = false;
            break;
        }
        else
        {
            item = static_cast<KListViewItem*>(item->nextSibling());
            searchText = item->text(0) + " " + item->text(1) + " " + item->text(6);
            m_find->setData(searchText);
            //kdDebug(2100) << "Next string:" << searchText << endl;
        }
    }
    delete m_find;

    if (foundItem)
    {
        keysList2->clearSelection();
        keysList2->setCurrentItem(item);
        keysList2->setSelected(item,true);
        keysList2->ensureItemVisible(item);
    }
    else
        findFirstKey();
}

void listKeys::addToKAB()
{
    KABC::Key key;
    if (!keysList2->currentItem())
        return;

    //QString email = extractKeyMail(keysList2->currentItem()).simplified();
    QString email = keysList2->currentItem()->text(1);

    KABC::AddressBook *ab = KABC::StdAddressBook::self();
    if (!ab->load())
    {
        KMessageBox::sorry(this, i18n("Unable to contact the address book. Please check your installation."));
        return;
    }

    KABC::Addressee::List addresseeList = ab->findByEmail(email);
    KToolInvocation::startServiceByDesktopName("kaddressbook");
    DCOPRef call("kaddressbook", "KAddressBookIface");
    if(!addresseeList.isEmpty())
        call.send("showContactEditor(QString)", addresseeList.first().uid());
    else
        call.send("addEmail(QString)", QString (keysList2->currentItem()->text(0)) + " <" + email + ">");
}

/*
void listKeys::allToKAB()
{
        KABC::Key key;
        QString email;
        QStringList keylist;
        KABC::Addressee a;

        KABC::AddressBook *ab = KABC::StdAddressBook::self();
        if ( !ab->load() ) {
                KMessageBox::sorry(this,i18n("Unable to contact the address book. Please check your installation."));
                return;
        }

        KListViewItem * myChild = keysList2->firstChild();
        while( myChild ) {
                //email=extractKeyMail(myChild).simplified();
                email=myChild->text(1);
                KABC::Addressee::List addressees = ab->findByEmail( email );
                if (addressees.count()==1) {
                        a=addressees.first();
                        KgpgInterface *ks=new KgpgInterface();
                        key.setTextData(ks->getKey(myChild->text(6),true));
                        a.insertKey(key);
                        ab->insertAddressee(a);
                        keylist<<myChild->text(6)+": "+email;
                }
                //            doSomething( myChild );
                myChild = myChild->nextSibling();
        }
        KABC::StdAddressBook::save();
        if (!keylist.isEmpty())
                KMessageBox::informationList(this,i18n("The following keys were exported to the address book:"),keylist);
        else
                KMessageBox::sorry(this,i18n("No entry matching your keys were found in the address book."));
}
*/

void listKeys::slotManpage()
{
    KToolInvocation::startServiceByDesktopName("khelpcenter", QString("man:/gpg"), 0, 0, 0, "", true);
}

void listKeys::slotTip()
{
    KTipDialog::showTip(this, QString("kgpg/tips"), true);
}

void listKeys::closeEvent (QCloseEvent *e)
{
    // kapp->ref(); // prevent KMainWindow from closing the app
    // KMainWindow::closeEvent(e);
    e->accept();
    // hide();
    // e->ignore();
}

void listKeys::showKeyServer()
{
    keyServer *ks = new keyServer(this);
    connect(ks, SIGNAL(importFinished(QString)), keysList2, SLOT(refreshcurrentkey(QString)));
    ks->exec();
    if (ks)
        delete ks;
    refreshkey();
}

void listKeys::checkList()
{
    QList<Q3ListViewItem*> exportList = keysList2->selectedItems();
    if (exportList.count() > 1)
    {
        stateChanged("multi_selected");
        for (int i = 0; i < exportList.count(); ++i)
            if (exportList.at(i) && !(exportList.at(i)->isVisible()))
                exportList.at(i)->setSelected(false);
    }
    else
    {
        if (keysList2->currentItem()->text(6).isEmpty())
            stateChanged("group_selected");
        else
            stateChanged("single_selected");

    }

    int serial = keysList2->currentItem()->pixmap(0)->serialNumber();
    if (serial == keysList2->pixkeySingle.serialNumber())
    {
        if (keysList2->currentItem()->depth() == 0)
            changeMessage(i18n("Public Key"), 0);
        else
            changeMessage(i18n("Sub Key"), 0);
    }
    else
    if (serial == keysList2->pixkeyPair.serialNumber())
        changeMessage(i18n("Secret Key Pair"), 0);
    else
    if (serial == keysList2->pixkeyGroup.serialNumber())
        changeMessage(i18n("Key Group"), 0);
    else
    if (serial == keysList2->pixsignature.serialNumber())
        changeMessage(i18n("Signature"), 0);
    else
    if (serial == keysList2->pixuserid.serialNumber())
        changeMessage(i18n("User ID"), 0);
    else
    if (keysList2->currentItem()->text(0) == i18n("Photo id"))
        changeMessage(i18n("Photo ID"), 0);
    else
    if (serial == keysList2->pixRevoke.serialNumber())
        changeMessage(i18n("Revocation Signature"), 0);
    else
    if (serial == keysList2->pixkeyOrphan.serialNumber())
        changeMessage(i18n("Orphaned Secret Key"), 0);
}

void listKeys::annule()
{
    // close window
    close();
}

void listKeys::quitApp()
{
    // close window
    exit(1); // FIXME another ?
}

void listKeys::readOptions()
{
    clipboardMode = QClipboard::Clipboard;
    if (KGpgSettings::useMouseSelection() && (kapp->clipboard()->supportsSelection()))
        clipboardMode = QClipboard::Selection;

    // re-read groups in case the config file location was changed
    QStringList groups = KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
    KGpgSettings::setGroups(groups.join(","));
    keysList2->groupNb = groups.count();
    if (keyStatusBar)
        changeMessage(i18n("%1 Keys, %2 Groups").arg(keysList2->childCount() - keysList2->groupNb).arg(keysList2->groupNb), 1);

    showTipOfDay = KGpgSettings::showTipOfDay();
}

void listKeys::showOptions()
{
    if (KConfigDialog::showDialog("settings"))
        return;

    kgpgOptions *optionsDialog = new kgpgOptions(this, "settings");
    connect(optionsDialog, SIGNAL(settingsUpdated()), this, SLOT(readAllOptions()));
    connect(optionsDialog, SIGNAL(homeChanged()), this, SLOT(refreshkey()));
    connect(optionsDialog, SIGNAL(refreshTrust(int, QColor)), keysList2, SLOT(refreshTrust(int, QColor)));
    connect(optionsDialog, SIGNAL(changeFont(QFont)), this, SIGNAL(fontChanged(QFont)));
    connect(optionsDialog, SIGNAL(installShredder()), this, SIGNAL(installShredder()));
    optionsDialog->exec();
    delete optionsDialog;
}

void listKeys::readAllOptions()
{
    readOptions();
    emit readAgainOptions();
}

void listKeys::slotSetDefKey()
{
    slotSetDefaultKey(static_cast<KListViewItem*>(keysList2->currentItem()));
}

void listKeys::slotSetDefaultKey(QString newID)
{
    KListViewItem *newdef = static_cast<KListViewItem*>(keysList2->findItem(newID, 6));
    if (newdef)
        slotSetDefaultKey(newdef);
}

void listKeys::slotSetDefaultKey(KListViewItem *newdef)
{
    //kdDebug(2100)<<"------------------start ------------"<<endl;
    if ((!newdef) || (newdef->pixmap(2)==NULL))
        return;
    //kdDebug(2100)<<newdef->text(6)<<endl;
    //kdDebug(2100)<<KGpgSettings::defaultKey()<<endl;
    if (newdef->text(6)==KGpgSettings::defaultKey())
        return;
    if (newdef->pixmap(2)->serialNumber()!=keysList2->trustgood.serialNumber())
    {
        KMessageBox::sorry(this,i18n("Sorry, this key is not valid for encryption or not trusted."));
        return;
    }

    KListViewItem *olddef = static_cast<KListViewItem*>(keysList2->findItem(KGpgSettings::defaultKey(), 6));

    KGpgSettings::setDefaultKey(newdef->text(6));
    KGpgSettings::writeConfig();
    if (olddef)
        keysList2->refreshcurrentkey(olddef);
    keysList2->refreshcurrentkey(newdef);
    keysList2->ensureItemVisible(keysList2->currentItem());
}

void listKeys::slotmenu(Q3ListViewItem *sel2, const QPoint &pos, int)
{
    KListViewItem *sel = static_cast<KListViewItem*>(sel2);

    // popup a different menu depending on which key is selected
    if (sel != 0)
    {
        if (keysList2->selectedItems().count() > 1)
        {
            QList<Q3ListViewItem*> exportList = keysList2->selectedItems();
            bool keyDepth = true;
            for (int i = 0; i < exportList.count(); ++i)
                if (exportList.at(i))
                    if (exportList.at(i)->depth() != 0)
                        keyDepth = false;

            if (!keyDepth)
            {
                signKey->setEnabled(false);
                refreshKey->setEnabled(false);
                popupout->exec(pos);
                return;
            }
            else
            {
                signKey->setEnabled(true);
                refreshKey->setEnabled(true);
            }
        }

        if (sel->depth() != 0)
        {
            //kdDebug(2100) << sel->text(0) << endl;
            if ((sel->text(4) == "-") && (sel->text(6).startsWith("0x")))
            {
                if ((sel->text(2) == "-") || (sel->text(2) == i18n("Revoked")))
                {
                    if ((sel->text(0).startsWith("[")) && (sel->text(0).endsWith("]"))) // ugly hack to detect unknown keys
                        importSignatureKey->setEnabled(true);
                    else
                        importSignatureKey->setEnabled(false);
                    popupsig->exec(pos);
                    return;
                }
            }
            else
            if (sel->text(0) == i18n("Photo id"))
                popupphoto->exec(pos);
            else
            if (sel->text(6) == ("-"))
                popupuid->exec(pos);
        }
        else
        {
            keysList2->setSelected(sel, true);
            if (keysList2->currentItem()->text(6).isEmpty())
                popupgroup->exec(pos);
            else
            {
                if ((keysList2->secretList.find(sel->text(6)) != -1) && (keysList2->selectedItems().count() == 1))
                    popupsec->exec(pos);
                else
                if ((keysList2->orphanList.indexOf(sel->text(6)) != -1) && (keysList2->selectedItems().count() == 1))
                    popuporphan->exec(pos);
                else
                    popup->exec(pos);
            }
            return;
        }
    }
    else
        popupout->exec(pos);
}

void listKeys::slotrevoke(QString keyID, QString revokeUrl, int reason, QString description)
{
    revKeyProcess = new KgpgInterface();
    revKeyProcess->KgpgRevokeKey(keyID, revokeUrl, reason, description);
}

void listKeys::revokeWidget()
{
    KDialogBase *keyRevokeWidget = new KDialogBase(KDialogBase::Swallow, i18n("Create Revocation Certificate"),  KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, this, 0, true);
    KgpgRevokeWidget *keyRevoke = new KgpgRevokeWidget();

    keyRevoke->keyID->setText(keysList2->currentItem()->text(0) + " (" + keysList2->currentItem()->text(1) + ") " + i18n("ID: ") + keysList2->currentItem()->text(6));
    keyRevoke->kURLRequester1->setURL(QDir::homeDirPath() + "/" + keysList2->currentItem()->text(1).section('@', 0, 0) + ".revoke");
    keyRevoke->kURLRequester1->setMode(KFile::File);

    keyRevoke->setMinimumSize(keyRevoke->sizeHint());
    keyRevoke->show();
    keyRevokeWidget->setMainWidget(keyRevoke);

    if (keyRevokeWidget->exec() != QDialog::Accepted)
        return;
    if (keyRevoke->cbSave->isChecked())
    {
        slotrevoke(keysList2->currentItem()->text(6), keyRevoke->kURLRequester1->url(), keyRevoke->comboBox1->currentItem(), keyRevoke->textDescription->text());
        if (keyRevoke->cbPrint->isChecked())
            connect(revKeyProcess, SIGNAL(revokeurl(QString)), this, SLOT(doFilePrint(QString)));
        if (keyRevoke->cbImport->isChecked())
            connect(revKeyProcess, SIGNAL(revokeurl(QString)), this, SLOT(slotImportRevoke(QString)));
    }
    else
    {
        slotrevoke(keysList2->currentItem()->text(6), QString::null, keyRevoke->comboBox1->currentItem(), keyRevoke->textDescription->text());
        if (keyRevoke->cbPrint->isChecked())
            connect(revKeyProcess, SIGNAL(revokecertificate(QString)), this, SLOT(doPrint(QString)));
        if (keyRevoke->cbImport->isChecked())
            connect(revKeyProcess, SIGNAL(revokecertificate(QString)), this, SLOT(slotImportRevokeTxt(QString)));
    }
}

void listKeys::slotImportRevoke(QString url)
{
    KgpgInterface *importKeyProcess = new KgpgInterface();
    connect(importKeyProcess, SIGNAL(importKeyFinished(QStringList)), keysList2, SLOT(refreshselfkey()));
    importKeyProcess->importKey(KURL::fromPathOrURL(url));
}

void listKeys::slotImportRevokeTxt(QString revokeText)
{
    KgpgInterface *importKeyProcess = new KgpgInterface();
    connect(importKeyProcess, SIGNAL(importKeyFinished(QStringList)), keysList2, SLOT(refreshselfkey()));
    importKeyProcess->importKey(revokeText);
}

void listKeys::slotexportsec()
{
    // export secret key
    QString warn = i18n("Secret keys SHOULD NOT be saved in an unsafe place.\n"
                        "If someone else can access this file, encryption with this key will be compromised!\nContinue key export?");
    int result = KMessageBox::questionYesNo(this, warn, i18n("Warning"), i18n("Export"), i18n("Do Not Export"));
    if (result != KMessageBox::Yes)
        return;

    QString sname = keysList2->currentItem()->text(1).section('@', 0, 0);
    sname = sname.section('.', 0, 0);
    if (sname.isEmpty())
        sname = keysList2->currentItem()->text(0).section(' ', 0, 0);
    sname.append(".asc");
    sname.prepend(QDir::homeDirPath() + "/");
    KURL url = KFileDialog::getSaveURL(sname, "*.asc|*.asc Files", this, i18n("Export PRIVATE KEY As"));

    if(!url.isEmpty())
    {
        QFile fgpg(url.path());
        if (fgpg.exists())
            fgpg.remove();

        KProcIO *p = new KProcIO();
        *p << "gpg" << "--no-tty" << "--output" << QFile::encodeName(url.path()) << "--armor" << "--export-secret-keys" << keysList2->currentItem()->text(6);
        p->start(KProcess::Block);

        if (fgpg.exists())
            KMessageBox::information(this, i18n("Your PRIVATE key \"%1\" was successfully exported.\nDO NOT leave it in an insecure place.").arg(url.path()));
        else
            KMessageBox::sorry(this, i18n("Your secret key could not be exported.\nCheck the key."));
    }
}

void listKeys::slotexport()
{
    // export key
    if (keysList2->currentItem() == 0)
        return;
    if (keysList2->currentItem()->depth() != 0)
        return;

    QList<Q3ListViewItem*> exportList = keysList2->selectedItems();
    if (exportList.count() == 0)
        return;

    QString sname;

    if (exportList.count() == 1)
    {
        sname = keysList2->currentItem()->text(1).section('@', 0, 0);
        sname = sname.section('.', 0, 0);
        if (sname.isEmpty())
            sname = keysList2->currentItem()->text(0).section(' ', 0, 0);
    }
    else
        sname = "keyring";

    sname.append(".asc");
    sname.prepend(QDir::homeDirPath() + "/");

    KDialogBase *dial = new KDialogBase(KDialogBase::Swallow, i18n("Public Key Export"), KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, this, "key_export",true);

    KeyExport *page = new KeyExport(dial);
    dial->setMainWidget(page);
    page->newFilename->setURL(sname);
    page->newFilename->setCaption(i18n("Save File"));
    page->newFilename->setMode(KFile::File);
    page->show();

    if (dial->exec() == QDialog::Accepted)
    {
        // export to file
        QString expname;
        bool exportAttr = page->exportAttributes->isChecked();
        if (page->checkServer->isChecked())
        {
            keyServer *expServer = new keyServer(0, "server_export", false);
            expServer->slotSetExportAttribute(exportAttr);
            QString exportKeysList;
            for (int i = 0; i < exportList.count(); ++i)
                if (exportList.at(i))
                    exportKeysList.append(" " + exportList.at(i)->text(6).simplified());

                expServer->slotExport(exportKeysList);
                return;
        }
        else
        if (page->checkFile->isChecked())
        {
            KProcIO *p = new KProcIO();
            *p << "gpg" << "--no-tty";

            expname = page->newFilename->url().simplified();
            if (!expname.isEmpty())
            {
                QFile fgpg(expname);
                if (fgpg.exists())
                    fgpg.remove();

                *p << "--output" << QFile::encodeName(expname) << "--export" << "--armor";
                if (!exportAttr)
                    *p << "--export-options" << "no-include-attributes";

                for (int i = 0; i < exportList.count(); ++i)
                    if (exportList.at(i))
                        *p << (exportList.at(i)->text(6)).simplified();

                p->start(KProcess::Block);

                if (fgpg.exists())
                    KMessageBox::information(this, i18n("Your public key \"%1\" was successfully exported\n").arg(expname));
                else
                    KMessageBox::sorry(this, i18n("Your public key could not be exported\nCheck the key."));
            }
        }
        else
        {
            QStringList klist;
            for (int i = 0; i < exportList.count(); ++i)
                if (exportList.at(i))
                    klist.append(exportList.at(i)->text(6).simplified());

            KgpgInterface *kexp = new KgpgInterface();
            QString result = kexp->getKeys(true, exportAttr, klist);
            delete kexp;

            if (page->checkClipboard->isChecked())
                slotProcessExportClip(result);
            else
                slotProcessExportMail(result);
        }
    }

    delete dial;
}

void listKeys::slotProcessExportMail(QString keys)
{
    // start default Mail application
    KToolInvocation::invokeMailer(QString::null, QString::null, QString::null, QString::null, keys);
}

void listKeys::slotProcessExportClip(QString keys)
{
    kapp->clipboard()->setText(keys, clipboardMode);
}

void listKeys::showKeyInfo(QString keyID)
{
    KgpgKeyInfo *opts = new KgpgKeyInfo(keyID, this, "key_props");
    opts->show();
}

void listKeys::slotShowPhoto()
{
    KTrader::OfferList offers = KTrader::self()->query("image/jpeg", "Type == 'Application'");
    KService::Ptr ptr = offers.first();
    //KMessageBox::sorry(0,ptr->desktopEntryName());
    KProcIO *p = new KProcIO();
    *p << "gpg" << "--no-tty" << "--photo-viewer" << QFile::encodeName(ptr->desktopEntryName() + " %i") << "--edit-key" << keysList2->currentItem()->parent()->text(6) << "uid" << keysList2->currentItem()->text(6) << "showphoto" << "quit";
    p->start(KProcess::DontCare, true);
}

void listKeys::listsigns()
{
    // kdDebug(2100) << "Edit -------------------------------" << endl;
    if (keysList2->currentItem() == 0)
        return;

    if (keysList2->currentItem()->depth() != 0)
    {
        if (keysList2->currentItem()->text(0) == i18n("Photo id"))
        {
            // display photo
            slotShowPhoto();
        }
        return;
    }

    if (keysList2->currentItem()->pixmap(0)->serialNumber() == keysList2->pixkeyOrphan.serialNumber())
    {
        if (KMessageBox::questionYesNo(this, i18n("This key is an orphaned secret key (secret key without public key.) It is currently not usable.\n\n"
                                               "Would you like to regenerate the public key?"), QString::null, i18n("Generate"), i18n("Do Not Generate")) == KMessageBox::Yes)
            slotregenerate();
            return;
    }

    // open a key info dialog (KgpgKeyInfo, see begining of this file)
    QString key = keysList2->currentItem()->text(6);
    if (!key.isEmpty())
    {
        KgpgKeyInfo *opts = new KgpgKeyInfo(key, this, "key_props");
        connect(opts, SIGNAL(keyNeedsRefresh()), keysList2, SLOT(refreshselfkey()));
        opts->exec();
        delete opts;
    }
    else
        editGroup();
}

void listKeys::groupAdd()
{
    QList<Q3ListViewItem*> addList = gEdit->availableKeys->selectedItems();
    for (int i = 0; i < addList.count(); ++i)
        if (addList.at(i))
            gEdit->groupKeys->insertItem(addList.at(i));
}

void listKeys::groupRemove()
{
    QList<Q3ListViewItem*> remList = gEdit->groupKeys->selectedItems();
    for (int i = 0; i < remList.count(); ++i)
        if (remList.at(i))
            gEdit->availableKeys->insertItem(remList.at(i));
}

void listKeys::deleteGroup()
{
    if (!keysList2->currentItem() || !keysList2->currentItem()->text(6).isEmpty())
        return;

    int result = KMessageBox::warningContinueCancel(this, i18n("<qt>Are you sure you want to delete group <b>%1</b> ?</qt>").arg(keysList2->currentItem()->text(0)), i18n("Warning"), KGuiItem(i18n("Delete"), "editdelete"));
    if (result != KMessageBox::Continue)
        return;

    KgpgInterface::delGpgGroup(keysList2->currentItem()->text(0), KGpgSettings::gpgConfigPath());
    KListViewItem *item = static_cast<KListViewItem*>(keysList2->currentItem()->nextSibling());
    delete keysList2->currentItem();

    if (!item)
        item = static_cast<KListViewItem*>(keysList2->lastChild());

    keysList2->setCurrentItem(item);
    keysList2->setSelected(item,true);

    QStringList groups = KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
    KGpgSettings::setGroups(groups.join(","));
    keysList2->groupNb = groups.count();
    changeMessage(i18n("%1 Keys, %2 Groups").arg(keysList2->childCount()-keysList2->groupNb).arg(keysList2->groupNb), 1);
}

void listKeys::groupChange()
{
    QStringList selected;
    KListViewItem *item = static_cast<KListViewItem*>(gEdit->groupKeys->firstChild());
    while (item)
    {
        selected += item->text(2);
        item = static_cast<KListViewItem*>(item->nextSibling());
    }
    KgpgInterface::setGpgGroupSetting(keysList2->currentItem()->text(0), selected,KGpgSettings::gpgConfigPath());
}

void listKeys::createNewGroup()
{
    QStringList badkeys;
    QStringList keysGroup;

    if (keysList2->selectedItems().count() > 0)
    {
        QList<Q3ListViewItem*> groupList = keysList2->selectedItems();
        bool keyDepth = true;
        for (int i = 0; i < groupList.count(); ++i)
            if (groupList.at(i))
            {
                if (groupList.at(i)->depth() != 0)
                    keyDepth = false;
                else
                if (groupList.at(i)->text(6).isEmpty())
                    keyDepth = false;
                else
                if (groupList.at(i)->pixmap(2))
                {
                    if (groupList.at(i)->pixmap(2)->serialNumber() == keysList2->trustgood.serialNumber())
                        keysGroup += groupList.at(i)->text(6);
                    else
                        badkeys += groupList.at(i)->text(0) + " (" + groupList.at(i)->text(1) + ") " + groupList.at(i)->text(6);
                }
            }

        if (!keyDepth)
        {
            KMessageBox::sorry(this, i18n("<qt>You cannot create a group containing signatures, subkeys or other groups.</qt>"));
            return;
        }

        QString groupName = KInputDialog::getText(i18n("Create New Group"), i18n("Enter new group name:"), QString::null, 0, this);
        if (groupName.isEmpty())
            return;
        if (!keysGroup.isEmpty())
        {
            if (!badkeys.isEmpty())
                KMessageBox::informationList(this, i18n("Following keys are not valid or not trusted and will not be added to the group:"), badkeys);

            KgpgInterface::setGpgGroupSetting(groupName, keysGroup, KGpgSettings::gpgConfigPath());
            QStringList groups = KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
            KGpgSettings::setGroups(groups.join(","));
            keysList2->refreshGroups();
            KListViewItem *newgrp = static_cast<KListViewItem*>(keysList2->findItem(groupName, 0));

            keysList2->clearSelection();
            keysList2->setCurrentItem(newgrp);
            keysList2->setSelected(newgrp, true);
            keysList2->ensureItemVisible(newgrp);
            keysList2->groupNb = groups.count();
            changeMessage(i18n("%1 Keys, %2 Groups").arg(keysList2->childCount() - keysList2->groupNb).arg(keysList2->groupNb), 1);
        }
        else
            KMessageBox::sorry(this, i18n("<qt>No valid or trusted key was selected. The group <b>%1</b> will not be created.</qt>").arg(groupName));
    }
}

void listKeys::groupInit(QStringList keysGroup)
{
    kdDebug(2100) << "preparing group" << endl;
    QStringList lostKeys;
    bool foundId;

    for (QStringList::Iterator it = keysGroup.begin(); it != keysGroup.end(); ++it)
    {
        KListViewItem *item = static_cast<KListViewItem*>(gEdit->availableKeys->firstChild());
        foundId = false;
        while (item)
        {
            kdDebug(2100) << "Searching in key: " << item->text(0) << endl;
            if (QString(*it).right(8).toLower() == item->text(2).right(8).toLower())
            {
                gEdit->groupKeys->insertItem(item);
                foundId = true;
                break;
            }
            item = static_cast<KListViewItem*>(item->nextSibling());
        }
        if (!foundId)
            lostKeys += QString(*it);
    }

    if (!lostKeys.isEmpty())
        KMessageBox::informationList(this, i18n("Following keys are in the group but are not valid or not in your keyring. They will be removed from the group."), lostKeys);
}

void listKeys::editGroup()
{
    if (!keysList2->currentItem() || !keysList2->currentItem()->text(6).isEmpty())
        return;
    QStringList keysGroup;
    //KDialogBase *dialogGroupEdit=new KDialogBase( this, "edit_group", true,i18n("Group Properties"),KDialogBase::Ok | KDialogBase::Cancel);
    KDialogBase *dialogGroupEdit = new KDialogBase(KDialogBase::Swallow, i18n("Group Properties"), KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, this, 0, true);

    gEdit = new groupEdit();
    gEdit->buttonAdd->setPixmap(KGlobal::iconLoader()->loadIcon("down", KIcon::Small, 20));
    gEdit->buttonRemove->setPixmap(KGlobal::iconLoader()->loadIcon("up", KIcon::Small, 20));

    connect(gEdit->buttonAdd, SIGNAL(clicked()), this, SLOT(groupAdd()));
    connect(gEdit->buttonRemove, SIGNAL(clicked()), this, SLOT(groupRemove()));
    // connect(dialogGroupEdit->okClicked(),SIGNAL(clicked()),this,SLOT(groupChange()));
    connect(gEdit->availableKeys, SIGNAL(doubleClicked (Q3ListViewItem *, const QPoint &, int)), this, SLOT(groupAdd()));
    connect(gEdit->groupKeys, SIGNAL(doubleClicked (Q3ListViewItem *, const QPoint &, int)), this, SLOT(groupRemove()));
    KListViewItem *item = static_cast<KListViewItem*>(keysList2->firstChild());
    if (!item)
        return;

    if (item->pixmap(2))
        if (item->pixmap(2)->serialNumber() == keysList2->trustgood.serialNumber())
            (void) new KListViewItem(gEdit->availableKeys, item->text(0), item->text(1), item->text(6));

    while (item->nextSibling())
    {
        item = static_cast<KListViewItem*>(item->nextSibling());
        if (item->pixmap(2))
            if (item->pixmap(2)->serialNumber() == keysList2->trustgood.serialNumber())
                (void) new KListViewItem(gEdit->availableKeys, item->text(0), item->text(1), item->text(6));
    }

    keysGroup = KgpgInterface::getGpgGroupSetting(keysList2->currentItem()->text(0), KGpgSettings::gpgConfigPath());
    groupInit(keysGroup);
    dialogGroupEdit->setMainWidget(gEdit);
    gEdit->availableKeys->setColumnWidth(0, 200);
    gEdit->availableKeys->setColumnWidth(1, 200);
    gEdit->availableKeys->setColumnWidth(2, 100);
    gEdit->availableKeys->setColumnWidthMode(0, KListView::Manual);
    gEdit->availableKeys->setColumnWidthMode(1, KListView::Manual);
    gEdit->availableKeys->setColumnWidthMode(2, KListView::Manual);

    gEdit->groupKeys->setColumnWidth(0, 200);
    gEdit->groupKeys->setColumnWidth(1, 200);
    gEdit->groupKeys->setColumnWidth(2, 100);
    gEdit->groupKeys->setColumnWidthMode(0, KListView::Manual);
    gEdit->groupKeys->setColumnWidthMode(1, KListView::Manual);
    gEdit->groupKeys->setColumnWidthMode(2, KListView::Manual);

    gEdit->setMinimumSize(gEdit->sizeHint());
    gEdit->show();
    if (dialogGroupEdit->exec() == QDialog::Accepted)
        groupChange();
    delete dialogGroupEdit;
}

void listKeys::signkey()
{
    if (keysList2->currentItem() == 0)
        return;
    if (keysList2->currentItem()->depth() != 0)
        return;

    signList = keysList2->selectedItems();
    bool keyDepth = true;
    for (int i = 0; i < signList.count(); ++i)
        if (signList.at(i))
            if (signList.at(i)->depth() != 0)
                keyDepth = false;

    if (!keyDepth)
    {
        KMessageBox::sorry(this, i18n("You can only sign primary keys. Please check your selection."));
        return;
    }

    if (signList.count() == 1)
    {
        QString fingervalue;
        QString opt;

        QStringList list(keysList2->currentItem()->text(6));
        KgpgInterface *interface = new KgpgInterface();
        KgpgListKeys listkeys = interface->readPublicKeys(true, list);
        delete interface;
        fingervalue = listkeys.at(0).fingerprint();

        opt = i18n("<qt>You are about to sign key:<br><br>%1<br>ID: %2<br>Fingerprint: <br><b>%3</b>.<br><br>"
                   "You should check the key fingerprint by phoning or meeting the key owner to be sure that someone "
                   "is not trying to intercept your communications</qt>").arg(keysList2->currentItem()->text(0) + " (" + keysList2->currentItem()->text(1) + ")").arg(keysList2->currentItem()->text(6)).arg(fingervalue);

        if (KMessageBox::warningContinueCancel(this, opt) != KMessageBox::Continue)
            return;
    }
    else
    {
        QStringList signKeyList;
        for (int i = 0; i < signList.count(); ++i)
            if (signList.at(i))
                signKeyList += signList.at(i)->text(0) + " (" + signList.at(i)->text(1) + ")" + ": " + signList.at(i)->text(6);

        if (KMessageBox::warningContinueCancelList(this, i18n("<qt>You are about to sign the following keys in one pass.<br><b>If you have not carefully checked all fingerprints, the security of your communications may be compromised.</b></qt>"), signKeyList) != KMessageBox::Continue)
            return;
    }

    KgpgSelectSecretKey *opts = new KgpgSelectSecretKey(this, 0, true, signList.count());
    if (opts->exec() != QDialog::Accepted)
    {
        delete opts;
        return;
    }

    globalkeyID = QString(opts->getKeyID());
    globalkeyMail = QString(opts->getKeyMail());
    globalisLocal = opts->isLocalSign();
    globalChecked = opts->getSignTrust();
    globalCount = signList.count();
    m_isterminal = opts->isTerminalSign();
    keyCount = 0;
    delete opts;

    signLoop();
}

void listKeys::signLoop()
{
    if (keyCount < globalCount)
    {
        kdDebug(2100) << "Sign process for key: " << keyCount + 1 << " on a total of " << signList.count() << endl;
        if (signList.at(keyCount))
        {
            KgpgInterface *interface = new KgpgInterface();
            interface->signKey(signList.at(keyCount)->text(6), globalkeyID, globalisLocal, globalChecked, m_isterminal);
            connect(interface, SIGNAL(signKeyFinished(int, KgpgInterface*)), this, SLOT(signatureResult(int, KgpgInterface*)));
        }
    }
}

void listKeys::signatureResult(int success, KgpgInterface *interface)
{
    delete interface;
    if (success == 2)
        keysList2->refreshcurrentkey(static_cast<KListViewItem*>(signList.at(keyCount)));
    else
    if (success == 1)
        KMessageBox::sorry(this, i18n("<qt>Bad passphrase, key <b>%1</b> not signed.</qt>").arg(signList.at(keyCount)->text(0) + i18n(" (") + signList.at(keyCount)->text(1) + i18n(")")));
    else
    if (success == 4)
        KMessageBox::sorry(this, i18n("<qt>The key <b>%1</b> is already signed.</qt>").arg(signList.at(keyCount)->text(0) + i18n(" (") + signList.at(keyCount)->text(1) + i18n(")")));

    keyCount++;
    signLoop();
}


void listKeys::importallsignkey()
{
    if (keysList2->currentItem() == 0)
        return;

    if (!keysList2->currentItem()->firstChild())
    {
        keysList2->currentItem()->setOpen(true);
        keysList2->currentItem()->setOpen(false);
    }

    QString missingKeysList;
    KListViewItem *current = static_cast<KListViewItem*>(keysList2->currentItem()->firstChild());
    while (current)
    {
        if ((current->text(0).startsWith("[")) && (current->text(0).endsWith("]"))) // ugly hack to detect unknown keys
            missingKeysList += current->text(6) + " ";
        current = static_cast<KListViewItem*>(current->nextSibling());
    }

    if (!missingKeysList.isEmpty())
        importsignkey(missingKeysList);
    else
        KMessageBox::information(this, i18n("All signatures for this key are already in your keyring"));
}

void listKeys::preimportsignkey()
{
    if (keysList2->currentItem() == NULL)
        return;
    else
        importsignkey(keysList2->currentItem()->text(6));
}

bool listKeys::importRemoteKey(QString keyID)
{
    kServer = new keyServer(0, "server_dialog", false, true);
    kServer->slotSetText(keyID);
    //kServer->page->Buttonimport->setDefault(true);
    //kServer->page->tabWidget2->setTabEnabled(kServer->page->tabWidget2->page(1),false);
    kServer->show();
    kServer->raise();
    connect(kServer, SIGNAL(importFinished(QString)), this, SLOT( dcopImportFinished()));

    return true;
}

void listKeys::dcopImportFinished()
{
    if (kServer)
        kServer = 0L;

    QByteArray params;
    QDataStream stream(&params, QIODevice::WriteOnly);

    stream.setVersion(QDataStream::Qt_3_1);
    stream << true;
    kapp->dcopClient()->emitDCOPSignal("keyImported(bool)", params);
    refreshkey();
}

void listKeys::importsignkey(QString importKeyId)
{
    // sign a key
    kServer = new keyServer(0, "server_dialog", false);
    kServer->slotSetText(importKeyId);
    //kServer->Buttonimport->setDefault(true);
    kServer->slotImport();
    //kServer->show();
    connect(kServer, SIGNAL(importFinished(QString)), this, SLOT(importfinished()));
}

void listKeys::importfinished()
{
    if (kServer)
        kServer = 0L;
    refreshkey();
}

void listKeys::delsignkey()
{
    // sign a key
    if (keysList2->currentItem() == 0)
        return;

    if (keysList2->currentItem()->depth() > 1)
    {
        KMessageBox::sorry(this, i18n("Edit key manually to delete this signature."));
        return;
    }

    QString signID;
    QString parentKey;
    QString signMail;
    QString parentMail;

    // open a key selection dialog (KgpgSelectSecretKey, see begining of this file)
    parentKey = keysList2->currentItem()->parent()->text(6);
    signID = keysList2->currentItem()->text(6);
    parentMail = keysList2->currentItem()->parent()->text(0) + " (" + keysList2->currentItem()->parent()->text(1) + ")";
    signMail = keysList2->currentItem()->text(0) + " (" + keysList2->currentItem()->text(1) + ")";

    if (parentKey == signID)
    {
        KMessageBox::sorry(this, i18n("Edit key manually to delete a self-signature."));
        return;
    }

    QString ask = i18n("<qt>Are you sure you want to delete signature<br><b>%1</b> from key:<br><b>%2</b>?</qt>").arg(signMail).arg(parentMail);

    if (KMessageBox::questionYesNo(this, ask, QString::null, KStdGuiItem::del(), KStdGuiItem::cancel()) != KMessageBox::Yes)
        return;

    KgpgInterface *delSignKeyProcess = new KgpgInterface();
    connect(delSignKeyProcess, SIGNAL(delsigfinished(bool)), this, SLOT(delsignatureResult(bool)));
    delSignKeyProcess->KgpgDelSignature(parentKey, signID);
}

void listKeys::delsignatureResult(bool success)
{
    if (success)
    {
        KListViewItem *top = static_cast<KListViewItem*>(keysList2->currentItem());
        while (top->depth() != 0)
            top = static_cast<KListViewItem*>(top->parent());
        while (top->firstChild() != 0)
            delete top->firstChild();
        keysList2->refreshcurrentkey(top);
    }
    else
        KMessageBox::sorry(this, i18n("Requested operation was unsuccessful, please edit the key manually."));
}

void listKeys::slotedit()
{
    if (!keysList2->currentItem())
        return;
    if (keysList2->currentItem()->depth() != 0)
        return;
    if (keysList2->currentItem()->text(6).isEmpty())
        return;

    KProcess kp;
    KConfig *config = KGlobal::config();
    config->setGroup("General");
    kp << config->readPathEntry("TerminalApplication","konsole");
    kp << "-e" << "gpg" <<"--no-secmem-warning" <<"--edit-key" << keysList2->currentItem()->text(6) << "help";
    kp.start(KProcess::Block);
    keysList2->refreshcurrentkey(static_cast<KListViewItem*>(keysList2->currentItem()));
}

void listKeys::slotgenkey()
{
    // generate key
    KgpgKeyGenerate *genkey = new KgpgKeyGenerate(this, 0);
    if (genkey->exec() == QDialog::Accepted)
    {
        if (!genkey->getmode()) // normal mode
        {
            // extract data from genkey
            int kexp = genkey->getkeyexp();
            QString ktype = genkey->getkeytype();
            QString ksize = genkey->getkeysize();
            QString knumb = genkey->getkeynumb();
            QString kcomment = genkey->getkeycomm();
            newKeyName = genkey->getkeyname();
            newKeyMail = genkey->getkeymail();
            delete genkey;

            QByteArray password;
            bool goodpass = false;
            while (!goodpass)
            {
                int code = KPasswordDialog::getNewPassword(this, password, i18n("<b>Enter passphrase for %1</b>:<br>Passphrase should include non alphanumeric characters and random sequences").arg(newKeyName+" <"+newKeyMail+">"));
                if (code != QDialog::Accepted)
                    return;

                if (password.length() < 5)
                    KMessageBox::sorry(this, i18n("This passphrase is not secure enough.\nMinimum length= 5 characters"));
                else
                    goodpass = true;
            }

            pop = new KPassivePopup((QWidget*)parent(), "new_key");
            pop->setTimeout(0);

            QWidget *wid = new QWidget(pop);
            QVBoxLayout *vbox = new QVBoxLayout(wid,3);

            KVBox *passiveBox = pop->standardView(i18n("Generating new key pair."), QString::null, KGlobal::iconLoader()->loadIcon("kgpg", KIcon::Desktop), wid);

            QMovie anim(locate("appdata", "pics/kgpg_anim.gif"));

            QLabel *tex = new QLabel(wid);
            QLabel *tex2 = new QLabel(wid);
            tex->setAlignment(Qt::AlignHCenter);
            tex->setMovie(&anim);
            tex2->setText(i18n("\nPlease wait..."));
            vbox->addWidget(passiveBox);
            vbox->addWidget(tex);
            vbox->addWidget(tex2);

            pop->setView(wid);
            pop->show();

            changeMessage(i18n("Generating New Key..."), 0, true);

            QRect qRect(QApplication::desktop()->screenGeometry());
            int iXpos = qRect.width() / 2 - pop->width() / 2;
            int iYpos = qRect.height() / 2 - pop->height() / 2;
            pop->move(iXpos, iYpos);
            pop->setAutoDelete(false);

            KProcIO *proc = new KProcIO();
            message = QString::null;

            //*proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--batch"<<"--passphrase-fd"<<res<<"--gen-key"<<"-a"<<"kgpg.tmp";
            *proc << "gpg" << "--no-tty" << "--status-fd=2" << "--no-secmem-warning" << "--batch" << "--gen-key";

            // when process ends, update dialog infos
            connect(proc, SIGNAL(processExited(KProcess*)), this, SLOT(genover(KProcess*)));

            proc->start(KProcess::NotifyOnExit, true);

            if (ktype == "RSA")
                proc->writeStdin(QString("Key-Type: 1"));
            else
            {
                proc->writeStdin(QString("Key-Type: DSA"));
                proc->writeStdin(QString("Subkey-Type: ELG-E"));
                proc->writeStdin(QString("Subkey-Length:").append(ksize));
            }

            proc->writeStdin(QByteArray("Passphrase:").append(password));
            proc->writeStdin(QString("Key-Length:").append(ksize));
            proc->writeStdin(QString("Name-Real:").append(newKeyName.toUtf8()));

            if (!newKeyMail.isEmpty())
                proc->writeStdin(QString("Name-Email:").append(newKeyMail));
            if (!kcomment.isEmpty())
                proc->writeStdin(QString("Name-Comment:").append(kcomment.toUtf8()));
            if (kexp == 0)
                proc->writeStdin(QString("Expire-Date:0"));
            if (kexp == 1)
                proc->writeStdin(QString("Expire-Date:").append(knumb));
            if (kexp == 2)
                proc->writeStdin(QString("Expire-Date:%1w").arg(knumb));
            if (kexp == 3)
                proc->writeStdin(QString("Expire-Date:%1m").arg(knumb));
            if (kexp == 4)
                proc->writeStdin(QString("Expire-Date:%1y").arg(knumb));

            proc->writeStdin(QString("%commit"));
            connect(proc, SIGNAL(readReady(KProcIO *)), this, SLOT(readgenprocess(KProcIO *)));
            proc->closeWhenDone();
        }
        else // start expert (=konsole) mode
        {
            KProcess kp;
            KConfig *config = KGlobal::config();
            config->setGroup("General");
            kp << config->readPathEntry("TerminalApplication", "konsole");
            kp << "-e" <<"gpg" <<"--gen-key";
            kp.start(KProcess::Block);
            refreshkey();
        }
    }
}

void listKeys::readgenprocess(KProcIO *p)
{
    QString required;
    while (p->readln(required,true) != -1)
    {
        if (required.find("KEY_CREATED") != -1)
            newkeyFinger = required.simplified().section(' ', -1);
        message += required + "\n";
    }
    // sample:   [GNUPG:] KEY_CREATED B 156A4305085A58C01E2988229282910254D1B368
}

void listKeys::genover(KProcess *)
{
    newkeyID = QString::null;
    continueSearch = true;
    KProcIO *conprocess = new KProcIO();
    *conprocess<< "gpg";
    *conprocess<<"--no-secmem-warning"<<"--with-colon"<<"--fingerprint"<<"--list-keys"<<newKeyName;
    connect(conprocess, SIGNAL(readReady(KProcIO *)), this, SLOT(slotReadFingerProcess(KProcIO *)));
    connect(conprocess, SIGNAL(processExited(KProcess *)), this, SLOT(newKeyDone(KProcess *)));
    conprocess->start(KProcess::NotifyOnExit,true);
}

void listKeys::slotReadFingerProcess(KProcIO *p)
{
    QString outp;
    while (p->readln(outp) != -1)
    {
        if (outp.startsWith("pub") && (continueSearch))
            newkeyID = outp.section(':', 4, 4).right(8).prepend("0x");

        if (outp.startsWith("fpr"))
            if (newkeyFinger.toLower() == outp.section(':', 9, 9).toLower())
                continueSearch = false;
                // kdDebug(2100)<<newkeyFinger<<" test:"<<outp.section(':',9,9)<<endl;
    }
}

void listKeys::newKeyDone(KProcess *)
{
    changeMessage(i18n("Ready"), 0);
    // refreshkey();
    if (newkeyID.isEmpty())
    {
        delete pop;
        KMessageBox::detailedSorry(this, i18n("Something unexpected happened during the key pair creation.\nPlease check details for full log output."), message);
        refreshkey();
        return;
    }

    keysList2->refreshKeys(QStringList(newkeyID));
    changeMessage(i18n("%1 Keys, %2 Groups").arg(keysList2->childCount() - keysList2->groupNb).arg(keysList2->groupNb), 1);
    KDialogBase *keyCreated = new KDialogBase(this, "key_created", true, i18n("New Key Pair Created"), KDialogBase::Ok);
    newKey *page = new newKey(keyCreated);
    page->TLname->setText("<b>" + newKeyName + "</b>");
    page->TLemail->setText("<b>" + newKeyMail + "</b>");

    if (!newKeyMail.isEmpty())
        page->kURLRequester1->setURL(QDir::homeDirPath() + "/" + newKeyMail.section("@", 0, 0) + ".revoke");
    else
        page->kURLRequester1->setURL(QDir::homeDirPath() + "/" + newKeyName.section(" ", 0, 0) + ".revoke");

    page->TLid->setText("<b>" + newkeyID + "</b>");
    page->LEfinger->setText(newkeyFinger);
    page->CBdefault->setChecked(true);
    page->show();
    //page->resize(page->maximumSize());
    keyCreated->setMainWidget(page);
    delete pop;

    keyCreated->exec();

    KListViewItem *newdef = static_cast<KListViewItem*>(keysList2->findItem(newkeyID, 6));
    if (newdef)
        if (page->CBdefault->isChecked())
            slotSetDefaultKey(newdef);
        else
        {
            keysList2->clearSelection();
            keysList2->setCurrentItem(newdef);
            keysList2->setSelected(newdef,true);
            keysList2->ensureItemVisible(newdef);
        }

    if (page->CBsave->isChecked())
    {
        slotrevoke(newkeyID, page->kURLRequester1->url(), 0, i18n("backup copy"));
        if (page->CBprint->isChecked())
            connect(revKeyProcess, SIGNAL(revokeurl(QString)), this, SLOT(doFilePrint(QString)));
    }
    else
    if (page->CBprint->isChecked())
    {
        slotrevoke(newkeyID, QString::null, 0, i18n("backup copy"));
        connect(revKeyProcess, SIGNAL(revokecertificate(QString)), this, SLOT(doPrint(QString)));
    }
}

void listKeys::doFilePrint(QString url)
{
    QFile qfile(url);
    if (qfile.open(QIODevice::ReadOnly))
    {
        QTextStream t(&qfile);
        doPrint(t.read());
    }
    else
        KMessageBox::sorry(this, i18n("<qt>Cannot open file <b>%1</b> for printing...</qt>").arg(url));
}

void listKeys::doPrint(QString txt)
{
    KPrinter prt;
    //kdDebug(2100) << "Printing..." << endl;
    if (prt.setup(this))
    {
        QPainter painter(&prt);
        int width = painter.device()->width();
        int height = painter.device()->height();
        painter.drawText(0, 0, width, height, Qt::AlignLeft|Qt::AlignTop|Qt::TextDontClip, txt);
    }
}

void listKeys::deleteseckey()
{
    // delete a key
    QString res = keysList2->currentItem()->text(0) + " (" + keysList2->currentItem()->text(1) + ")";
    int result = KMessageBox::warningContinueCancel(this,
                        i18n("<p>Delete <b>SECRET KEY</b> pair <b>%1</b>?</p>Deleting this key pair means you will never be able to decrypt files encrypted with this key again.").arg(res),
                        i18n("Warning"),
                        KGuiItem(i18n("Delete"),"editdelete"));
    if (result != KMessageBox::Continue)
        return;

    KProcess *conprocess = new KProcess();
    KConfig *config = KGlobal::config();
    config->setGroup("General");
    *conprocess<< config->readPathEntry("TerminalApplication","konsole");
    *conprocess<<"-e"<<"gpg"
    <<"--no-secmem-warning"
    <<"--delete-secret-key"<<keysList2->currentItem()->text(6);
    connect(conprocess, SIGNAL(processExited(KProcess *)), this, SLOT(reloadSecretKeys()));
    conprocess->start(KProcess::NotifyOnExit, KProcess::AllOutput);
}

void listKeys::reloadSecretKeys()
{
    FILE *fp;
    char line[300];
    keysList2->secretList = QString::null;
    fp = popen("gpg --no-secmem-warning --no-tty --with-colon --list-secret-keys", "r");
    while (fgets(line, sizeof(line), fp))
    {
        QString lineRead = line;
        if (lineRead.startsWith("sec"))
            keysList2->secretList += "0x" + lineRead.section(':', 4, 4).right(8) + ",";
        }
    pclose(fp);
    deletekey();
}

void listKeys::confirmdeletekey()
{
    if (keysList2->currentItem()->depth() != 0)
    {
        if ((keysList2->currentItem()->depth() == 1) && (keysList2->currentItem()->text(4) == "-") && (keysList2->currentItem()->text(6).startsWith("0x")))
            delsignkey();
        return;
    }

    if (keysList2->currentItem()->text(6).isEmpty())
    {
        deleteGroup();
        return;
    }

    if (((keysList2->secretList.find(keysList2->currentItem()->text(6)) != -1) || (keysList2->orphanList.indexOf(keysList2->currentItem()->text(6)) != -1)) && (keysList2->selectedItems().count() == 1))
        deleteseckey();
    else
    {
        QStringList keysToDelete;
        QString secList;
        QList<Q3ListViewItem*> exportList = keysList2->selectedItems();
        bool secretKeyInside = false;
        for (int i = 0; i < exportList.count(); ++i)
            if (exportList.at(i))
            {
                if (keysList2->secretList.find(exportList.at(i)->text(6)) != -1)
                {
                    secretKeyInside = true;
                    secList += exportList.at(i)->text(0) + " (" + exportList.at(i)->text(1) + ")<br>";
                    exportList.at(i)->setSelected(false);
                }
                else
                    keysToDelete += exportList.at(i)->text(0) + " (" + exportList.at(i)->text(1) + ")";
            }

        if (secretKeyInside)
        {
            int result = KMessageBox::warningContinueCancel(this, i18n("<qt>The following are secret key pairs:<br><b>%1</b>They will not be deleted.<br></qt>").arg(secList));
            if (result != KMessageBox::Continue)
                return;
        }

        if (keysToDelete.isEmpty())
            return;

        int result = KMessageBox::warningContinueCancelList(this, i18n("<qt><b>Delete the following public key?</b></qt>", "<qt><b>Delete the following %n public keys?</b></qt>", keysToDelete.count()), keysToDelete, i18n("Warning"), KStdGuiItem::del());
        if (result != KMessageBox::Continue)
            return;
        else
            deletekey();
    }
}

void listKeys::deletekey()
{
    QList<Q3ListViewItem*> exportList = keysList2->selectedItems();
    if (exportList.count() == 0)
        return;

    KProcess gp;
    gp << "gpg"
    << "--no-tty"
    << "--no-secmem-warning"
    << "--batch"
    << "--yes"
    << "--delete-key";

    for (int i = 0; i < exportList.count(); ++i)
        if (exportList.at(i))
            gp << (exportList.at(i)->text(6)).simplified();

    gp.start(KProcess::Block);

    for (int i = 0; i < exportList.count(); ++i)
        if (exportList.at(i))
            keysList2->refreshcurrentkey(static_cast<KListViewItem*>(exportList.at(i)));

    if (keysList2->currentItem())
    {
        KListViewItem * myChild = static_cast<KListViewItem*>(keysList2->currentItem());
        while(!myChild->isVisible())
        {
            myChild = static_cast<KListViewItem*>(myChild->nextSibling());
            if (!myChild)
                break;
        }

        if (!myChild)
        {
            KListViewItem * myChild = static_cast<KListViewItem*>(keysList2->firstChild());
            while(!myChild->isVisible())
            {
                myChild = static_cast<KListViewItem*>(myChild->nextSibling());
                if (!myChild)
                    break;
            }
        }

        if (myChild)
        {
            myChild->setSelected(true);
            keysList2->setCurrentItem(myChild);
        }
    }
    else
        stateChanged("empty_list");

    changeMessage(i18n("%1 Keys, %2 Groups").arg(keysList2->childCount() - keysList2->groupNb).arg(keysList2->groupNb), 1);
}

void listKeys::slotPreImportKey()
{
    KDialogBase *dial = new KDialogBase(KDialogBase::Swallow, i18n("Key Import"), KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, this, "key_import", true);

    SrcSelect *page = new SrcSelect();
    dial->setMainWidget(page);
    page->newFilename->setCaption(i18n("Open File"));
    page->newFilename->setMode(KFile::File);
    page->resize(page->minimumSize());
    dial->resize(dial->minimumSize());

    if (dial->exec() == QDialog::Accepted)
    {
        changeMessage(i18n("Importing..."), 0, true);
        if (page->checkFile->isChecked())
        {
            QString impname = page->newFilename->url().simplified();
            if (!impname.isEmpty())
            {
                // import from file
                KgpgInterface *importKeyProcess = new KgpgInterface();
                connect(importKeyProcess, SIGNAL(importKeyFinished(QStringList)), keysList2, SLOT(slotReloadKeys(QStringList)));
                connect(importKeyProcess, SIGNAL(importKeyOrphaned()), keysList2, SLOT(slotReloadOrphaned()));
                importKeyProcess->importKey(KURL::fromPathOrURL(impname));
            }
        }
        else
        {
            QString keystr = kapp->clipboard()->text(clipboardMode);
            if (!keystr.isEmpty())
            {
                KgpgInterface *importKeyProcess = new KgpgInterface();
                connect(importKeyProcess,SIGNAL(importKeyFinished(QStringList)),keysList2,SLOT(slotReloadKeys(QStringList)));
                connect(importKeyProcess,SIGNAL(importKeyOrphaned()),keysList2,SLOT(slotReloadOrphaned()));
                importKeyProcess->importKey(keystr);
            }
        }
    }
    delete dial;
}

void listKeys::refreshkey()
{
    keysList2->refreshAll();
    listViewSearch->updateSearch(listViewSearch->text());
}

#include "listkeys.moc"

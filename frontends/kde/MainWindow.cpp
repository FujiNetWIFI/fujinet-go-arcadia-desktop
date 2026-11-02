/*
 * MainWindow -- see MainWindow.h.
 *
 * Plain Qt6 Widgets, deliberately not KDE Frameworks: it picks up Breeze
 * through the platform theme anyway, and staying framework-free keeps this
 * frontend usable outside a KDE session.
 *
 * Copyright (C) 2026 Thomas Cherryhomes
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "MainWindow.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QStatusBar>
#include <QUrl>
#include <QVBoxLayout>

#include "DisplayWidget.h"
#include "FujiNetWindows.h"
#include "KeyForward.h"
#include "SettingsDialog.h"
#include "SoftDisplayWidget.h"
#include "controllers/ControllersWindow.h"
#include "debugger/DebuggerWindow.h"

namespace {

const char *const kCartFilter =
    "Arcadia 2001 cartridges (*.bin *.rom *.arc *.zip *.BIN *.ROM *.ARC *.ZIP);;All files (*)";

QString local(const char *s) { return QString::fromUtf8(s ? s : ""); }

bool envOn(const char *name)
{
    const QByteArray v = qgetenv(name);
    return !v.isEmpty() && v[0] != '0';
}

/* A status line that elides instead of widening the window. */
class ElidedLabel : public QLabel {
public:
    QSize minimumSizeHint() const override { return QSize(40, QLabel::minimumSizeHint().height()); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        const QRect r = contentsRect();
        p.setPen(palette().color(foregroundRole()));
        p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, fontMetrics().elidedText(text(), Qt::ElideRight, r.width()));
    }
};

} // namespace

MainWindow::MainWindow(arcsession *session, QWidget *parent)
    : QMainWindow(parent), m_session(session)
{
    setWindowTitle(QStringLiteral("FujiNet Go Arcadia 2001"));
    /* 4:3 at 972 x 729, and the chrome */
    resize(972, 729 + 60);
    setAcceptDrops(true);

    auto *central = new QWidget;
    m_stack = new QVBoxLayout(central);
    m_stack->setContentsMargins(0, 0, 0, 0);
    m_stack->setSpacing(0);

    auto *gpu = new DisplayWidget(session, central);
    m_display = gpu;
    m_stack->addWidget(gpu, 1);
    setCentralWidget(central);
    /* No usable GPU device: fall back to painting in software */
    connect(gpu, &QRhiWidget::renderFailed, this, &MainWindow::useSoftwareDisplay, Qt::QueuedConnection);

    m_dot = new QLabel(QStringLiteral("●"));
    auto *status = new ElidedLabel;
    status->setText(QStringLiteral("Starting..."));
    status->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_status = status;
    statusBar()->addWidget(m_dot);
    statusBar()->addWidget(m_status, 1);

    m_toast = new QLabel(this);
    m_toast->setWordWrap(true);
    m_toast->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_toast->setAlignment(Qt::AlignCenter);
    m_toast->setStyleSheet(QStringLiteral(
        "background: rgba(30, 30, 30, 235); color: white; border-radius: 8px; padding: 8px 14px;"));
    m_toast->hide();
    m_toastTimer.setSingleShot(true);
    connect(&m_toastTimer, &QTimer::timeout, m_toast, &QWidget::hide);

    buildMenus();
    applyPicture();

    /* One timer for the news the other threads leave: the gamepad thread
     * cannot call into Qt, so it posts system actions and gamepad events and
     * this takes them. */
    m_padGen = arcsession_gamepad_generation(session);
    connect(&m_tick, &QTimer::timeout, this, &MainWindow::tick);
    m_tick.start(100);
    updateStatus();
}

/* ---- starting ------------------------------------------------------------- */

void MainWindow::start()
{
    if (arcsession_start(m_session, nullptr) != 0)
        QMessageBox::warning(this, QStringLiteral("Cannot Start the Console"),
                             local(arcsession_last_error(m_session)));
    updateStatus();
    syncMenus();
    openAtLaunch();
}

/* The env hooks that open the other windows at launch, once the event loop
 * runs: attaching the debugger before the machine exists would not stop it. */
void MainWindow::openAtLaunch()
{
    if (envOn("ARC_OPEN_CONTROLLERS"))
        QTimer::singleShot(0, this, [this] { toggleControllers(); });
    if (envOn("ARC_OPEN_DEBUGGER"))
        QTimer::singleShot(0, this, [this] { DebuggerWindow::showFor(this, m_session); });
    if (envOn("ARC_OPEN_SETTINGS"))
        QTimer::singleShot(0, this, &MainWindow::showSettings);
}

/* ---- menus ---------------------------------------------------------------- */

void MainWindow::buildMenus()
{
    QMenu *machine = menuBar()->addMenu(QStringLiteral("&Machine"));
    machine->addAction(QStringLiteral("&Open Cartridge..."), QKeySequence(Qt::CTRL | Qt::Key_O), this, [this] {
        const QString f = QFileDialog::getOpenFileName(this, QStringLiteral("Open Cartridge"), QString(),
                                                       QString::fromUtf8(kCartFilter));
        if (!f.isEmpty()) openCart(f);
    });
    machine->addAction(QStringLiteral("&Eject Cartridge"), this, [this] {
        if (arcsession_eject(m_session) != 0)
            QMessageBox::warning(this, QStringLiteral("Eject failed"), local(arcsession_last_error(m_session)));
        else
            toast(QStringLiteral("Cartridge ejected: back to the FujiNet CONFIG client"));
        updateStatus();
    });
    machine->addAction(QStringLiteral("&Import Cartridge to SD..."), this, [this] {
        const QString f = QFileDialog::getOpenFileName(this, QStringLiteral("Import Cartridge to SD"), QString(),
                                                       QString::fromUtf8(kCartFilter));
        if (f.isEmpty()) return;
        char dest[1024];
        if (arcsession_import_cart_to_sd(m_session, f.toLocal8Bit().constData(), dest, sizeof dest) != 0)
            QMessageBox::warning(this, QStringLiteral("Import failed"), local(arcsession_last_error(m_session)));
        else
            toast(QStringLiteral("%1 is on the SD host: boot it from the CONFIG client")
                      .arg(QFileInfo(local(dest)).fileName()));
    });
    machine->addSeparator();
    m_pauseAction = machine->addAction(QStringLiteral("&Pause"));
    m_pauseAction->setCheckable(true);
    connect(m_pauseAction, &QAction::triggered, this, [this](bool on) {
        arcsession_set_paused(m_session, on ? 1 : 0);
        updateStatus();
    });
    /* Reset acts through its key binding too (F1 by default, remappable); the
     * menu shows the bound key and does not claim it as a shortcut, so the
     * binding stays the one that acts. */
    m_resetAction = machine->addAction(QStringLiteral("&Reset"), this, [this] {
        arcsession_reset(m_session);
        toast(QStringLiteral("Console RESET"));
    });
    machine->addAction(QStringLiteral("Reset to &CONFIG"), QKeySequence(Qt::CTRL | Qt::Key_R), this, [this] {
        if (arcsession_reset_to_config(m_session) != 0)
            QMessageBox::warning(this, QStringLiteral("Reset failed"), local(arcsession_last_error(m_session)));
        else
            toast(QStringLiteral("Back to the FujiNet CONFIG client"));
        updateStatus();
    });
    connect(machine, &QMenu::aboutToShow, this, &MainWindow::syncMenus);
    machine->addSeparator();
    machine->addAction(QStringLiteral("P&references..."), QKeySequence(Qt::CTRL | Qt::Key_Comma), this,
                       &MainWindow::showSettings);
    machine->addSeparator();
    machine->addAction(QStringLiteral("&Quit"), QKeySequence::Quit, this, [this] { close(); });

    QMenu *view = menuBar()->addMenu(QStringLiteral("&View"));
    view->addAction(QStringLiteral("&Controllers"), QKeySequence(Qt::Key_F9), this, &MainWindow::toggleControllers);
    view->addAction(QStringLiteral("&Debugger"), QKeySequence(Qt::Key_F12), this,
                    [this] { DebuggerWindow::toggleFor(this, m_session); });
    view->addSeparator();
    m_tvAction = view->addAction(QStringLiteral("&TV Aspect"));
    m_tvAction->setCheckable(true);
    m_tvAction->setToolTip(QStringLiteral("Show the picture across a 4:3 television, as the console does; "
                                          "off shows square pixels"));
    connect(m_tvAction, &QAction::triggered, this, [this](bool on) {
        /* the family's "aspect": 0 the TV's 4:3, 1 square pixels */
        arcsession_set_int(m_session, "aspect", on ? 0 : 1);
        applyPicture();
    });
    m_smoothAction = view->addAction(QStringLiteral("&Smooth Scaling"));
    m_smoothAction->setCheckable(true);
    connect(m_smoothAction, &QAction::triggered, this, [this](bool on) {
        arcsession_set_int(m_session, "smooth", on ? 1 : 0);
        applyPicture();
    });
    view->addAction(QStringLiteral("&Fullscreen"), QKeySequence(Qt::Key_F11), this, [this] {
        if (isFullScreen()) showNormal(); else showFullScreen();
    });

    QMenu *fuji = menuBar()->addMenu(QStringLiteral("&FujiNet"));
    fuji->addAction(QStringLiteral("&Web UI"), this, [this] {
        if (arcsession_fujinet_external(m_session)) {
            toast(QStringLiteral("FujiNet is external: use its own web UI"));
            return;
        }
        if (!arcsession_fujinet_running(m_session)) {
            toast(QStringLiteral("FujiNet is off (Preferences > FujiNet)"));
            return;
        }
        fujinet_config_show(this, m_session);
    });
    fuji->addAction(QStringLiteral("Console &Log"), this, [this] {
        if (arcsession_fujinet_external(m_session)) {
            toast(QStringLiteral("FujiNet is external: see its own console"));
            return;
        }
        fujinet_log_show(this, m_session);
    });

    QMenu *help = menuBar()->addMenu(QStringLiteral("&Help"));
    help->addAction(QStringLiteral("&About FujiNet Go Arcadia 2001"), this, &MainWindow::showAbout);
    syncMenus();
}

/* The bound key of the console's RESET as the menu's shortcut text (it
 * follows a rebinding), and the state of the checkable items. */
void MainWindow::syncMenus()
{
    const arc_binding b = arcsession_binding_get(m_session, ARC_TARGET_SWITCH(ARC_SW_RESET));
    char key[48] = "";
    if (b.keysym) arcsession_keysym_name(b.keysym, key, sizeof key);
    const QString text = QStringLiteral("&Reset");
    m_resetAction->setText(key[0] ? text + QLatin1Char('\t') + local(key) : text);
    m_pauseAction->setChecked(arcsession_is_paused(m_session) != 0);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, QStringLiteral("About FujiNet Go Arcadia 2001"),
        QStringLiteral("<b>FujiNet Go Arcadia 2001</b> %1<br><br>"
                       "The Emerson Arcadia 2001 (and its Bandai and Hanimex-style clones), with a "
                       "built-in FujiNet.<br><br>"
                       "The emulator is MAME's Arcadia 2001 -- the Signetics 2650 CPU and the 2637 "
                       "UVI video and sound chip (Peter Trauner, Wilbert Pol, Couriersud, Miodrag "
                       "Milanovic, Juergen Buchmueller and the MAME team) -- transposed to C. The FujiNet firmware "
                       "and the cartridge are by Thomas Cherryhomes and the FujiNet "
                       "contributors (GPL-3.0-or-later).<br><br>"
                       "Copyright &copy; 2026 Thomas Cherryhomes -- GPL-3.0-or-later<br>"
                       "<a href=\"https://fujinet.online/\">fujinet.online</a> &middot; "
                       "<a href=\"https://github.com/FujiNetWIFI/fujinet-go-arcadia-desktop/issues\">"
                       "Report an issue</a>")
            .arg(QStringLiteral(ARC_VERSION_STRING)));
}

/* ---- picture, settings ----------------------------------------------------- */

void MainWindow::showSettings()
{
    const bool restart = SettingsDialog::run(this, m_session);
    applyPicture();
    syncMenus();
    if (restart) restartSession();
}

void MainWindow::useSoftwareDisplay()
{
    if (dynamic_cast<SoftDisplayWidget *>(m_display)) return;
    auto *old = m_display->widget();
    auto *soft = new SoftDisplayWidget(m_session, centralWidget());
    m_display = soft;
    m_stack->replaceWidget(old, soft);
    delete old;   /* the GPU display, which stops its feed */
    applyPicture();
}

void MainWindow::applyPicture()
{
    const bool tv = arcsession_get_int(m_session, "aspect", 0) == 0;
    const bool smooth = arcsession_get_int(m_session, "smooth", 0) != 0;
    m_display->setTvAspect(tv);
    m_display->setSmooth(smooth);
    if (m_tvAction) m_tvAction->setChecked(tv);
    if (m_smoothAction) m_smoothAction->setChecked(smooth);
}

/* A machine option changed: a power cycle with the new settings (a full
 * restart when FujiNet, audio or the gamepads were switched). */
void MainWindow::restartSession()
{
    if (arcsession_restart(m_session) != 0) {
        QMessageBox::warning(this, QStringLiteral("Restart failed"), local(arcsession_last_error(m_session)));
    } else {
        toast(QStringLiteral("Machine options applied (power cycled)"));
    }
    arcControllersRefresh();
    updateStatus();
    syncMenus();
}

void MainWindow::toggleControllers()
{
    arcControllersToggle(this, m_session);
}

/* ---- the timer ---------------------------------------------------------------- */

void MainWindow::tick()
{
    drainSysactions();
    if (++m_ticks % 3 == 0) pollGamepads();
    if (m_ticks % 10 == 0) updateStatus();
}

void MainWindow::drainSysactions()
{
    int sa;
    while (arcsession_sysaction_take(m_session, &sa)) runSysaction(sa);
}

void MainWindow::runSysaction(int sa)
{
    switch (sa) {
    case ARC_SYSACT_RESET_CONFIG:
        arcsession_sysaction(m_session, sa);
        toast(QStringLiteral("Back to the FujiNet CONFIG client"));
        updateStatus();
        break;
    case ARC_SYSACT_DEBUG_STOP:
        DebuggerWindow::showFor(this, m_session);
        break;
    default: break;
    }
}

void MainWindow::pollGamepads()
{
    const unsigned gen = arcsession_gamepad_generation(m_session);
    if (gen == m_padGen) return;
    m_padGen = gen;
    char msg[160];
    if (arcsession_gamepad_last_event(m_session, msg, sizeof msg) > 0) toast(local(msg));
}

void MainWindow::updateStatus()
{
    QString text;
    bool on = false;
    const QString cart = local(arcsession_cart_path(m_session));
    if (!arcsession_is_running(m_session)) {
        text = QStringLiteral("Stopped");
    } else {
        char st[160];
        arcsession_cart_status(m_session, st, sizeof st);
        on = arcsession_cart_link_up(m_session) == 1;
        text = QStringLiteral("FujiNet: %1").arg(local(st));
        if (!cart.isEmpty())
            text = QStringLiteral("%1 — %2").arg(QFileInfo(cart).fileName(), text);
        else if (!arcsession_cart_booted_game(m_session))
            text = QStringLiteral("CONFIG — %1").arg(text);
        if (arcsession_is_paused(m_session)) text = QStringLiteral("Paused — %1").arg(text);
    }
    m_status->setText(text);
    m_status->setToolTip(text);
    m_dot->setStyleSheet(on ? QStringLiteral("color: %1;").arg(arcAccentTextColor(this).name())
                            : QStringLiteral("color: gray;"));
}

/* ---- toasts ---------------------------------------------------------------- */

void MainWindow::toast(const QString &text)
{
    m_toast->setText(text);
    placeToast();
    m_toast->show();
    m_toast->raise();
    m_toastTimer.start(text.size() > 80 ? 6000 : 3500);
}

void MainWindow::placeToast()
{
    const QRect c = centralWidget()->geometry();
    /* one line if it fits, wrapped within the picture otherwise */
    const QMargins pad = m_toast->contentsMargins();
    const int w = qMin(m_toast->fontMetrics().horizontalAdvance(m_toast->text()) + pad.left() + pad.right() + 40,
                       qMin(600, c.width() - 32));
    m_toast->setFixedSize(w, m_toast->heightForWidth(w));
    m_toast->move(c.x() + (c.width() - m_toast->width()) / 2, c.bottom() - m_toast->height() - 16);
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    if (m_toast->isVisible()) placeToast();
}

/* ---- cartridges -------------------------------------------------------- */

void MainWindow::openCart(const QString &path)
{
    if (arcsession_load_cart(m_session, path.toLocal8Bit().constData()) != 0) {
        QMessageBox::warning(this, QStringLiteral("Could not open"), local(arcsession_last_error(m_session)));
        return;
    }
    toast(QStringLiteral("Running %1").arg(QFileInfo(path).fileName()));
    updateStatus();
}

void MainWindow::loadMedia(const QString &path)
{
    const QByteArray p = path.toLocal8Bit();
    if (arcsession_media_is_cartridge(p.constData())) {
        openCart(path);
        return;
    }
    char dest[1024];
    if (arcsession_import_media(m_session, p.constData(), dest, sizeof dest) != 0) {
        QMessageBox::warning(this, QStringLiteral("Import failed"), local(arcsession_last_error(m_session)));
        return;
    }
    toast(QStringLiteral("Copied to FujiNet's SD folder: mount it from the CONFIG client"));
}

void MainWindow::activateFor(const QString &path)
{
    setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
    if (!path.isEmpty()) loadMedia(path);
}

/* ---- keys, drops, focus ------------------------------------------------------------- */

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->isAutoRepeat()) return;
    if (e->key() == Qt::Key_F9) { toggleControllers(); return; }
    if (e->key() == Qt::Key_F12) { DebuggerWindow::toggleFor(this, m_session); return; }
    if (arcIsAppKey(e)) { QMainWindow::keyPressEvent(e); return; }

    const uint32_t ks = arcKeysymFromQt(e);
    if (!ks) { QMainWindow::keyPressEvent(e); return; }

    const int sa = arcsession_key_sysaction(m_session, ks);
    if (sa >= 0) {
        if (!m_sysactDown[sa]) { m_sysactDown[sa] = true; runSysaction(sa); }
        return;
    }
    if (!arcsession_key(m_session, ks, 1)) QMainWindow::keyPressEvent(e);
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    if (e->isAutoRepeat()) return;
    const uint32_t ks = arcKeysymFromQt(e);
    if (!ks) { QMainWindow::keyReleaseEvent(e); return; }
    const int sa = arcsession_key_sysaction(m_session, ks);
    if (sa >= 0) { m_sysactDown[sa] = false; return; }
    if (!arcsession_key(m_session, ks, 0)) QMainWindow::keyReleaseEvent(e);
}

bool MainWindow::event(QEvent *e)
{
    if (e->type() == QEvent::WindowDeactivate) {
        arcsession_release_all(m_session);
        for (bool &d : m_sysactDown) d = false;
    }
    return QMainWindow::event(e);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *e)
{
    const QList<QUrl> urls = e->mimeData()->urls();
    if (urls.isEmpty()) return;
    const QString path = urls.first().toLocalFile();
    if (!path.isEmpty()) loadMedia(path);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    m_tick.stop();
    m_toastTimer.stop();
    QMainWindow::closeEvent(e);
}

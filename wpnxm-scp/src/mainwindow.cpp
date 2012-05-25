#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tray.h"

#include <QMessageBox>
#include <QSharedMemory>
#include <QtGui>
#include <QRegExp>
#include <QDesktopServices>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // disable Maximize functionality
    setWindowFlags( (windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMaximizeButtonHint);
    //setFixedWidth(600);
    //setFixedHeight(270);

    // overrides the window title defined in mainwindow.ui
    setWindowTitle(APP_NAME_AND_VERSION);

    createActions();

    createTrayIcon();

    // fetch version numbers from the daemons and set label text accordingly
    ui->label_Nginx_Version->setText( getNginxVersion() );
    ui->label_PHP_Version->setText( getPHPVersion() );
    ui->label_MariaDb_Version->setText( getMariaVersion() );

    // the initial state of daemon status icons is disabled
    ui->label_Nginx_Status->setEnabled(false);
    ui->label_PHP_Status->setEnabled(false);
    ui->label_MariaDB_Status->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setLabelStatusActive(QString label, bool enabled)
{
    if(label == "nginx")
    {
        ui->label_Nginx_Status->setEnabled(enabled);
    }

    if(label == "php")
    {
        ui->label_PHP_Status->setEnabled(enabled);
    }

    if(label == "nginx")
    {
        ui->label_MariaDB_Status->setEnabled(enabled);
    }
}

void MainWindow::createTrayIcon()
{
    // The tray icon is an instance of the QSystemTrayIcon class.
    // To check whether a system tray is present on the user's desktop,
    // we call the static QSystemTrayIcon::isSystemTrayAvailable() function.
    if (false == QSystemTrayIcon::isSystemTrayAvailable())
    {
        QMessageBox::critical(0, APP_NAME, tr("You don't have a system tray."));
        //return 1;
    }
    else
    {
        // instantiate and attach the tray icon to the system tray
        trayIcon = new Tray(qApp);
        trayIcon->show();

        // handle clicks on the icon
        connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

        // if process state of a daemon changes, then change the label status in UI::MainWindow too
        connect(trayIcon, SIGNAL(signalSetLabelStatusActive(QString, bool)),
                this, SLOT(setLabelStatusActive(QString, bool)));
    }
}

void MainWindow::createActions()
 {
     minimizeAction = new QAction(tr("Mi&nimize"), this);
     connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

     restoreAction = new QAction(tr("&Restore"), this);
     connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

     quitAction = new QAction(tr("&Quit"), this);
     // qApp is global pointer to QApplication
     connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

     // PushButtons:: Website, ReportBug, Donate
     connect(ui->pushButton_Website, SIGNAL(clicked()), this, SLOT(goToWebsite()));
     connect(ui->pushButton_GoogleGroup, SIGNAL(clicked()), this, SLOT(goToGoogleGroup()));
     connect(ui->pushButton_ReportBug, SIGNAL(clicked()), this, SLOT(goToReportIssue()));
     connect(ui->pushButton_Donate, SIGNAL(clicked()), this, SLOT(goToDonate()));

     // PushButtons: Configuration, Help, About, Close
     connect(ui->pushButton_Configuration, SIGNAL(clicked()), this, SLOT(openConfigurationDialog()));
     connect(ui->pushButton_Help, SIGNAL(clicked()), this, SLOT(openHelpDialog()));
     connect(ui->pushButton_About, SIGNAL(clicked()), this, SLOT(openAboutDialog()));
     connect(ui->pushButton_Close, SIGNAL(clicked()), qApp, SLOT(quit()));

     // Actions - Start
     //connect(ui->pushButton_StartNginx, SIGNAL(clicked()), trayIcon, SLOT(runNginx()));
     //connect(ui->pushButton_StartPHP, SIGNAL(clicked()), trayIcon, SLOT(runPhp()));
     //connect(ui->pushButton_StartMariaDb, SIGNAL(clicked()), trayIcon, SLOT(runMySQL()));

     // Actions - Stop
    // connect(ui->pushButton_StopNginx, SIGNAL(clicked()), trayIcon, SLOT(stopNginx()));
    // connect(ui->pushButton_StopPHP, SIGNAL(clicked()), trayIcon, SLOT(stopPhp()));
     //connect(ui->pushButton_StopMariaDb, SIGNAL(clicked()), trayIcon, SLOT(stopMySQL()));
 }

void MainWindow::changeEvent(QEvent *event)
{
    switch (event->type())
    {
        //case QEvent::LanguageChange:
        //    this->ui->retranslateUi(this);
        //    break;
        case QEvent::WindowStateChange:
            {
                // minimize to tray (do not minimize to taskbar)
                if (this->windowState() & Qt::WindowMinimized)
                {
                    // @todo provide configuration options to let the user decide on this
                    //if (Preferences::instance().minimizeToTray())
                    //{
                        QTimer::singleShot(0, this, SLOT(hide()));
                    //}
                }

                break;
            }
        default:
            break;
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible()) {
        QMessageBox::information(this, APP_NAME,
                                 tr("The program will keep running in the system tray.<br>"
                                    "To terminate the program, choose <b>Quit</b> in the context menu of the system tray."));
        hide();

        // do not propagate the event to the base class
        event->ignore();
    }
}

void MainWindow::setVisible(bool visible)
 {
     minimizeAction->setEnabled(visible);
     restoreAction->setEnabled(isMaximized() || !visible);
     QMainWindow::setVisible(visible);
 }

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
        //case QSystemTrayIcon::MiddleClick:

            // clicking the tray icon, when the main window is hidden, shows the main window
            if(!isVisible()) {
                setVisible(true);
                setWindowState( windowState() & ( ~Qt::WindowMinimized | Qt::WindowActive ) );
            }
            else {
                // clicking the tray icon, when the main window is shown, hides it
                setVisible(false);
            }

            break;
        default:;
    }
}

QString MainWindow::getNginxVersion()
{
    QProcess* processNginx;

    processNginx = new QProcess(this);
    //processNginx->setWorkingDirectory(cfgNginxDir);
    //processNginx->start("./nginx", QStringList() << "-v");
    processNginx->waitForFinished(-1);

    //QString p_stdout = processNginx->readAllStandardOutput();
    QString p_stderr = processNginx->readAllStandardError();

    // test
    QString p_stdout = "nginx version: nginx/1.1.11";

    qDebug() << p_stdout;
    qDebug() << p_stderr;

    return parseVersionNumber(p_stdout);
}

QString MainWindow::getMariaVersion()
{
    QProcess* processMaria;

    processMaria = new QProcess(this);
    //processMaria->setWorkingDirectory(cfgMariaDir);
    processMaria->start("./mysqld", QStringList() << "-V"); // upper-case V
    processMaria->waitForFinished(-1);

    //QString p_stdout = processMaria->readAllStandardOutput();
    QString p_stderr = processMaria->readAllStandardError();

    // test
    QString p_stdout = "mysql  Ver 15.1 Distrib 5.5.23-MariaDB, for Win32 (x86)";

    qDebug() << p_stdout;
    qDebug() << p_stderr;

    return parseVersionNumber(p_stdout.mid(15));
}

QString MainWindow::getPHPVersion()
{
    QProcess* processPhp;

    processPhp = new QProcess(this);
    //processPhp->setWorkingDirectory(cfgPHPDir);
    //processPhp->start(cfgPHPDir+cfgPHPExec, QStringList() << "-v");
    processPhp->waitForFinished(-1);

    //QString p_stdout = processPhp->readAllStandardOutput();
    QString p_stderr = processPhp->readAllStandardError();

    // test
    QString p_stdout = "PHP 5.4.0 (cli) (built: Feb 29 2012 19:06:50)";

    qDebug() << p_stdout;
    qDebug() << p_stderr;

    return parseVersionNumber(p_stdout);
}

QString MainWindow::parseVersionNumber(QString stringWithVersion)
{
    qDebug() << stringWithVersion;

    // The RegExp for matching version numbers is (\d+\.)?(\d+\.)?(\d+\.)?(\*|\d+)
    // The following one is escaped:
    QRegExp regex("(\\d+\\.)?(\\d+\\.)?(\\d+\\.)?(\\*|\\d+)");

    // match
    regex.indexIn(stringWithVersion);

    qDebug() << regex.cap(0);
    QString cap = regex.cap(0);
    return cap;

// Leave this for debugging reasons
//    int pos = 0;
//    while((pos = regex.indexIn(stringWithVersion, pos)) != -1)
//    {
//        qDebug() << "Match at pos " << pos
//                 << " with length " << regex.matchedLength()
//                 << ", captured = " << regex.capturedTexts().at(0).toLatin1().data()
//                 << ".\n";
//        pos += regex.matchedLength();
//    }
}

void MainWindow::goToWebsite()
{
    QDesktopServices::openUrl(QUrl("http://wpn-xm.org/"));
}

void MainWindow::goToGoogleGroup()
{
    QDesktopServices::openUrl(QUrl("http://groups.google.com/group/wpn-xm/"));
}

void MainWindow::goToReportIssue()
{
    QDesktopServices::openUrl(QUrl("https://github.com/jakoch/WPN-XM/issues/"));
}

void MainWindow::goToDonate()
{
    QDesktopServices::openUrl(QUrl("http://wpn-xm.org/#donate"));
}

void MainWindow::openHelpDialog()
{

}

void MainWindow::openConfigurationDialog()
{

}

void MainWindow::openAboutDialog()
{
    QMessageBox::about(this, tr("About WPN-XM"),
        tr("<b>"  APP_NAME_AND_VERSION "</b><br>"
        "<table>"
        "<tr><td><b>Website</b></td><td><a href=\"http://wpn-xm.org/\">http://wpn-xm.org/</a></td></tr>"
        "<tr><td><b>License</b></td><td>GNU/GPL version 3, or any later version.</td></tr>"
        "<tr><td><b>Author(s)</b></td><td>Yann Le Moigne (c) 2010,</td></tr>"
        "<tr><td>&nbsp;</td><td>Jens-Andr� Koch (c) 2011 - onwards.</td></tr>"
        "<tr><td><b>Github</b></td><td><a href=\"https://github.com/jakoch/WPN-XM/\">https://github.com/jakoch/WPN-XM/</a>"
        "</td></tr></table>"));
}



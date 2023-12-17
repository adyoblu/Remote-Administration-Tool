#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    NetworkManager& TCPSocket = NetworkManager::getInstance();
    bool connected=TCPSocket.connectToServer();
    if (connected) {
        qDebug() << "Connected to server!";
    } else {
        qDebug() << "Failed to connect to server!";
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

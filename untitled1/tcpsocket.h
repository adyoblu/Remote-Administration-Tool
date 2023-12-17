#include <QObject>
#include <QTcpSocket>
#include <QDebug>
#include <QString>
class NetworkManager : public QObject
{
    Q_OBJECT
public:
    static NetworkManager& getInstance();
    //{
    //static NetworkManager instance;
    // return instance;
    // }
    static void destroyInstance();
    bool connectToServer()
    {
        m_socket = new QTcpSocket(this);
        m_socket->connectToHost("192.168.137.165", 5566);

        if (m_socket->waitForConnected()) {
            qDebug() << "Connected to server!";
            return true;
        } else {
            qDebug() << "Failed to connect to server!";
            return false;
        }
    }

    void sendToServer(const QString& data)
    {
        m_socket->write(data.toStdString().c_str());
        m_socket->flush();
    }

    QByteArray receiveFromServer()
    {
        QByteArray data;
        if (m_socket->waitForReadyRead()) {
            data = m_socket->readAll();
        }
        return data;
    }

private:
    static NetworkManager* instance;
    NetworkManager() {}
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    QTcpSocket* m_socket;
};

/*
 * Garuda plugin - Enables communication with other Garuda enabled apps.
 *
 * Copyright (C) 2011-2012  Monash University
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 * 
 * Author: Michael Wybrow <mjwybrow@users.sourceforge.net>
*/

#include <cassert>

#include <QMessageBox>

#include "appgarudaclient.h"
#include "qt-json/json.h"


AppGarudaClient::AppGarudaClient(QObject *parent) :
    QObject(parent),
    m_tcp_socket(NULL)
{
    openSocket();

    activateDunnartWithCore();
    registerDunnart();
}

void AppGarudaClient::openSocket(void)
{
    if (m_tcp_socket == NULL)
    {
        m_tcp_socket = new QTcpSocket();

        connect(m_tcp_socket, SIGNAL(readyRead()), this, SLOT(readData()));

        connect(m_tcp_socket, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(displayError(QAbstractSocket::SocketError)));

        m_tcp_socket->connectToHost("localhost", 9000);

        const int timeout = 5 * 1000; // 5 seconds.
        if (!m_tcp_socket->waitForConnected(timeout))
        {
            delete m_tcp_socket;
            m_tcp_socket = NULL;
            return;
        }
    }
}

void AppGarudaClient::closeSocket(void)
{
    m_tcp_socket->disconnectFromHost();
    m_tcp_socket->waitForDisconnected();
    delete m_tcp_socket;
    m_tcp_socket = NULL;
}

bool AppGarudaClient::connectedToServer(void) const
{
    return (m_tcp_socket &&
            (m_tcp_socket->state() == QAbstractSocket::ConnectedState));
}

void AppGarudaClient::sendData(const QString& string)
{
    if (!connectedToServer())
    {
        return;
    }

    qDebug() << "GarudaClient Send: " << string;

    QByteArray block = string.toUtf8().data();
    block.append("\n");
    int written = m_tcp_socket->write(block);
    Q_UNUSED (written)
    assert (written == block.size());
    m_tcp_socket->flush();
}

void AppGarudaClient::readData(void)
{
    assert(m_tcp_socket);
    QByteArray in;

    while (m_tcp_socket->bytesAvailable() > 0)
    {
        in += m_tcp_socket->read(1000);
        //qDebug() << "Read Data: " << in;
    }

    int sentinelPos = -1;
    while ((sentinelPos = in.indexOf('\n')) != -1)
    {
        QByteArray message = in.left(sentinelPos);
        in.remove(0, sentinelPos + 1);
        qDebug() << "GarudaClient Receive: " << message;
        parseMessage(message);
        emit newMessage(message);
    }
}

void AppGarudaClient::parseMessage(const QString &message)
{
    QVariant var = QtJson::Json::parse(message);
    QVariantMap data = var.toMap();

    QVariantMap header = data["header"].toMap();
    QString action =  header["id"].toString();

    if (action == "GetLoadableSoftwaresResponse")
    {
        emit showCompatibleSoftwareResponse(data);
    }
    else if (action == "LoadFileRequest")
    {
        QVariantMap body = data["body"].toMap();
        QString filePath = body["filePath"].toString();
        emit fileOpenRequest(filePath);
    }
}

void AppGarudaClient::activateDunnartWithCore(void)
{
    QVariantMap header;
    header["id"] = "ActivateSoftwareRequest";
    header["version"] = "0.1";

    QVariantMap software;
    software["id"] = "Dunnart";
    software["name"] = "";
    software["version"] = "2.0";

    QVariantMap body;
    body["software"] = software;
    body["softwareId"] = "Dunnart";
    body["version"] = "2.0";

    QVariantMap root;
    root["header"] = header;
    root["body"] = body;

    QByteArray data = QtJson::Json::serialize(root);

    sendData(QString(data));
}

void AppGarudaClient::registerDunnart(void)
{
    QVariantMap header;
    header["id"] = "RegisterSoftwareRequest";
    header["version"] = "0.1";

    QVariantMap layoutFormat;
    layoutFormat["fileExtension"] = "xml";
    layoutFormat["fileFormat"] = "TestSoftwareA";

    QVariantList fileFormats;
    fileFormats.append(layoutFormat);

    QVariantMap environments;
    environments["HOME"] = "home";

    QVariantMap config;
    config["id"] = "Dunnart";
    config["name"] = "Dunnart";
    config["version"] = "2.0";
    config["workingDirectory"] = QDir::currentPath();
    config["filePath"] = QCoreApplication::applicationFilePath();
    config["inputFileFormats"] = fileFormats;
    config["outputFileFormats"] = fileFormats;
    config["environments"] = environments;

    QVariantMap body;
    body["softwareId"] = "Dunnart";
    body["name"] = "Dunnart";
    body["version"] = "2.0";
    body["workingDir"] = QDir::currentPath();
    body["filePath"] = QCoreApplication::applicationFilePath();
    body["inputFileFormats"] = fileFormats;
    body["outputFileFormats"] = fileFormats;
    body["environments"] = environments;

    QVariantMap root;
    root["header"] = header;
    root["softwareConfig"] = config;
    root["body"] = body;

    QByteArray data = QtJson::Json::serialize(root);

    sendData(QString(data));
}

void AppGarudaClient::deregisterWithCore(void)
{
    QVariantMap header;
    header["id"] = "DeregisterSoftwareRequest";
    header["version"] = "0.1";

    QVariantMap software;
    software["id"] = "Dunnart";
    software["name"] = "";
    software["version"] = "2.0";

    QVariantMap body;
    body["software"] = software;
    body["softwareId"] = "Dunnart";
    body["version"] = "2.0";

    QVariantMap root;
    root["header"] = header;
    root["body"] = body;

    QByteArray data = QtJson::Json::serialize(root);

    sendData(QString(data));
}

void AppGarudaClient::loadFileIntoSoftware(QFileInfo fileInfo, QString softwareId,
        QString softwareVersion)
{
    QVariantMap header;
    header["id"] = "LoadFileOntoSoftwareRequest";
    header["version"] = "0.1";

    QVariantMap software;
    software["id"] = softwareId;
    software["name"] = "";
    software["version"] = softwareVersion;

    QVariantMap body;
    body["filePath"] = fileInfo.absoluteFilePath();
    body["softwareId"] = softwareId;
    body["version"] = softwareVersion;

    QVariantMap root;
    root["header"] = header;
    root["body"] = body;
    root["software"] = software;

    QByteArray data = QtJson::Json::serialize(root);

    sendData(QString(data));
}

void AppGarudaClient::showCompatibleSoftwareFor(QString extension, QString format)
{
    QVariantMap header;
    header["id"] = "GetLoadableSoftwaresRequest";
    header["version"] = "0.1";

    QVariantMap software;
    software["id"] = "Dunnart";
    software["name"] = "";
    software["version"] = "2.0";

    QVariantMap body;
    body["software"] = software;
    body["softwareId"] = "Dunnart";
    body["version"] = "2.0";

    // XXX Not in the Garuda alpha 2 for some reason.
    //body["fileExtension"] = extension;
    //body["fileFormat"] = format;
    Q_UNUSED (extension)
    Q_UNUSED (format)

    QVariantMap root;
    root["header"] = header;
    root["body"] = body;

    QByteArray data = QtJson::Json::serialize(root);

    sendData(QString(data));
}


void AppGarudaClient::displayError(QAbstractSocket::SocketError socketError)
{
    QWidget *widget = NULL;
    switch (socketError)
    {
        case QAbstractSocket::HostNotFoundError:
            QMessageBox::information(widget, tr("Garuda Plugin: Network Error"),
                                 tr("The host was not found. Please check the "
                                    "host name and port settings."));
            break;
        case QAbstractSocket::SocketTimeoutError:
        case QAbstractSocket::ConnectionRefusedError:
            return;
            QMessageBox::information(widget, tr("Garuda Plugin: Network Error"),
                                 tr("Cannot connect to the Garuda core "
                                    "server.  Please check your network "
                                    "connection and try again later."));
            break;
        default:
            QMessageBox::information(widget, tr("Garuda Plugin: Network Error"),
                                     tr("The following error occurred: %1.")
                                     .arg(m_tcp_socket->errorString()));
    }
}

// vim: filetype=cpp ts=4 sw=4 et tw=0 wm=0 cindent

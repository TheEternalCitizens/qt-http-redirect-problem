#include "window.h"

#include <QApplication>
#include <QPushButton>
#include <QtNetwork>

Window::Window(QWidget *parent) : QWidget(parent)
{
    setFixedSize(1000, 500);

    m_button = new QPushButton("Press to send request", this);
    m_button->setGeometry(10, 10, 980, 480);
    m_button->setCheckable(true);

    m_networkAccessManager = new QNetworkAccessManager(this);

    connect(m_button, SIGNAL (clicked(bool)), this, SLOT (slotButtonClicked(bool)));
    connect(m_networkAccessManager, &QNetworkAccessManager::finished, this, &Window::slotHttpRequestFinished);
}

void Window::slotButtonClicked(bool checked)
{
    if (checked) {
        m_button->setText("Sending request");
        QString url = "http://localhost:4567/login";
        QString data = "hello=world";
        QNetworkRequest networkRequest = QNetworkRequest(url);
        QNetworkReply *networkReply = m_networkAccessManager->post(networkRequest, data.toUtf8());
    } else {
        m_button->setText("Press to send request");
    }
}

void Window::slotHttpRequestFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        m_button->setText("ERROR\n" + reply->errorString());
    } else {
        m_button->setText("SUCCESS\n" + reply->readAll());
    }
}

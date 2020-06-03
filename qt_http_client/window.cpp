#include "window.h"

#include <QApplication>
#include <QPushButton>
#include <QtNetwork>

static const std::map<QString, QNetworkRequest::RedirectPolicy> policies = {
    { "Manual Redirect Policy", QNetworkRequest::ManualRedirectPolicy },
    { "No Less Safe Redirect Policy", QNetworkRequest::NoLessSafeRedirectPolicy },
    { "Same Origin Redirect Policy", QNetworkRequest::SameOriginRedirectPolicy },
    { "User Verified Redirect Policy", QNetworkRequest::UserVerifiedRedirectPolicy },
};

Window::Window(QWidget *parent) : QWidget(parent)
{
    setFixedSize(1000, 500);

    m_button = new QPushButton("Press to send request", this);
    m_button->setGeometry(10, 10, 980, 440);
    m_button->setCheckable(true);

    m_comboBox = new QComboBox(this);
    m_comboBox->setGeometry(10, 460, 980, 30);
    for (std::pair<QString, int> element : policies)
    {
        m_comboBox->addItem(element.first);
    }

    m_networkAccessManager = new QNetworkAccessManager(this);
    m_redirectPolicy = QNetworkRequest::ManualRedirectPolicy;

    connect(m_button, SIGNAL (clicked(bool)), this, SLOT (slotButtonClicked(bool)));
    connect(m_comboBox, &QComboBox::currentTextChanged, this, &Window::slotCurrentTextChanged);
    connect(m_networkAccessManager, &QNetworkAccessManager::finished, this, &Window::slotHttpRequestFinished);


    // Debug all signals to network access manager
    connect(m_networkAccessManager, &QNetworkAccessManager::authenticationRequired, this,
    [=](QNetworkReply *reply, QAuthenticator *authenticator){
        qDebug() << "networkAccessManager authenticationRequired";
    });
    connect(m_networkAccessManager, &QNetworkAccessManager::encrypted, this,
    [=](QNetworkReply *reply){
        qDebug() << "networkAccessManager encrypted";
    });
    connect(m_networkAccessManager, &QNetworkAccessManager::finished, this,
    [=](QNetworkReply *reply){
        qDebug() << "networkAccessManager finished";
    });
    connect(m_networkAccessManager, &QNetworkAccessManager::networkAccessibleChanged, this,
    [=](QNetworkAccessManager::NetworkAccessibility accessible){
        qDebug() << "networkAccessManager networkAccessibleChanged";
    });
    connect(m_networkAccessManager, &QNetworkAccessManager::preSharedKeyAuthenticationRequired, this,
    [=](QNetworkReply *reply, QSslPreSharedKeyAuthenticator *authenticator){
        qDebug() << "networkAccessManager preSharedKeyAuthenticationRequired";
    });
    connect(m_networkAccessManager, &QNetworkAccessManager::proxyAuthenticationRequired, this,
    [=](const QNetworkProxy &proxy, QAuthenticator *authenticator){
        qDebug() << "networkAccessManager proxyAuthenticationRequired";
    });
    connect(m_networkAccessManager, &QNetworkAccessManager::sslErrors, this,
    [=](QNetworkReply *reply, const QList<QSslError> &errors){
        qDebug() << "networkAccessManager sslErrors";
    });
    connect(m_networkAccessManager, &QObject::destroyed, [] {
        qDebug() << "networkAccessManager got deleted!";
    });
    connect(this, &QObject::destroyed, [] {
        qDebug() << "Window got deleted!";
    });
}

void Window::slotButtonClicked(bool checked)
{
    if (!checked) {
        m_button->setText("Press to send request");
        return;
    }

    m_button->setText("Sending request");
    QString url = "http://localhost:4567/login";
    QString data = "hello=world";
    m_networkRequest = QNetworkRequest(url);
    m_networkRequest.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute,
        m_redirectPolicy
    );
    m_networkReply = m_networkAccessManager->post(m_networkRequest, data.toUtf8());


    // Debug all signals to network reply
    connect(m_networkReply, &QNetworkReply::downloadProgress, this,
    [=](qint64 bytesReceived, qint64 bytesTotal){
        qDebug() << "networkReply downloadProgress";
    });
    connect(m_networkReply, &QNetworkReply::encrypted, this,
    [=](){
        qDebug() << "networkReply encrypted";
    });
    connect(m_networkReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this,
    [=](QNetworkReply::NetworkError code){
        qDebug() << "networkReply errorOccurred";
    });
    connect(m_networkReply, &QNetworkReply::finished, this,
    [=](){
        qDebug() << "networkReply finished";
    });
    connect(m_networkReply, &QNetworkReply::metaDataChanged, this,
    [=](){
        qDebug() << "networkReply metaDataChanged";
    });
    connect(m_networkReply, &QNetworkReply::preSharedKeyAuthenticationRequired, this,
    [=](QSslPreSharedKeyAuthenticator *authenticator){
        qDebug() << "networkReply preSharedKeyAuthenticationRequired";
    });
    connect(m_networkReply, &QNetworkReply::redirectAllowed, this,
    [=](){
        qDebug() << "networkReply redirectAllowed";
    });
    connect(m_networkReply, &QNetworkReply::redirected, this,
    [=](const QUrl &url){
        qDebug() << "networkReply redirected";
    });
    connect(m_networkReply, &QNetworkReply::sslErrors, this,
    [=](const QList<QSslError> &errors){
        qDebug() << "networkReply sslErrors";
    });
    connect(m_networkReply, &QNetworkReply::uploadProgress, this,
    [=](qint64 bytesSent, qint64 bytesTotal){
        qDebug() << "networkReply uploadProgress";
    });
    connect(m_networkReply, &QObject::destroyed, [] {
        qDebug() << "networkReply got deleted!";
    });
}


void Window::slotCurrentTextChanged(const QString text)
{
    m_redirectPolicy = policies.at(text);
}

void Window::slotHttpRequestFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        m_button->setText("ERROR\n" + reply->errorString());
    } else {
        m_button->setText("SUCCESS\n" + reply->readAll());
    }
}

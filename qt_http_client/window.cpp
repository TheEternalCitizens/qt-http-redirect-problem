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

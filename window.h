#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QComboBox>
#include <QtNetwork>

class QPushButton;
class Window : public QWidget
{
    Q_OBJECT
public:
    explicit Window(QWidget *parent = nullptr);
private slots:
    void slotButtonClicked(bool checked);
    void slotHttpRequestFinished(QNetworkReply*);
    void slotCurrentTextChanged(QString);
private:
    QPushButton *m_button;
    QComboBox *m_comboBox;
    QNetworkAccessManager* m_networkAccessManager;
    QNetworkRequest::RedirectPolicy m_redirectPolicy;

signals:

};

#endif // WINDOW_H

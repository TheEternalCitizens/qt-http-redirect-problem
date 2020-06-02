#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
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
private:
    QPushButton *m_button;
    QNetworkAccessManager* m_networkAccessManager;

signals:

};

#endif // WINDOW_H

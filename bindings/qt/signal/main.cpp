#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDebug>
#include <QSocketNotifier>
#include <QStringList>
#include <cstdio>

class TestServer : public QLocalServer{
    Q_OBJECT;
public:
    TestServer():QLocalServer(){
        connect(this,SIGNAL(newConnection()),this,SLOT(hasNewConnection()));
        setvbuf(stdin , NULL , _IONBF , 0);
        notify=new QSocketNotifier(
#ifdef Q_CC_MSVC
                               _fileno(stdin)
#else
                               fileno(stdin)
#endif
                               , QSocketNotifier::Read, this);
        connect(notify, SIGNAL(activated(int)), this, SLOT(activated(int)));
    }
private slots:
    void hasNewConnection(){
        QLocalSocket * socket=nextPendingConnection();
        connect(socket,SIGNAL(disconnected ()),this,SLOT(socketDisconnected()));
        sockets<<socket;
    }
    void socketDisconnected(){
        QLocalSocket * socket=qobject_cast<QLocalSocket*>(sender());
        sockets.removeAll(socket);
        socket->deleteLater();
    }
    void activated(int){
        char c = getchar();
        char cs [2];
        cs[1]=0;
        cs[0]=c;
        foreach(QLocalSocket * s,sockets){
            s->write(cs);
        }
        if (c == EOF) {
            qApp->quit();
            return;
        }
    }
private:
    QList<QLocalSocket*> sockets;
    QSocketNotifier * notify;
};

int main(int argc, char ** argv){
    QCoreApplication app(argc,argv);
    if (app.arguments().count()<2){
        qDebug("usage: %s  /var/ipc/user/name/app/signal",qPrintable(app.arguments().at(0)));
        return 1;
    }
    TestServer t;
    if(!t.listen(app.arguments().at(1))){
        qDebug()<<t.errorString();
        return 2;
    }
    return app.exec();
}

#include "main.moc"

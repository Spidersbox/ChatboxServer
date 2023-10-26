#include "chatterboxserver.h"

#include <QSqlDatabase>
#include <QSqlError>  // db.lastError error reporting
#include <QtSql>
#include <QTcpSocket>
#include <QRegExp>

QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
bool ok=0;

ChatterBoxServer::ChatterBoxServer(QObject *parent) : QTcpServer(parent)
{
  // Initialize the database:
//  QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
  db.setHostName("localhost");
  db.setDatabaseName("CHAT");
  db.setUserName("chat");
  db.setPassword("chat1");
  ok=db.open();
  if(!ok)
  {
    qDebug() << db.lastError();
    qFatal( "Failed to connect." );
  }
  qDebug() << "Database is connected.";
  QSqlQuery qry;

  qry.prepare("SET SESSION wait_timeout = 345600");
  if(!qry.exec())
    qDebug() << qry.lastError();

  qry.prepare("SET SESSION interactive_timeout = 345600");
  if(!qry.exec())
    qDebug() << qry.lastError();

  qry.prepare( "SHOW VARIABLES LIKE '%timeout%'");
  if(!qry.exec())
    qDebug() << qry.lastError();
  else
  {
QString user, message;
    QSqlRecord data = qry.record();
    for( int r=0; qry.next(); r++ )
    {
      user=qry.value(0).toString();
      message=qry.value(1).toString();
qDebug() <<user << ":" << message;
db.close();
//    client->write(QString("Server: "+ user + ":" + message + "\n").toUtf8());
    }
//    qDebug() << data;
  }
}


void ChatterBoxServer::incomingConnection(int socketfd)
{
    QTcpSocket *client = new QTcpSocket(this);
    client->setSocketDescriptor(socketfd);
    clients.insert(client);

    qDebug() << "New client from:" << client->peerAddress().toString();

    connect(client, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));
}

void ChatterBoxServer::readyRead()
{
    QTcpSocket *client = (QTcpSocket*)sender();
    while(client->canReadLine())
    {
        QString line = QString::fromUtf8(client->readLine()).trimmed();
//        qDebug() << "Read line:" << line;

        QRegExp meRegex("^/me:(.*)$");

        if(meRegex.indexIn(line) != -1)
        {
            QString user = meRegex.cap(1);
            users[client] = user;
            foreach(QTcpSocket *client, clients)
                client->write(QString("Server:" + user + " has joined.\n").toUtf8());
sendMessages(user,client);
            sendUserList();
        }
        else if(users.contains(client))
        {
            QString message = line;
            QString user = users[client];
//            qDebug() << "User:" << user;
//            qDebug() << "Message:" << message;

//  if(!db.isOpen())
    ok=db.open();
  dbAdd(user,message);
  db.close();

   // current date/time based on current system
   time_t now = time(0);

   // convert now to string form
   QString date = ctime(&now);
            foreach(QTcpSocket *otherClient, clients)
                otherClient->write(QString(user+" at "+date.trimmed() + ": " + message + "\n").toUtf8());
        }
        else
        {
            qWarning() << "Got bad message from client:" << client->peerAddress().toString() << line;
        }
    }
}

void ChatterBoxServer::disconnected()
{
  QTcpSocket *client = (QTcpSocket*)sender();
  qDebug() << "Client disconnected:" << client->peerAddress().toString();

  clients.remove(client);

  QString user = users[client];
  users.remove(client);
  if(user.length())
  {
    sendUserList();
    foreach(QTcpSocket *client, clients)
      client->write(QString("Server:" + user + " has left.\n").toUtf8());
  }
}

void ChatterBoxServer::sendUserList()
{
//  if(!db.isOpen())
//    ok=db.open();
  QStringList userList;
  foreach(QString user, users.values())
    userList << user;

  foreach(QTcpSocket *client, clients)
    client->write(QString("/users:" + userList.join(",") + "\n").toUtf8());
}

void ChatterBoxServer::dbAdd(QString user,QString message)
{
//  if(!db.isOpen())
    ok=db.open();
  if(ok)  // is db open?
  {
    message = message.replace("\'", "\\\'");
    message = message.replace("\"", "\\\"");


    // current date/time based on current system
    time_t now = time(0);
   
    // convert now to string form
    QString date = ctime(&now);

    QSqlQuery qry;
    qry.prepare( "INSERT INTO MESS(User,Date, Message) VALUES ('"+user+"','"+date.trimmed()+"',\""+message+"\")");
    if(!qry.exec())
      qDebug() << qry.lastError();
    else
    {
      qDebug() << "dbAdd: adding " << message <<" by " <<user <<" at "<<date;
      db.close();
    }
  }
  else
  {
    qDebug() << "dbAdd: database is not open";
  }
}

void ChatterBoxServer::sendMessages(QString user,QTcpSocket *client)
{
//  if(!db.isOpen())
    ok=db.open();
  int maxmess=30;
  QSqlQuery qry;

  QString message ="Hello ";
  QString date="";
  message.append(user);
  message.append(", these are the last ");
  message.append(QString::number(maxmess));
  message.append(" messages");
  client->write(QString("Server: " + message + "\n").toUtf8());
  user="";
  message="";

  QString command ="SELECT * FROM \(SELECT * FROM MESS ORDER BY id DESC LIMIT ";
  command.append(QString::number(maxmess));
  command.append(" \) sub ORDER BY id ASC");

  qry.prepare(command);
  if(!qry.exec())
    qDebug() << qry.lastError();

  QSqlRecord data = qry.record();

  for( int r=0; qry.next(); r++ )
  {
    user=qry.value(1).toString();
    date=qry.value(2).toString();
    message=qry.value(3).toString();
qDebug() <<user << ":" << message;

    client->write(QString("Server: "+ user +" at "+date+ ": " + message + "\n").toUtf8());
  }
  db.close();
}


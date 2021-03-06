#ifndef PGENET_SERVER_H
#define PGENET_SERVER_H

#include <mutex>
#include <condition_variable>
#include <atomic>

#include <asio.hpp>
#include <asio/io_service.hpp>

#include <QScopedPointer>


#include <QObject>


#include <QtConcurrent>

#include "low-level/pgenetll_server.h"
#include "util/rawpacketdecoder.h"

#include <ConnectionLib/Shared/pgenet_packetmanager.h>
#include <ConnectionLib/Server/user/pgenet_usermanager.h>

#include <ConnectionLib/Shared/util/threadedlogger.h>

#include "session/pgenet_globalsession.h"


using asio::ip::tcp;

class PGENET_Server : public QObject, public PGENET_PacketManager
{
    Q_OBJECT
    Q_DISABLE_COPY(PGENET_Server)
public:
    PGENET_Server(QObject* parent = 0);
    virtual ~PGENET_Server();

    void start();

    enum class PGENET_ServerState {
        Closed,
        Running
    };

    PGENET_ServerState currentState() const;
    PGENET_UserManager* getUserManager();

signals:
    void incomingText(QString text);

private:



    // ///////////// GENERAL STATE ////////////////////////

    PGENET_ServerState m_currentState;


    // ///////////// ASYC IO_SERVICE RUNNER ///////////////

    void _ioService_run();
    QFuture<void> _ioServiceState;

    // ///////////// ASYC TOOLS ///////////////////////////

    std::mutex _bgWorker_mutex;
    std::condition_variable _bgWorker_alert;

    void _bgWorker_quit();

    // BACKGROUND WORKER:
    void _bgWorker_WaitForIncomingFullPackets();
    void _bgWorker_WaitForIncomingFullPacketsUnindentified();
    QFuture<void> _bgWorkerState_FullPackets;
    QFuture<void> _bgWorkerState_FullPacketsUnindentified;

    // ///////////// SERVER TOOLS /////////////////////////


    // User Manager:
    PGENET_UserManager m_userManager;

    // Packet Decoder:
    RawPacketDecoder m_pckDecoder;
    std::shared_ptr<ThreadedQueue<Packet*> > m_fullPackets;
    std::shared_ptr<ThreadedQueue_UnindentifedPackets> m_fullPacketsUnindentified;

    // Sessions:
    PGENET_GlobalSession m_globalSession;
    QMap<int, std::shared_ptr<PGENET_Session> > m_registeredSessions;

    // SERVICE / SERVER
    QScopedPointer<asio::io_service> m_service;
    PGENETLL_Server m_llserver;


};

#endif // PGENET_SERVER_H

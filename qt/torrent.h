/*
 * This file Copyright (C) Mnemosyne LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 * $Id$
 */

#ifndef QTR_TORRENT_H
#define QTR_TORRENT_H

#include <QObject>
#include <QIcon>
#include <QMetaType>
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QList>
#include <QTemporaryFile>
#include <QVariant>

#include <libtransmission/transmission.h>
#include <libtransmission/quark.h>

#include "speed.h"
#include "types.h"

extern "C"
{
    struct tr_variant;
}

class Prefs;
class QPixmap;
class QStyle;

struct Peer
{
    QString address;
    QString clientName;
    bool clientIsChoked;
    bool clientIsInterested;
    QString flagStr;
    bool isDownloadingFrom;
    bool isEncrypted;
    bool isIncoming;
    bool isUploadingTo;
    bool peerIsChoked;
    bool peerIsInterested;
    int port;
    double progress;
    Speed rateToClient;
    Speed rateToPeer;
};

typedef QList<Peer> PeerList;
Q_DECLARE_METATYPE(Peer)
Q_DECLARE_METATYPE(PeerList)

struct TrackerStat
{
    QString announce;
    int announceState;
    int downloadCount;
    bool hasAnnounced;
    bool hasScraped;
    QString host;
    int id;
    bool isBackup;
    int lastAnnouncePeerCount;
    QString lastAnnounceResult;
    int lastAnnounceStartTime;
    bool lastAnnounceSucceeded;
    int lastAnnounceTime;
    bool lastAnnounceTimedOut;
    QString lastScrapeResult;
    int lastScrapeStartTime;
    bool lastScrapeSucceeded;
    int lastScrapeTime;
    bool lastScrapeTimedOut;
    int leecherCount;
    int nextAnnounceTime;
    int nextScrapeTime;
    int scrapeState;
    int seederCount;
    int tier;
    QPixmap getFavicon( ) const;
};

typedef QList<TrackerStat> TrackerStatsList;
Q_DECLARE_METATYPE(TrackerStat)
Q_DECLARE_METATYPE(TrackerStatsList)

struct TrFile
{
    TrFile(): index(-1), priority(0), wanted(true), size(0), have(0) { }
    int index;
    int priority;
    bool wanted;
    uint64_t size;
    uint64_t have;
    QString filename;
};

typedef QList<TrFile> FileList;
Q_DECLARE_METATYPE(TrFile)
Q_DECLARE_METATYPE(FileList)


class Torrent: public QObject
{
        Q_OBJECT;

    public:

        enum
        {
            ID,
            UPLOAD_SPEED,
            DOWNLOAD_SPEED,
            DOWNLOAD_DIR,
            ACTIVITY,
            NAME,
            ERROR,
            ERROR_STRING,
            SIZE_WHEN_DONE,
            LEFT_UNTIL_DONE,
            HAVE_UNCHECKED,
            HAVE_VERIFIED,
            DESIRED_AVAILABLE,
            TOTAL_SIZE,
            PIECE_SIZE,
            PIECE_COUNT,
            PEERS_GETTING_FROM_US,
            PEERS_SENDING_TO_US,
            WEBSEEDS_SENDING_TO_US,
            PERCENT_DONE,
            METADATA_PERCENT_DONE,
            PERCENT_VERIFIED,
            DATE_ACTIVITY,
            DATE_ADDED,
            DATE_STARTED,
            DATE_CREATED,
            PEERS_CONNECTED,
            ETA,
            RATIO,
            DOWNLOADED_EVER,
            UPLOADED_EVER,
            FAILED_EVER,
            TRACKERS,
            TRACKERSTATS,
            MIME_ICON,
            SEED_RATIO_LIMIT,
            SEED_RATIO_MODE,
            SEED_IDLE_LIMIT,
            SEED_IDLE_MODE,
            DOWN_LIMIT,
            DOWN_LIMITED,
            UP_LIMIT,
            UP_LIMITED,
            HONORS_SESSION_LIMITS,
            PEER_LIMIT,
            HASH_STRING,
            IS_FINISHED,
            IS_PRIVATE,
            IS_STALLED,
            COMMENT,
            CREATOR,
            MANUAL_ANNOUNCE_TIME,
            PEERS,
            TORRENT_FILE,
            BANDWIDTH_PRIORITY,
            QUEUE_POSITION,

            PROPERTY_COUNT
        };

    public:
        Torrent( Prefs&, int id );
        virtual ~Torrent( );

    signals:
        void torrentChanged( int id );
        void torrentCompleted( int id );

    private:

        enum Group
        {
            INFO, // info fields that only need to be loaded once
            STAT, // commonly-used stats that should be refreshed often
            STAT_EXTRA,  // rarely used; only refresh if details dialog is open
            DERIVED // doesn't come from RPC
        };

        struct Property
        {
            int id;
            tr_quark key;
            int type;
            int group;
        };

        static Property myProperties[];

        bool magnetTorrent;

    public:
        typedef QList<tr_quark> KeyList;
        static const KeyList& getInfoKeys( );
        static const KeyList& getStatKeys( );
        static const KeyList& getExtraStatKeys( );

    private:
        static KeyList buildKeyList( Group group );

    private:
        QVariant myValues[PROPERTY_COUNT];

        int getInt            ( int key ) const;
        bool getBool          ( int key ) const;
        QTime getTime         ( int key ) const;
        QIcon getIcon         ( int key ) const;
        double getDouble      ( int key ) const;
        qulonglong getSize    ( int key ) const;
        QString getString     ( int key ) const;
        QDateTime getDateTime ( int key ) const;

        bool setInt        ( int key, int value );
        bool setBool       ( int key, bool value );
        bool setIcon       ( int key, const QIcon& );
        bool setDouble     ( int key, double );
        bool setString     ( int key, const char * );
        bool setSize       ( int key, qulonglong );
        bool setDateTime   ( int key, const QDateTime& );

    public:
        int getBandwidthPriority( ) const { return getInt( BANDWIDTH_PRIORITY ); }
        int id( ) const { return getInt( ID ); }
        QString name( ) const { return getString( NAME ); }
        QString creator( ) const { return getString( CREATOR ); }
        QString comment( ) const { return getString( COMMENT ); }
        QString getPath( ) const { return getString( DOWNLOAD_DIR ); }
        QString getError( ) const;
        QString hashString( ) const { return getString( HASH_STRING ); }
        QString torrentFile( ) const { return getString( TORRENT_FILE ); }
        bool hasError( ) const { return !getError( ).isEmpty( ); }
        bool isDone( ) const { return getSize( LEFT_UNTIL_DONE ) == 0; }
        bool isSeed( ) const { return haveVerified() >= totalSize(); }
        bool isPrivate( ) const { return getBool( IS_PRIVATE ); }
        bool getSeedRatio( double& setme ) const;
        uint64_t haveVerified( ) const { return getSize( HAVE_VERIFIED ); }
        uint64_t haveUnverified( ) const { return getSize( HAVE_UNCHECKED ); }
        uint64_t desiredAvailable( ) const { return getSize( DESIRED_AVAILABLE ); }
        uint64_t haveTotal( ) const { return haveVerified( ) + haveUnverified(); }
        uint64_t totalSize( ) const { return getSize( TOTAL_SIZE ); }
        uint64_t sizeWhenDone( ) const { return getSize( SIZE_WHEN_DONE ); }
        uint64_t leftUntilDone( ) const { return getSize( LEFT_UNTIL_DONE ); }
        uint64_t pieceSize( ) const { return getSize( PIECE_SIZE ); }
        bool hasMetadata( ) const { return getDouble( METADATA_PERCENT_DONE ) >= 1.0; }
        bool isMagnet( ) const { return magnetTorrent; }
        int  pieceCount( ) const { return getInt( PIECE_COUNT ); }
        double ratio( ) const { return getDouble( RATIO ); }
        double percentComplete( ) const { return haveTotal() / (double)totalSize(); }
        double percentDone( ) const { return getDouble( PERCENT_DONE ); }
        double metadataPercentDone( ) const { return getDouble( METADATA_PERCENT_DONE ); }
        uint64_t downloadedEver( ) const { return getSize( DOWNLOADED_EVER ); }
        uint64_t uploadedEver( ) const { return getSize( UPLOADED_EVER ); }
        uint64_t failedEver( ) const { return getSize( FAILED_EVER ); }
        int compareTracker( const Torrent& ) const;
        int compareSeedRatio( const Torrent& ) const;
        int compareRatio( const Torrent& ) const;
        int compareETA( const Torrent& ) const;
        bool hasETA( ) const { return getETA( ) >= 0; }
        int getETA( ) const { return getInt( ETA ); }
        QDateTime lastActivity( ) const { return getDateTime( DATE_ACTIVITY ); }
        QDateTime lastStarted( ) const { return getDateTime( DATE_STARTED ); }
        QDateTime dateAdded( ) const { return getDateTime( DATE_ADDED ); }
        QDateTime dateCreated( ) const { return getDateTime( DATE_CREATED ); }
        QDateTime manualAnnounceTime( ) const { return getDateTime( MANUAL_ANNOUNCE_TIME ); }
        bool canManualAnnounce( ) const { return isReadyToTransfer() && (manualAnnounceTime()<=QDateTime::currentDateTime()); }
        int peersWeAreDownloadingFrom( ) const { return getInt( PEERS_SENDING_TO_US ); }
        int webseedsWeAreDownloadingFrom( ) const { return getInt( WEBSEEDS_SENDING_TO_US ); }
        int peersWeAreUploadingTo( ) const { return getInt( PEERS_GETTING_FROM_US ); }
        bool isUploading( ) const { return peersWeAreUploadingTo( ) > 0; }
        int connectedPeers( ) const { return getInt( PEERS_CONNECTED ); }
        int connectedPeersAndWebseeds( ) const { return connectedPeers( ) + getInt( WEBSEEDS_SENDING_TO_US ); }
        Speed downloadSpeed( ) const { return Speed::fromBps( getSize( DOWNLOAD_SPEED ) ); }
        Speed uploadSpeed( ) const { return Speed::fromBps( getSize( UPLOAD_SPEED ) ); }
        double getVerifyProgress( ) const { return getDouble( PERCENT_VERIFIED ); }
        bool hasFileSubstring( const QString& substr ) const;
        bool hasTrackerSubstring( const QString& substr ) const;
        Speed uploadLimit( ) const { return Speed::fromKBps( getInt( UP_LIMIT ) ); }
        Speed downloadLimit( ) const { return Speed::fromKBps( getInt( DOWN_LIMIT ) ); }
        bool uploadIsLimited( ) const { return getBool( UP_LIMITED ); }
        bool downloadIsLimited( ) const { return getBool( DOWN_LIMITED ); }
        bool honorsSessionLimits( ) const { return getBool( HONORS_SESSION_LIMITS ); }
        int peerLimit( ) const { return getInt( PEER_LIMIT ); }
        double seedRatioLimit( ) const { return getDouble( SEED_RATIO_LIMIT ); }
        tr_ratiolimit seedRatioMode( ) const { return (tr_ratiolimit) getInt( SEED_RATIO_MODE ); }
        int seedIdleLimit( ) const { return getInt( SEED_IDLE_LIMIT ); }
        tr_idlelimit seedIdleMode( ) const { return (tr_idlelimit) getInt( SEED_IDLE_MODE ); }
        TrackerStatsList trackerStats( ) const{ return myValues[TRACKERSTATS].value<TrackerStatsList>(); }
        QStringList trackers() const { return myValues[TRACKERS].value<QStringList>(); }
        PeerList peers( ) const{ return myValues[PEERS].value<PeerList>(); }
        const FileList& files( ) const { return myFiles; }
        int queuePosition( ) const { return getInt( QUEUE_POSITION ); }
        bool isStalled( ) const { return getBool( IS_STALLED ); }

    public:
        QString activityString( ) const;
        tr_torrent_activity getActivity( ) const { return (tr_torrent_activity) getInt( ACTIVITY ); }
        bool isFinished( ) const { return getBool( IS_FINISHED ); }
        bool isPaused( ) const { return getActivity( ) == TR_STATUS_STOPPED; }
        bool isWaitingToVerify( ) const { return getActivity( ) == TR_STATUS_CHECK_WAIT; }
        bool isVerifying( ) const { return getActivity( ) == TR_STATUS_CHECK; }
        bool isDownloading( ) const { return getActivity( ) == TR_STATUS_DOWNLOAD; }
        bool isWaitingToDownload( ) const { return getActivity( ) == TR_STATUS_DOWNLOAD_WAIT; }
        bool isSeeding( ) const { return getActivity( ) == TR_STATUS_SEED; }
        bool isWaitingToSeed( ) const { return getActivity( ) == TR_STATUS_SEED_WAIT; }
        bool isReadyToTransfer( ) const { return getActivity()==TR_STATUS_DOWNLOAD || getActivity()==TR_STATUS_SEED; }
        bool isQueued( ) const { return isWaitingToDownload() || isWaitingToSeed(); }
        void notifyComplete( ) const;

    public:
        void update( tr_variant * dict );
        void setMagnet( bool magnet ) { magnetTorrent = magnet; }

    private:
        const char * getMimeTypeString( ) const;
        void updateMimeIcon( );

    public:
        QIcon getMimeTypeIcon( ) const { return getIcon( MIME_ICON ); }

    private:
        Prefs& myPrefs;
        FileList myFiles;
};

Q_DECLARE_METATYPE(const Torrent*)

#endif


/* This file is part of the Springlobby (GPL v2 or later), see COPYING */

/**
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

DO NOT CHANGE THIS FILE!

this file is deprecated and will be replaced with

lsl/networking/tasserver.h

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
**/


#ifndef SPRINGLOBBY_HEADERGUARD_TASSERVER_H
#define SPRINGLOBBY_HEADERGUARD_TASSERVER_H

#include <wx/string.h>
#include <wx/longlong.h>
#include <wx/timer.h>
#include <list>

#include "iserver.h"
#include "utils/crc.h"

const unsigned int FIRST_UDP_SOURCEPORT = 8300;

class Ui;
class Socket;
class User;
struct UserBattleStatus;
class IServerEvents;
class wxString;
class PingThread;

//! @brief TASServer protocol implementation.
class TASServer : public IServer, public wxTimer
{
public:
	TASServer();
	~TASServer();

	// Overloaded functions from Server
	bool ExecuteSayCommand( const wxString& cmd );

	void Register( const wxString& servername, const wxString& host, const int port, const wxString& nick, const wxString& password);
	void AcceptAgreement();

	void Connect( const wxString& servername, const wxString& addr, const int port );
	void Disconnect();
	bool IsConnected();

	void Login();
	void Logout();
	bool IsOnline() const;

	void Ping();

	void UDPPing();/// used for nat travelsal
	/// generic udp "ping" function
	/// return value: actual source port which was used. May differ from src_port
	/// 0 if udp ping failed
	unsigned int UdpPing(unsigned int src_port, const wxString &target, unsigned int target_port, const wxString &message);
	/// specialized udp ping functions
	void UdpPingTheServer( const wxString &message );/// used for nat travelsal. pings the server.
	void UdpPingAllClients();/// used when hosting with nat holepunching

	const User& GetMe() const;
	User& GetMe();

	void JoinChannel( const wxString& channel, const wxString& key );
	void PartChannel( const wxString& channel );

	void DoActionChannel( const wxString& channel, const wxString& msg );
	void SayChannel( const wxString& channel, const wxString& msg );

	void DoActionPrivate( const wxString& nick, const wxString& msg );
	void SayPrivate( const wxString& nick, const wxString& msg );

	void SayBattle( int battleid, const wxString& msg );
	void DoActionBattle( int battleid, const wxString& msg );

	void Ring( const wxString& nick );

	void ModeratorSetChannelTopic( const wxString& channel, const wxString& topic );
	void ModeratorSetChannelKey( const wxString& channel, const wxString& key );
	void ModeratorMute( const wxString& channel, const wxString& nick, int duration, bool byip );
	void ModeratorUnmute( const wxString& channel, const wxString& nick );
	void ModeratorKick( const wxString& channel, const wxString& reason );
	void ModeratorBan( const wxString& nick, bool byip );
	void ModeratorUnban( const wxString& nick );
	void ModeratorGetIP( const wxString& nick );
	void ModeratorGetLastLogin( const wxString& nick );
	void ModeratorGetLastIP( const wxString& nick );
	void ModeratorFindByIP( const wxString& ipadress );

	void AdminGetAccountAccess( const wxString& nick );
	void AdminChangeAccountAccess( const wxString& nick, const wxString& accesscode );
	void AdminSetBotMode( const wxString& nick, bool isbot );

	void HostBattle( BattleOptions bo, const wxString& password = wxEmptyString );
	void JoinBattle( const int& battleid, const wxString& password = wxEmptyString );
	void LeaveBattle( const int& battleid );
	void SendMyBattleStatus( UserBattleStatus& bs );
	void SendMyUserStatus(const UserStatus& us);

	void ForceSide( int battleid, User& user, int side );
	void ForceTeam( int battleid, User& user, int team );
	void ForceAlly( int battleid, User& user, int ally );
	void ForceColour( int battleid, User& user, const LSL::lslColor& col );
	void ForceSpectator( int battleid, User& user, bool spectator );
	void BattleKickPlayer( int battleid, User& user );
	void SetHandicap( int battleid, User& user, int handicap);

	void AddBot( int battleid, const wxString& nick, UserBattleStatus& status );
	void RemoveBot( int battleid, User& bot );
	void UpdateBot( int battleid, User& bot, UserBattleStatus& status );

	void StartHostedBattle();
	void SendHostInfo( HostInfo update );
	void SendHostInfo( const wxString& Tag );
	void SendUserPosition( const User& user );

	void SendRaw( const wxString& raw );

	void RequestInGameTime( const wxString& nick );

	void SendUdpSourcePort( int udpport );
	void SendNATHelperInfos( const wxString& username, const wxString& ip, int port );

	IBattle* GetCurrentBattle();

	void RequestChannels();
	// TASServer specific functions
	void ExecuteCommand( const wxString& in );
	void ExecuteCommand( const wxString& cmd, const wxString& inparams, int replyid = -1 );

	void HandlePong( int replyid );

	void OnConnected(Socket& sock );
	void OnDisconnected(Socket& sock );
	void OnDataReceived(Socket& sock );
	void OnError(const wxString& err);

	bool IsPasswordHash( const wxString& pass )  const;
	wxString GetPasswordHash( const wxString& pass ) const;

	int TestOpenPort( unsigned int port ) const;

/*
	void SendScriptToProxy( const wxString& script );

	void SendScriptToClients( const wxString& script );
*/

	void SetRelayIngamePassword( const User& user );

	wxArrayString GetRelayHostList() ;
	virtual const IServerEvents* serverEvents() const {
		return m_se;
	}

private:

	//! @brief Struct used internally by the TASServer class to calculate ping roundtimes.
	struct TASPingListItem {
		int id;
		wxLongLong t;
	};

	CRC m_crc;

	IServerEvents* m_se;
	double m_ser_ver;

	wxString m_last_denied;
	bool m_connected;
	bool m_online;
	bool m_debug_dont_catch;
	bool m_id_transmission;
	bool m_redirecting;
	wxString m_buffer;
	int m_last_udp_ping;
	int m_last_ping; //time last ping was sent
	int m_last_net_packet; //time last packet was received
	wxLongLong m_lastnotify;
	unsigned int m_last_id;

	std::list<TASPingListItem> m_pinglist;

	unsigned long m_udp_private_port;
	unsigned long m_nat_helper_port;

	int m_battle_id;

	bool m_server_lanmode;
	unsigned int m_account_id_count;

	wxString m_agreement;

	wxString m_addr;
	wxString m_delayed_open_command;

	bool m_do_finalize_join_battle;
	int m_finalize_join_battle_id;
	wxString m_finalize_join_battle_pw;
	bool m_token_transmission;

	void FinalizeJoinBattle();

	void SendCmd( const wxString& command, const wxString& param = wxEmptyString );
	void RelayCmd( const wxString& command, const wxString& param = wxEmptyString );

	wxString m_current_chan_name_mutelist;

	wxArrayString m_relay_host_manager_list;

private:
	void Notify();

    wxString m_relay_host_bot;
    wxString m_relay_host_manager;

};

#endif // SPRINGLOBBY_HEADERGUARD_TASSERVER_H

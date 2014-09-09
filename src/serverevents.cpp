/* This file is part of the Springlobby (GPL v2 or later), see COPYING */

//
// Class: ServerEvents
//

#ifdef _MSC_VER
#ifndef NOMINMAX
    #define NOMINMAX
#endif // NOMINMAX
#include <winsock2.h>
#endif // _MSC_VER

#include <wx/intl.h>
#include <wx/log.h>
#include <stdexcept>

#include "serverevents.h"
#include "gui/mainwindow.h"
#include "gui/ui.h"
#include "channel.h"
#include "user.h"
#include "gui/uiutils.h"
#include "iserver.h"
#include "downloader/httpdownloader.h"
#include "settings.h"
#include "gui/customdialogs.h"
#include "utils/tasutil.h"
#include "utils/uievents.h"
#include "log.h"
#include "utils/conversion.h"

#include <lslutils/globalsmanager.h>

void ServerEvents::OnConnected( const wxString& server_name, const wxString& server_ver, bool supported, const wxString& server_spring_ver, bool /*unused*/ )
{
	slLogDebugFunc("%s %s", STD_STRING(server_ver).c_str(), STD_STRING(server_spring_ver).c_str());
    //Server version will include patchlevel from release 89 onwards
    m_serv.SetRequiredSpring( server_spring_ver.BeforeFirst('.') );
    ui().OnConnected( m_serv, server_name, server_ver, supported );
    m_serv.Login();
}


void ServerEvents::OnDisconnected( bool wasonline )
{
	slLogDebugFunc("");
    m_serv.SetRequiredSpring (wxEmptyString);
	try {
		ui().OnDisconnected( m_serv, wasonline );
	} catch (LSL::Util::GlobalDestroyedError& ) {
		/* At the end of the program, the global reference in ui() might already have been nullified. */
	}
}


void ServerEvents::OnLogin()
{
}


void ServerEvents::OnLoginInfoComplete()
{
	slLogDebugFunc("");
	wxString nick = TowxString(m_serv.GetMe().GetNick());
	wxArrayString highlights = sett().GetHighlightedWords();
	if ( highlights.Index( nick ) == -1 )
	{
		highlights.Add( nick );
		sett().SetHighlightedWords( highlights );
	}
    //m_serv.RequestChannels();
	GlobalEvent::Send(GlobalEvent::OnLogin);
    std::vector<ChannelJoinInfo> autojoin = sett().GetChannelsJoin();
	for ( std::vector<ChannelJoinInfo>::const_iterator itor = autojoin.begin(); itor != autojoin.end(); ++itor )
    {
		if ( itor->name.IsEmpty() )
			continue;
		Channel& chan = m_serv._AddChannel( itor->name );
		chan.SetPassword( STD_STRING(itor->password) );
		ui().OnJoinedChannelSuccessful( chan, itor == autojoin.begin() );
	}
	for ( std::vector<ChannelJoinInfo>::const_iterator itor = autojoin.begin(); itor != autojoin.end(); ++itor )
        m_serv.JoinChannel( itor->name, itor->password );

    ui().OnLoggedIn( );
}


void ServerEvents::OnUnknownCommand( const wxString& command, const wxString& params )
{
	slLogDebugFunc("");
    ui().OnUnknownCommand( m_serv, command, params );
}


void ServerEvents::OnSocketError( const Sockerror& /*unused*/ )
{
    //wxLogDebugFunc( wxEmptyString );
}


void ServerEvents::OnProtocolError( const Protocolerror /*unused*/ )
{
    //wxLogDebugFunc( wxEmptyString );
}


void ServerEvents::OnMotd( const wxString& msg )
{
	slLogDebugFunc("");
    ui().OnMotd( m_serv, msg );
}


void ServerEvents::OnPong( wxLongLong ping_time )
{
	//wxLongLong is non-POD and cannot be passed to wxFormat as such. use c-string rep instead. converting to long might loose precision
	UiEvents::StatusData data( wxString::Format( _("ping: %s ms"), ping_time.ToString().c_str() ), 2 );
	UiEvents::GetStatusEventSender( UiEvents::addStatusMessage ).SendEvent( data );
}


void ServerEvents::OnNewUser( const wxString& nick, const wxString& country, int cpu, const wxString& id )
{
	slLogDebugFunc("");
    try
    {
        ASSERT_LOGIC( !m_serv.UserExists( nick ), _T("New user from server, but already exists!") );
    }
    catch (...)
    {
        return;
    }
    User& user = m_serv._AddUser( nick );
    if ( useractions().DoActionOnUser( UserActions::ActNotifLogin, nick ) )
        actNotifBox( SL_MAIN_ICON, nick + _(" is online") );
    user.SetCountry( STD_STRING(country));
    user.SetCpu( cpu );
		user.SetID(STD_STRING(id));
    ui().OnUserOnline( user );
}


void ServerEvents::OnUserStatus( const wxString& nick, UserStatus status )
{
	slLogDebugFunc("");
    try
    {
        User& user = m_serv.GetUser( nick );

        UserStatus oldStatus = user.GetStatus();
        user.SetStatus( status );
        if ( useractions().DoActionOnUser( UserActions::ActNotifStatus, nick ) )
        {
            wxString diffString = TowxString(status.GetDiffString( oldStatus ));
            if ( diffString != wxEmptyString )
                actNotifBox( SL_MAIN_ICON, nick + _(" is now ") + diffString );
        }

        ui().OnUserStatusChanged( user );
        if ( user.GetBattle() != 0 )
        {
            IBattle& battle = *user.GetBattle();
            try
            {
            if ( battle.GetFounder().GetNick() == user.GetNick() )
            {
                if ( status.in_game != battle.GetInGame() )
                {
                    battle.SetInGame( status.in_game );
                    if ( status.in_game ) battle.StartSpring();
					else
						BattleEvents::GetBattleEventSender( BattleEvents::BattleInfoUpdate ).SendEvent( std::make_pair(user.GetBattle(),wxString()) );
                }
            }
            }catch(...){}
        }
    }
    catch (...)
    {
        wxLogWarning( _("OnUserStatus() failed ! (exception)") );
    }
}


void ServerEvents::OnUserQuit( const wxString& nick )
{
	slLogDebugFunc("");
    try
    {
        User &user=m_serv.GetUser( nick );
				IBattle* userbattle = user.GetBattle();
				if ( userbattle )
				{
					int battleid = userbattle->GetID();
					try
					{
						if ( &userbattle->GetFounder() == &user )
						{
							for ( int i = 0; i < int(userbattle->GetNumUsers()); i ++ )
							{
								User& battleuser = userbattle->GetUser( i );
								OnUserLeftBattle( battleid, TowxString(battleuser.GetNick()));
							}
							 OnBattleClosed( battleid );
						}
						else OnUserLeftBattle( battleid, TowxString(user.GetNick()));
					}catch(...){}
				}
        ui().OnUserOffline( user );
        m_serv._RemoveUser( nick );
        if ( useractions().DoActionOnUser( UserActions::ActNotifLogin, nick ) )
            actNotifBox( SL_MAIN_ICON, nick + _(" just went offline") );
    }
    catch (std::runtime_error &except)
    {
    }
}


void ServerEvents::OnBattleOpened( int id, BattleType type, NatType nat, const wxString& nick,
									const wxString& host, int port, int maxplayers,
									bool haspass, int rank, const wxString& maphash, const wxString& engineName, const wxString& engineVersion, const wxString& map,
									const wxString& title, const wxString& mod )
{
	slLogDebugFunc("");
    try
    {
        ASSERT_EXCEPTION( !m_serv.BattleExists( id ), _T("New battle from server, but already exists!") );
        IBattle& battle = m_serv._AddBattle( id );

        User& user = m_serv.GetUser( nick );
        battle.OnUserAdded( user );

        battle.SetBattleType( type );
        battle.SetNatType( nat );
        battle.SetFounder( STD_STRING(nick));
        battle.SetHostIp( STD_STRING(host));
        battle.SetHostPort( port );
        battle.SetMaxPlayers( maxplayers );
        battle.SetIsPassworded( haspass );
        battle.SetRankNeeded( rank );
        battle.SetHostMap( STD_STRING(map), STD_STRING(maphash));
        battle.SetDescription( STD_STRING(title));
        battle.SetHostMod( STD_STRING(mod), "" );
		battle.SetEngineName(STD_STRING(engineName));
		battle.SetEngineVersion(STD_STRING(engineVersion));

        if ( useractions().DoActionOnUser( UserActions::ActNotifBattle, TowxString(user.GetNick())) )
            actNotifBox( SL_MAIN_ICON, TowxString(user.GetNick()) + _(" opened battle ") + title );

        ui().OnBattleOpened( battle );
        if ( user.Status().in_game )
        {
            battle.SetInGame( true );
            battle.StartSpring();
        }
    }
    catch (std::runtime_error &except)
    {
    }
}


void ServerEvents::OnJoinedBattle( int battleid, const wxString& hash )
{
	slLogDebugFunc("");
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );

        battle.SetHostMod( battle.GetHostModName(), STD_STRING(hash));
        UserBattleStatus& bs = m_serv.GetMe().BattleStatus();
        bs.spectator = false;

        if ( !battle.IsFounderMe() || battle.IsProxy() )
        {
            battle.CustomBattleOptions().loadOptions(LSL::Enum::MapOption, battle.GetHostMapName());
            battle.CustomBattleOptions().loadOptions(LSL::Enum::ModOption, battle.GetHostModName());
        }

        ui().OnJoinedBattle( battle );
    }
    catch (std::runtime_error &except)
    {
    }
}


void ServerEvents::OnHostedBattle( int battleid )
{
	slLogDebugFunc("");
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );

				if ( battle.GetBattleType() == BT_Played )
				{
                    battle.CustomBattleOptions().loadOptions(LSL::Enum::MapOption, battle.GetHostMapName());
                    battle.CustomBattleOptions().loadOptions(LSL::Enum::ModOption, battle.GetHostModName());
				}
				else
				{
					battle.GetBattleFromScript( true );
				}


        const std::string presetname = STD_STRING(sett().GetModDefaultPresetName( TowxString(battle.GetHostModName())));
        if ( !presetname.empty() )
        {
            battle.LoadOptionsPreset( presetname );
        }

        battle.LoadMapDefaults( battle.GetHostMapName() );

        m_serv.SendHostInfo( IBattle::HI_Send_All_opts );

        ui().OnHostedBattle( battle );
    }
    catch (assert_exception) {}
}


void ServerEvents::OnStartHostedBattle( int battleid )
{
	slLogDebugFunc("");
    IBattle& battle = m_serv.GetBattle( battleid );
    battle.SetInGame( true );
    battle.StartSpring();
}


void ServerEvents::OnClientBattleStatus( int battleid, const wxString& nick, UserBattleStatus status )
{
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
        User& user = battle.GetUser( STD_STRING(nick) );

		//if ( battle.IsFounderMe() ) AutoCheckCommandSpam( battle, user );

        status.color_index = user.BattleStatus().color_index;
        battle.OnUserBattleStatusUpdated( user, status );
    }
    catch (std::runtime_error &except)
    {
    }
}


void ServerEvents::OnUserJoinedBattle( int battleid, const wxString& nick, const wxString& userScriptPassword )
{
    try
    {
		slLogDebugFunc("");
        User& user = m_serv.GetUser( nick );
        IBattle& battle = m_serv.GetBattle( battleid );

        battle.OnUserAdded( user );
		user.BattleStatus().scriptPassword = STD_STRING(userScriptPassword);
        ui().OnUserJoinedBattle( battle, user );
				try
				{
					if ( &user == &battle.GetFounder() )
					{
							if ( user.Status().in_game )
							{
									battle.SetInGame( true );
									battle.StartSpring();
							}
					}
        }catch(...){}
    }
    catch (std::runtime_error &except)
    {
    }
}


void ServerEvents::OnUserLeftBattle( int battleid, const wxString& nick )
{
	slLogDebugFunc("");
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
        User& user = battle.GetUser(STD_STRING(nick));
        // this is necessary since the user will be deleted when the gui function is called
        bool isbot = user.BattleStatus().IsBot();
		user.BattleStatus().scriptPassword.clear();
        battle.OnUserRemoved( user );
        ui().OnUserLeftBattle( battle, user, isbot );
    }
    catch (std::runtime_error &except)
    {
    }

}


void ServerEvents::OnBattleInfoUpdated( int battleid, int spectators, bool locked, const wxString& maphash, const wxString& map )
{
	slLogDebugFunc("");
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );

        battle.SetSpectators( spectators );
        battle.SetIsLocked( locked );

        wxString oldmap = TowxString(battle.GetHostMapName());

        battle.SetHostMap( STD_STRING(map), STD_STRING(maphash));

        if ( (oldmap != map) && (battle.UserExists( m_serv.GetMe().GetNick())) )
        {
            battle.SendMyBattleStatus();
            battle.CustomBattleOptions().loadOptions(LSL::Enum::MapOption, STD_STRING(map));
            battle.Update( STD_STRING(wxString::Format( _T("%d_mapname"), LSL::Enum::PrivateOptions )));
        }

		BattleEvents::GetBattleEventSender( BattleEvents::BattleInfoUpdate ).SendEvent( std::make_pair(&battle,wxString()) );
    }
    catch (assert_exception) {}
}

void ServerEvents::OnSetBattleInfo( int battleid, const wxString& param, const wxString& value )
{
    slLogDebugFunc("%s, %s",  STD_STRING(param).c_str(), STD_STRING(value).c_str());
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
		battle.m_script_tags[STD_STRING(param)] = STD_STRING(value);
        wxString key = param;
        if ( key.Left( 5 ) == _T("game/") )
        {
            key = key.AfterFirst( '/' );
            if ( key.Left( 11 ) == _T( "mapoptions/" ) )
            {
                key = key.AfterFirst( '/' );
                battle.CustomBattleOptions().setSingleOption(STD_STRING(key), STD_STRING(value), LSL::Enum::MapOption );
                battle.Update(STD_STRING(wxString::Format(_T("%d_%s"), LSL::Enum::MapOption, key.c_str() )));
            }
            else if ( key.Left( 11 ) == _T( "modoptions/" ) )
            {
                key = key.AfterFirst( '/' );
                battle.CustomBattleOptions().setSingleOption(STD_STRING(key), STD_STRING(value), LSL::Enum::ModOption );
                battle.Update(STD_STRING(wxString::Format(_T("%d_%s"), LSL::Enum::ModOption,  key.c_str() )));
            }
            else if ( key.Left( 8 ) == _T( "restrict" ) )
            {
            	OnBattleDisableUnit( battleid, key.AfterFirst(_T('/')), s2l(value) );
            }
            else if ( key.Left( 4 ) == _T( "team" ) && key.Find( _T("startpos") ) != wxNOT_FOUND )
            {
            	 int team = s2l( key.BeforeFirst(_T('/')).Mid( 4 ) );
							 if ( key.Find( _T("startposx") ) != wxNOT_FOUND )
							 {
							 	 int numusers = battle.GetNumUsers();
							 	 for ( int i = 0; i < numusers; i++ )
							 	 {
							 	 	 User& usr = battle.GetUser( i );
							 	 	 UserBattleStatus& status = usr.BattleStatus();
							 	 	 if ( status.team == team )
							 	 	 {
							 	 	 	 status.pos.x = s2l( value );
										 battle.OnUserBattleStatusUpdated( usr, status );
							 	 	 }
							 	 }
							 }
							 else if ( key.Find( _T("startposy") ) != wxNOT_FOUND )
							 {
							 	 int numusers = battle.GetNumUsers();
							 	 for ( int i = 0; i < numusers; i++ )
							 	 {
							 	 	 User& usr = battle.GetUser( i );
							 	 	 UserBattleStatus& status = usr.BattleStatus();
							 	 	 if ( status.team == team )
							 	 	 {
							 	 	 	 status.pos.y = s2l( value );
							 	 	 	 battle.OnUserBattleStatusUpdated( usr, status );
							 	 	 }
							 	 }
							 }
            }
            else
            {
                battle.CustomBattleOptions().setSingleOption( STD_STRING(key), STD_STRING(value), LSL::Enum::EngineOption );
                battle.Update(STD_STRING(wxString::Format(_T("%d_%s"), LSL::Enum::EngineOption, key.c_str() )));
            }
        }
    }
    catch (assert_exception) {}
}

void ServerEvents::OnUnsetBattleInfo( int /*battleid*/, const wxString& /*param*/)
{
	//FIXME: implement this
}


void ServerEvents::OnBattleInfoUpdated( int battleid )
{
    slLogDebugFunc("");
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
		BattleEvents::GetBattleEventSender( BattleEvents::BattleInfoUpdate ).SendEvent( std::make_pair(&battle,wxString()) );
    }
    catch ( assert_exception ) {}
}


void ServerEvents::OnBattleClosed( int battleid )
{
	slLogDebugFunc("");
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );

        ui().OnBattleClosed( battle );

        m_serv._RemoveBattle( battleid );
    }
    catch ( assert_exception ) {}
}


void ServerEvents::OnBattleDisableUnit( int battleid, const wxString& unitname, int count )
{
	slLogDebugFunc("");
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
        battle.RestrictUnit(STD_STRING(unitname), count );
        battle.Update(STD_STRING(wxString::Format( _T("%d_restrictions"), LSL::Enum::PrivateOptions )));
    }
    catch ( assert_exception ) {}
}


void ServerEvents::OnBattleEnableUnit( int battleid, const wxString& unitname )
{
	slLogDebugFunc("");
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
        battle.UnrestrictUnit(STD_STRING(unitname));
        battle.Update(STD_STRING(wxString::Format( _T("%d_restrictions"), LSL::Enum::PrivateOptions )));
    }
    catch ( assert_exception ) {}
}


void ServerEvents::OnBattleEnableAllUnits( int battleid )
{
	slLogDebugFunc("");
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
        battle.UnrestrictAllUnits();
        battle.Update(STD_STRING(wxString::Format( _T("%d_restrictions"), LSL::Enum::PrivateOptions )));
    }
    catch ( assert_exception ) {}
}


void ServerEvents::OnJoinChannelResult( bool success, const wxString& channel, const wxString& reason )
{
	slLogDebugFunc("");
    if ( success )
    {
        Channel& chan = m_serv._AddChannel( channel );
        chan.SetPassword(STD_STRING(m_serv.m_channel_pw[channel]));
        ui().OnJoinedChannelSuccessful( chan );

    }
    else
    {
        ui().ShowMessage( _("Join channel failed"), _("Could not join channel ") + channel + _(" because: ") + reason );
    }
}


void ServerEvents::OnChannelSaid( const wxString& channel, const wxString& who, const wxString& message )
{
	slLogDebugFunc("");
    try
    {
        if ( ( m_serv.GetMe().GetNick() ==  STD_STRING(who) ) || !useractions().DoActionOnUser( UserActions::ActIgnoreChat, who ) )
            m_serv.GetChannel( channel ).Said( m_serv.GetUser( who ), STD_STRING(message) );
    }
    catch (std::runtime_error &except)
    {
    }
}


void ServerEvents::OnChannelJoin( const wxString& channel, const wxString& who )
{
	slLogDebugFunc("");
    try
    {
        m_serv.GetChannel( channel ).OnChannelJoin( m_serv.GetUser( who ) );
    }
    catch (std::runtime_error &except)
    {
    }
}


void ServerEvents::OnChannelPart( const wxString& channel, const wxString& who, const wxString& message )
{
	slLogDebugFunc("");
    try
    {
        m_serv.GetChannel( channel ).Left( m_serv.GetUser( who ), STD_STRING(message));
    }
    catch (std::runtime_error &except)
    {
    }
}


void ServerEvents::OnChannelTopic( const wxString& channel, const wxString& who, const wxString& message, int /*unused*/ )
{
	slLogDebugFunc("");
    try
    {
        m_serv.GetChannel( channel ).SetTopic( STD_STRING(message), STD_STRING(who));
    }
    catch (std::runtime_error &except)
    {
    }
}


void ServerEvents::OnChannelAction( const wxString& channel, const wxString& who, const wxString& action )
{
	slLogDebugFunc("");
    try
    {
		if ( ( m_serv.GetMe().GetNick() ==  STD_STRING(who) ) || !useractions().DoActionOnUser( UserActions::ActIgnoreChat, who ) )
			m_serv.GetChannel( channel ).DidAction( m_serv.GetUser( who ), STD_STRING(action));
    }
    catch (std::runtime_error &except)
    {
    }
}


void ServerEvents::OnPrivateMessage( const wxString& user, const wxString& message, bool fromme )
{
	slLogDebugFunc("");
    try
    {
        User& who = m_serv.GetUser( user );
        if (!useractions().DoActionOnUser( UserActions::ActIgnorePM, TowxString(who.GetNick())))
            ui().OnUserSaid( who, message, fromme );
    }
    catch (std::runtime_error &except)
    {
    }
}

void ServerEvents::OnPrivateMessageEx( const wxString& user, const wxString& action, bool fromme )
{
	slLogDebugFunc("");
	try
	{
		User& who = m_serv.GetUser( user );
		if (!useractions().DoActionOnUser( UserActions::ActIgnorePM, TowxString(who.GetNick())) )
			ui().OnUserSaidEx( who, action, fromme );
	}
	catch (std::runtime_error &except)
	{
	}
}

void ServerEvents::OnChannelList( const wxString& channel, const int& numusers, const wxString& topic )
{
    ui().mw().OnChannelList( channel, numusers, topic );
}


void ServerEvents::OnUserJoinChannel( const wxString& channel, const wxString& who )
{
	slLogDebugFunc("");
    try
    {
        m_serv.GetChannel( channel ).Joined( m_serv.GetUser( who ) );
    }
    catch (std::runtime_error &except)
    {
    }
}


void ServerEvents::OnRequestBattleStatus( int battleid )
{
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
        ui().OnRequestBattleStatus( battle );
    }
    catch (assert_exception) {}
}


void ServerEvents::OnSaidBattle( int battleid, const wxString& nick, const wxString& msg )
{
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
		if ( ( m_serv.GetMe().GetNick() ==  STD_STRING(nick)) || !useractions().DoActionOnUser( UserActions::ActIgnoreChat, nick ) )
		{
			ui().OnSaidBattle( battle, nick, msg );
		}
		AutoHost* ah = battle.GetAutoHost();
        if (ah != NULL) {
			ah->OnSaidBattle( nick, msg );
		}
    }
    catch (assert_exception) {}
}

void ServerEvents::OnBattleAction( int /*battleid*/, const wxString& nick, const wxString& msg )
{
    try
    {
		UiEvents::GetUiEventSender( UiEvents::OnBattleActionEvent ).SendEvent(
				UiEvents::OnBattleActionData( nick, msg )
			);
    }
    catch (assert_exception) {}
}


void ServerEvents::OnBattleStartRectAdd( int battleid, int allyno, int left, int top, int right, int bottom )
{
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
        battle.AddStartRect( allyno, left, top, right, bottom );
        battle.StartRectAdded( allyno );
        battle.Update(STD_STRING(wxString::Format( _T("%d_mapname"), LSL::Enum::PrivateOptions )));
    }
    catch (assert_exception) {}
}


void ServerEvents::OnBattleStartRectRemove( int battleid, int allyno )
{
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
        battle.RemoveStartRect( allyno );
        battle.StartRectRemoved( allyno );
        battle.Update(STD_STRING(wxString::Format( _T("%d_mapname"), LSL::Enum::PrivateOptions )));
    }
    catch (assert_exception) {}
}


void ServerEvents::OnBattleAddBot( int battleid, const wxString& nick, UserBattleStatus status )
{
	slLogDebugFunc("");
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
        battle.OnBotAdded(STD_STRING(nick), status );
        User& bot = battle.GetUser(STD_STRING(nick));
        ASSERT_LOGIC( &bot != 0, _T("Bot null after add.") );
        ui().OnUserJoinedBattle( battle, bot );
    }
    catch (assert_exception) {}
}

void ServerEvents::OnBattleUpdateBot( int battleid, const wxString& nick, UserBattleStatus status )
{
    OnClientBattleStatus( battleid, nick, status );
}


void ServerEvents::OnBattleRemoveBot( int battleid, const wxString& nick )
{
	slLogDebugFunc("");
    try
    {
        IBattle& battle = m_serv.GetBattle( battleid );
		User& user = battle.GetUser(STD_STRING(nick));
		bool isbot = user.BattleStatus().IsBot();
		ui().OnUserLeftBattle( battle, user, isbot );
        battle.OnUserRemoved( user );
    }
    catch (std::runtime_error &except)
    {
    }
}


void ServerEvents::OnAcceptAgreement( const wxString& agreement )
{
    ui().OnAcceptAgreement( agreement );
}


void ServerEvents::OnRing( const wxString& from )
{
    ui().OnRing( from );
}

void ServerEvents::OnServerBroadcast( const wxString& message )
{
	ui().OnServerBroadcast( m_serv, message );
}

void ServerEvents::OnServerMessage( const wxString& message )
{
    ui().OnServerMessage( m_serv, message );
}


void ServerEvents::OnServerMessageBox( const wxString& message )
{
    ui().ShowMessage( _("Server Message"), message );
}


void ServerEvents::OnChannelMessage( const wxString& channel, const wxString& msg )
{
	ui().OnChannelMessage(m_serv.GetChannel(channel), msg );
}


void ServerEvents::OnHostExternalUdpPort( const unsigned int udpport )
{
    if ( !m_serv.GetCurrentBattle() ) return;
    if ( m_serv.GetCurrentBattle()->GetNatType() == NAT_Hole_punching || m_serv.GetCurrentBattle()->GetNatType() == NAT_Fixed_source_ports ) m_serv.GetCurrentBattle()->SetHostPort( udpport );
}


void ServerEvents::OnMyInternalUdpSourcePort( const unsigned int udpport )
{
    if ( !m_serv.GetCurrentBattle() ) return;
    m_serv.GetCurrentBattle()->SetMyInternalUdpSourcePort(udpport);
}


void ServerEvents::OnMyExternalUdpSourcePort( const unsigned int udpport )
{
    if ( !m_serv.GetCurrentBattle() ) return;
    m_serv.GetCurrentBattle()->SetMyExternalUdpSourcePort(udpport);
}

void ServerEvents::OnClientIPPort( const wxString &username, const wxString &ip, unsigned int udpport )
{
    wxLogMessage(_T("OnClientIPPort(%s,%s,%d)"),username.c_str(),ip.c_str(),udpport);
    if ( !m_serv.GetCurrentBattle() )
    {
        wxLogMessage(_T("GetCurrentBattle() returned null"));
        return;
    }
    try
    {
        User &user=m_serv.GetCurrentBattle()->GetUser(STD_STRING(username));

        user.BattleStatus().ip=STD_STRING(ip);
        user.BattleStatus().udpport=udpport;
        wxLogMessage(_T("set to %s %d "),user.BattleStatus().ip.c_str(),user.BattleStatus().udpport);

		if (sett().GetShowIPAddresses()) {
			UiEvents::GetUiEventSender( UiEvents::OnBattleActionEvent ).SendEvent(
					UiEvents::OnBattleActionData( username,wxString::Format(_(" has ip=%s"),ip.c_str()) )
				);
		}

        if (m_serv.GetCurrentBattle()->GetNatType() != NAT_None && (udpport==0))
        {
            // todo: better warning message
            //something.OutputLine( _T(" ** ") + who.GetNick() + _(" does not support nat traversal! ") + GetChatTypeStr() + _T("."), sett().GetChatColorJoinPart(), sett().GetChatFont() );
			UiEvents::GetUiEventSender( UiEvents::OnBattleActionEvent ).SendEvent(
					UiEvents::OnBattleActionData( username,_(" does not really support nat traversal") )
				);

        }
        m_serv.GetCurrentBattle()->CheckBan(user);
    }
    catch (std::runtime_error)
    {
        wxLogMessage(_T("runtime_error inside OnClientIPPort()"));
    }
}


void ServerEvents::OnKickedFromBattle()
{
    customMessageBoxNoModal(SL_MAIN_ICON,_("You were kicked from the battle!"),_("Kicked by Host"));
}


void ServerEvents::OnRedirect( const wxString& address,  unsigned int port, const wxString& CurrentNick, const wxString& CurrentPassword )
{
		wxString name = address + _T(":") + TowxString(port);
    sett().SetServer( name, address, port );
    ui().DoConnect( name, CurrentNick, CurrentPassword );
}


void ServerEvents::AutoCheckCommandSpam( Battle& battle, User& user )
{
    wxString nick = TowxString(user.GetNick());
    MessageSpamCheck info = m_spam_check[nick];
    time_t now = time( 0 );
    if ( info.lastmessage == now ) info.count++;
    else info.count = 0;
    info.lastmessage = now;
    m_spam_check[nick] = info;
    if ( info.count == 7 )
    {
			battle.DoAction(STD_STRING(_T("is autokicking ") + nick + _T(" due to command spam.")));
			battle.KickPlayer( user );
    }
}

void ServerEvents::OnMutelistBegin( const wxString& channel )
{
    mutelistWindow( _("Begin mutelist for ") + channel, wxString::Format( _("%s mutelist"), channel.c_str() ) );
}

void ServerEvents::OnMutelistItem( const wxString& /*unused*/, const wxString& mutee, const wxString& description )
{
    wxString message = mutee;
    wxString desc = description;
    wxString mutetime = GetWordParam( desc );
		long time;
		if ( mutetime == _T("indefinite") ) message << _(" indefinite time remaining");
		else if ( mutetime.ToLong(&time) ) message << wxString::Format( _(" %d minutes remaining"), time/60 + 1 );
		else message << mutetime;
		if ( !desc.IsEmpty() )  message << _T(", ") << desc;
    mutelistWindow( message );
}

void ServerEvents::OnMutelistEnd( const wxString& channel )
{
    mutelistWindow( _("End mutelist for ") + channel );
}

void ServerEvents::OnScriptStart( int battleid )
{
	if ( !m_serv.BattleExists( battleid ) )
	{
			return;
	}
	m_serv.GetBattle( battleid ).ClearScript();
}

void ServerEvents::OnScriptLine( int battleid, const wxString& line )
{
	if ( !m_serv.BattleExists( battleid ) )
	{
			return;
	}
	m_serv.GetBattle( battleid ).AppendScriptLine(STD_STRING(line));
}

void ServerEvents::OnScriptEnd( int battleid )
{
	if ( !m_serv.BattleExists( battleid ) )
	{
			return;
	}
	m_serv.GetBattle( battleid ).GetBattleFromScript( true );
}

void ServerEvents::OnForceJoinBattle(int battleid, const wxString &scriptPW)
{
	IBattle* battle = m_serv.GetCurrentBattle();
    if ( battle != NULL ) {
        m_serv.LeaveBattle(battle->GetID());
	}
    m_serv.JoinBattle( battleid, scriptPW );
    UiEvents::GetStatusEventSender( UiEvents::addStatusMessage ).SendEvent(
            UiEvents::StatusData( _("Automatically moved to new battle"), 1 ) );
}

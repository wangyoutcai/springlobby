/* Author: Tobi Vollebregt */

#ifndef SPRINGLOBBY_HEADERGUARD_AUTOHOST_H
#define SPRINGLOBBY_HEADERGUARD_AUTOHOST_H

//including this header is only really needed for time_t ..
#include <wx/string.h>

class Battle;
class User;
class wxString;

//! @brief Autohost logic
class AutoHost
{
  public:

    AutoHost( Battle& battle );

    void SetEnabled( const bool enabled );
    bool GetEnabled();

    void OnSaidBattle( const wxString& nick, const wxString& msg );
    void OnUserAdded( User& user );
    void OnUserRemoved( User& user );
  private:

    void StartBattle();

    Battle& m_battle;

    bool m_enabled;
    time_t m_lastActionTime;
};

#endif // SPRINGLOBBY_HEADERGUARD_AUTOHOST_H

/**
    This file is part of SpringLobby,
    Copyright (C) 2007-09

    springsettings is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.

    springsettings is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SpringLobby.  If not, see <http://www.gnu.org/licenses/>.
**/


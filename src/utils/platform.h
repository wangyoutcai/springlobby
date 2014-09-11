/* This file is part of the Springlobby (GPL v2 or later), see COPYING */

#ifndef SPRINGLOBBY_HEADERGUARD_PLATFORM_H
#define SPRINGLOBBY_HEADERGUARD_PLATFORM_H

class wxLogWindow;
class wxWindow;
class wxString;
class wxLogChain;

#include <wx/string.h>
#include <wx/arrstr.h>

/**
    let origin be /path/to/some/dir and destination /some/other/path
    this will copy dir (and everything below that recursively to /some/other/path/dir
    \return true if successful
*/
bool CopyDirWithFilebackupRename( wxString from, wxString to, bool overwrite = true, bool backup = true );

//! set new cwd in ctor, reset to old in dtor
class CwdGuard {
    wxString m_old_cwd;
    public:
        CwdGuard( const wxString& new_cwd );
        ~CwdGuard();
};

//! remember pwd in ctor and reset in dtor
class PwdGuard {
	wxString m_old_pwd;
	public:
		PwdGuard( );
		~PwdGuard();
};



/**
  \in Format string with a single %s
  \out wxString with %s replaced with GetAppName()
  **/
wxString IdentityString(const wxString& format, bool lowerCase = false );

int RunProcess(const wxString& cmd, const wxArrayString& params, const bool async = false, const bool root = false);
int BrowseFolder(const wxString& path);
int WaitForExit(int pid);

//!

#endif // SPRINGLOBBY_HEADERGUARD_PLATFORM_H

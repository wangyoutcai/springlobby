/* This file is part of the Springlobby (GPL v2 or later), see COPYING */

#ifndef SPRINGLOBBY_HEADERGUARD_SPRING_H
#define SPRINGLOBBY_HEADERGUARD_SPRING_H

/**
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

DO NOT CHANGE THIS FILE!

this file is deprecated and will be replaced with

lsl/spring/spring.h

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
**/


#include <wx/event.h>

class wxCommandEvent;
class IBattle;
class SpringProcess;
class wxSpringProcess;
class wxString;


class Spring: public wxEvtHandler
{
public:
	explicit Spring();
	~Spring();

	bool IsRunning() const;
	bool Run( IBattle& battle );
	wxString WriteScriptTxt( IBattle& battle ) const;
	/* start engine */
	bool LaunchEngine(wxArrayString& params);
private:
	void OnTerminated( wxCommandEvent& event );
	bool LaunchEngine(const std::string& cmd, wxArrayString& params);

	SpringProcess* m_process;
	bool m_running;

	DECLARE_EVENT_TABLE()
};

Spring& spring();

#endif // SPRINGLOBBY_HEADERGUARD_SPRING_H

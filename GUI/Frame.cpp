/*
 *   Copyright (C) 2010-2015,2018,2021 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "Frame.h"
#include "DestinationsEvent.h"
#include "ChannelsEvent.h"
#include "ReceiveEvent.h"
#include "ErrorEvent.h"
#include "TextEvent.h"
#include "RSSIEvent.h"
#include "Version.h"
#include "App.h"

#include <wx/gbsizer.h>
#include <wx/aboutdlg.h>

enum {
	Choice_Channels = 6000,
	Choice_Destinations,
	Choice_Modules,

	Button_Transmit,

	Slider_Volume,
	Slider_MicGain,
	
	Timer_Timer
};

DEFINE_EVENT_TYPE(CHANNELS_EVENT)
DEFINE_EVENT_TYPE(DESTINATIONS_EVENT)
DEFINE_EVENT_TYPE(RECEIVE_EVENT)
DEFINE_EVENT_TYPE(TEXT_EVENT)
DEFINE_EVENT_TYPE(RSSI_EVENT)
DEFINE_EVENT_TYPE(ERROR_EVENT)

BEGIN_EVENT_TABLE(CFrame, wxFrame)
	EVT_MENU(wxID_EXIT, CFrame::onQuit)
	EVT_MENU(wxID_ABOUT, CFrame::onAbout)

	EVT_CLOSE(CFrame::onClose)

	EVT_TOGGLEBUTTON(Button_Transmit, CFrame::onTransmit)

	EVT_LISTBOX(Choice_Channels,     CFrame::onChannel)
	EVT_LISTBOX(Choice_Destinations, CFrame::onDestination)
	EVT_LISTBOX(Choice_Modules,      CFrame::onModule)

	EVT_COMMAND_SCROLL(Slider_Volume,  CFrame::onVolume)
	EVT_COMMAND_SCROLL(Slider_MicGain, CFrame::onMicGain)

	EVT_TIMER(Timer_Timer, CFrame::onTimer)

	EVT_CUSTOM(CHANNELS_EVENT,     wxID_ANY, CFrame::onChannels)
	EVT_CUSTOM(DESTINATIONS_EVENT, wxID_ANY, CFrame::onDestinations)

	EVT_CUSTOM(RECEIVE_EVENT, wxID_ANY, CFrame::onReceive)
	EVT_CUSTOM(TEXT_EVENT,    wxID_ANY, CFrame::onText)
	EVT_CUSTOM(RSSI_EVENT,    wxID_ANY, CFrame::onRSSI)
	EVT_CUSTOM(ERROR_EVENT,   wxID_ANY, CFrame::onError)
END_EVENT_TABLE()

CFrame::CFrame(CConf& conf, const wxString& title) :
wxFrame(NULL, -1, title),
m_conf(conf),
m_channels(NULL),
m_destinations(NULL),
m_modules(NULL),
m_transmit(NULL),
m_status(NULL),
m_volume(NULL),
m_micGain(NULL),
m_hrdSource(NULL),
m_hrdDestination(NULL),
m_hrdText(NULL),
m_hrdRSSI(NULL),
m_heard(NULL),
m_timer(this, Timer_Timer)
{
	SetMenuBar(createMenuBar());

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	wxPanel* panel = new wxPanel(this);

	wxGridBagSizer* panelSizer = new wxGridBagSizer();

	wxStaticText* channelsLabel = new wxStaticText(panel, -1, wxT("Channel"), wxDefaultPosition, wxSize(LABEL_WIDTH, -1), wxALIGN_RIGHT);
	panelSizer->Add(channelsLabel, wxGBPosition(0, 0), wxDefaultSpan, wxALL, BORDER_SIZE);

	m_channels = new wxChoice(panel, Choice_Channels, wxDefaultPosition, wxSize(CONTROL_WIDTH, CONTROL_HEIGHT));
	panelSizer->Add(m_channels, wxGBPosition(0, 1), wxDefaultSpan, wxALL, BORDER_SIZE);

	wxStaticText* destinationsLabel = new wxStaticText(panel, -1, wxT("Dest"), wxDefaultPosition, wxSize(LABEL_WIDTH, -1), wxALIGN_RIGHT);
	panelSizer->Add(destinationsLabel, wxGBPosition(0, 2), wxDefaultSpan, wxALL, BORDER_SIZE);

	m_destinations = new wxChoice(panel, Choice_Destinations, wxDefaultPosition, wxSize(CONTROL_WIDTH, CONTROL_HEIGHT));
	panelSizer->Add(m_destinations, wxGBPosition(0, 3), wxDefaultSpan, wxALL, BORDER_SIZE);

	wxStaticText* moduleLabel = new wxStaticText(panel, -1, wxT("Module"), wxDefaultPosition, wxSize(LABEL_WIDTH, -1), wxALIGN_RIGHT);
	panelSizer->Add(moduleLabel, wxGBPosition(0, 4), wxDefaultSpan, wxALL, BORDER_SIZE);

	m_modules = new wxChoice(panel, Choice_Modules, wxDefaultPosition, wxSize(CONTROL_WIDTH, CONTROL_HEIGHT));
	m_modules->Append(' ');
	for (wxChar c = wxT('A'); c <= wxT('Z'); c++)
		m_modules->Append(c);
	panelSizer->Add(m_modules, wxGBPosition(0, 5), wxDefaultSpan, wxALL, BORDER_SIZE);

	wxString module = m_conf.getModule();
	m_modules->SetStringSelection(module);

	m_transmit = new wxToggleButton(panel, Button_Transmit, _("Transmit"), wxDefaultPosition, wxSize(CONTROL_WIDTH, -1));
	panelSizer->Add(m_transmit, wxGBPosition(1, 0), wxDefaultSpan, wxALL, BORDER_SIZE);

	m_status = new wxStaticText(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
	panelSizer->Add(m_status, wxGBPosition(1, 1), wxDefaultSpan, wxALL, BORDER_SIZE);

	wxStaticText* volumeLabel = new wxStaticText(panel, -1, wxT("Volume"), wxDefaultPosition, wxSize(LABEL_WIDTH, -1), wxALIGN_RIGHT);
	panelSizer->Add(volumeLabel, wxGBPosition(1, 2), wxDefaultSpan, wxALL, BORDER_SIZE);

	int volume = m_conf.getVolume();
	m_volume = new wxSlider(panel, Slider_Volume, volume, 0, 500, wxDefaultPosition, wxSize(CONTROL_WIDTH, -1));
	panelSizer->Add(m_volume, wxGBPosition(1, 3), wxDefaultSpan, wxALL, BORDER_SIZE);

	wxStaticText* micGainLabel = new wxStaticText(panel, -1, wxT("Mic. Gain"), wxDefaultPosition, wxSize(LABEL_WIDTH, -1), wxALIGN_RIGHT);
	panelSizer->Add(micGainLabel, wxGBPosition(1, 4), wxDefaultSpan, wxALL, BORDER_SIZE);	

	int micGain = m_conf.getMicGain();
	m_micGain = new wxSlider(panel, Slider_MicGain, micGain, 0, 500, wxDefaultPosition, wxSize(CONTROL_WIDTH, -1));
	panelSizer->Add(m_micGain, wxGBPosition(1, 5), wxDefaultSpan, wxALL, BORDER_SIZE);

	wxStaticBoxSizer* info1Sizer = new wxStaticBoxSizer(new wxStaticBox(panel, -1, _("Current")), wxVERTICAL);

	wxGridBagSizer* info2Sizer = new wxGridBagSizer(BORDER_SIZE, BORDER_SIZE);

	wxStaticText* sourceLabel = new wxStaticText(panel, -1, _("Source:"), wxDefaultPosition, wxSize(LABEL_WIDTH, -1), wxALIGN_RIGHT);
	info2Sizer->Add(sourceLabel, wxGBPosition(0, 0), wxDefaultSpan, wxALL, BORDER_SIZE);

	m_hrdSource = new wxStaticText(panel, -1, wxEmptyString, wxDefaultPosition, wxSize(CONTROL_WIDTH, -1));
	info2Sizer->Add(m_hrdSource, wxGBPosition(0, 1), wxDefaultSpan, wxALL, BORDER_SIZE);

	wxStaticText* destLabel = new wxStaticText(panel, -1, _("Dest:"), wxDefaultPosition, wxSize(LABEL_WIDTH, -1), wxALIGN_RIGHT);
	info2Sizer->Add(destLabel, wxGBPosition(0, 2), wxDefaultSpan, wxALL, BORDER_SIZE);

	m_hrdDestination = new wxStaticText(panel, -1, wxEmptyString, wxDefaultPosition, wxSize(CONTROL_WIDTH, -1));
	info2Sizer->Add(m_hrdDestination, wxGBPosition(0, 3), wxDefaultSpan, wxALL, BORDER_SIZE);

	wxStaticText* hrdTextLabel = new wxStaticText(panel, -1, _("Text:"), wxDefaultPosition, wxSize(LABEL_WIDTH, -1), wxALIGN_RIGHT);
	info2Sizer->Add(hrdTextLabel, wxGBPosition(1, 0), wxDefaultSpan, wxALL, BORDER_SIZE);

	m_hrdText = new wxStaticText(panel, -1, wxEmptyString, wxDefaultPosition, wxSize(CONTROL_WIDTH, -1));
	info2Sizer->Add(m_hrdText, wxGBPosition(1, 1), wxDefaultSpan, wxALL, BORDER_SIZE);

	wxStaticText* hrdRSSILabel = new wxStaticText(panel, -1, _("RSSI:"), wxDefaultPosition, wxSize(LABEL_WIDTH, -1), wxALIGN_RIGHT);
	info2Sizer->Add(hrdRSSILabel, wxGBPosition(1, 2), wxDefaultSpan, wxALL, BORDER_SIZE);

	m_hrdRSSI = new wxStaticText(panel, -1, wxEmptyString, wxDefaultPosition, wxSize(LABEL_WIDTH * 3U, -1));
	info2Sizer->Add(m_hrdRSSI, wxGBPosition(1, 3), wxGBSpan(1, 3), wxALL, BORDER_SIZE);

	info1Sizer->Add(info2Sizer);

	panelSizer->Add(info1Sizer, wxGBPosition(2, 0), wxGBSpan(3, 6), wxALL | wxEXPAND, BORDER_SIZE);

	m_heard = new wxListCtrl(panel, -1, wxDefaultPosition, wxSize(HEARD_WIDTH, HEARD_HEIGHT), wxLC_REPORT | wxLC_SINGLE_SEL);
	m_heard->InsertColumn(0L, _("Date/Time"));
	m_heard->SetColumnWidth(0L, DATETIME_WIDTH);
	m_heard->InsertColumn(1L, _("Source"));
	m_heard->SetColumnWidth(1L, CALLSIGN_WIDTH);
	m_heard->InsertColumn(2L, _("Dest"));
	m_heard->SetColumnWidth(2L, MYCALLSIGN_WIDTH);
	m_heard->InsertColumn(3L, _("Text"));
	m_heard->SetColumnWidth(3L, CALLSIGN_WIDTH);
	panelSizer->Add(m_heard, wxGBPosition(5, 0), wxGBSpan(11, 6), wxALL | wxEXPAND, BORDER_SIZE);

	panel->SetSizer(panelSizer);
	panelSizer->SetSizeHints(panel);

	mainSizer->Add(panel);

	SetSizer(mainSizer);
	mainSizer->SetSizeHints(this);

	m_timer.Start(1000);
}

CFrame::~CFrame()
{
}

void CFrame::onTimer(wxTimerEvent& event)
{
	if (m_channels->GetCount() == 0)
		::wxGetApp().getChannels();
	else if (m_destinations->GetCount() == 0)
		::wxGetApp().getDestinations();
	else
		m_timer.Stop();
}

wxMenuBar* CFrame::createMenuBar()
{
	wxMenu* fileMenu = new wxMenu();
	fileMenu->Append(wxID_EXIT, _("Exit"));

	wxMenu* helpMenu = new wxMenu();
	helpMenu->Append(wxID_ABOUT, _("About M17 GUI"));

	wxMenuBar* menuBar = new wxMenuBar();
	menuBar->Append(fileMenu, _("File"));
	menuBar->Append(helpMenu, _("Help"));

	return menuBar;
}

void CFrame::setChannels(const wxArrayString& channels)
{
	CChannelsEvent event(channels, CHANNELS_EVENT);

	AddPendingEvent(event);
}

void CFrame::setDestinations(const wxArrayString& destinations)
{
	CDestinationsEvent event(destinations, DESTINATIONS_EVENT);

	AddPendingEvent(event);
}

void CFrame::showReceive(CReceiveData* data)
{
	wxASSERT(data != NULL);

	CReceiveEvent event(data, RECEIVE_EVENT);

	AddPendingEvent(event);
}

void CFrame::showText(const wxString& text)
{
	CTextEvent event(text, TEXT_EVENT);

	AddPendingEvent(event);
}

void CFrame::showRSSI(int rssi)
{
	CRSSIEvent event(rssi, RSSI_EVENT);

	AddPendingEvent(event);
}

void CFrame::error(const wxString& error)
{
	CErrorEvent event(error, ERROR_EVENT);

	AddPendingEvent(event);
}

void CFrame::onQuit(wxCommandEvent&)
{
	Close(false);
}

void CFrame::onClose(wxCloseEvent& event)
{
	if (!event.CanVeto()) {
		Destroy();
		return;
	}

	int reply = ::wxMessageBox(_("Do you want to exit M17 GUI"), _("M17 GUI"), wxOK | wxCANCEL | wxICON_QUESTION);
	switch (reply) {
		case wxOK:
			Destroy();
			break;
		case wxCANCEL:
			event.Veto();
			break;
	}
}

void CFrame::onAbout(wxCommandEvent&)
{
	wxAboutDialogInfo info;
	info.AddDeveloper(wxT("Jonathan Naylor, G4KLX"));
	info.SetCopyright(wxT("(C) 2010-2021 using GPL v2 or later"));
	info.SetName(APPLICATION_NAME);
	info.SetVersion(VERSION);
	info.SetDescription(_("This program controls the M17 Client daemon."));

	::wxAboutBox(info);
}

void CFrame::onChannel(wxCommandEvent& event)
{
	int n = event.GetSelection();
	if (n == wxNOT_FOUND)
		return;

	wxString channel = m_channels->GetString(n);

	m_conf.setChannel(channel);
	::wxGetApp().setChannel(channel);
}

void CFrame::onDestination(wxCommandEvent& event)
{
	int n = event.GetSelection();
	if (n == wxNOT_FOUND)
		return;

	wxString destination = m_destinations->GetString(n);

	m_conf.setDestination(destination);
}

void CFrame::onModule(wxCommandEvent& event)
{
	int n = event.GetSelection();
	if (n == wxNOT_FOUND)
		return;

	wxString module = m_modules->GetString(n);

	m_conf.setModule(module);
}

void CFrame::onVolume(wxScrollEvent& event)
{
	int volume = event.GetPosition();

	m_conf.setVolume(volume);
	::wxGetApp().setVolume((unsigned int)volume);
}

void CFrame::onMicGain(wxScrollEvent& event)
{
	int micGain = event.GetPosition();

	m_conf.setMicGain(micGain);
	::wxGetApp().setMicGain((unsigned int)micGain);
}

void CFrame::onTransmit(wxCommandEvent& event)
{
	bool tx = event.IsChecked();

	if (tx) {
		int n = m_destinations->GetSelection();
		if (n == wxNOT_FOUND) {
			m_transmit->SetValue(false);
			return;
		}

		wxString destination = m_destinations->GetString(n);

		n = m_modules->GetSelection();
		if (n == wxNOT_FOUND) {
			m_transmit->SetValue(false);
			return;
		}

		wxString module = m_modules->GetString(n);

		if (!module.IsSameAs(wxT(" "))) {
			destination.Append(wxT("           "));
			destination.Truncate(8U);
			destination.Append(module);
		}
		
		::wxGetApp().setDestination(destination);

		::wxGetApp().setTransmit(true);

		m_status->SetBackgroundColour(*wxRED);
		m_status->SetLabel(_("TRANSMIT"));
	} else {
		m_status->SetBackgroundColour(*wxLIGHT_GREY);
		m_status->SetLabel(wxEmptyString);

		::wxGetApp().setTransmit(false);
	}
}

void CFrame::onChannels(wxEvent& event)
{
	CChannelsEvent& chanEvent = dynamic_cast<CChannelsEvent&>(event);

	wxArrayString channels = chanEvent.getChannels();

	m_channels->Clear();
	m_channels->Append(channels);

	wxString channel = m_conf.getChannel();
	m_channels->SetStringSelection(channel);
}

void CFrame::onDestinations(wxEvent& event)
{
	CDestinationsEvent& destEvent = dynamic_cast<CDestinationsEvent&>(event);

	wxArrayString destinations = destEvent.getDestinations();

	m_destinations->Clear();
	m_destinations->Append(destinations);
	m_destinations->Append(wxT("        "));

	wxString destination = m_conf.getDestination();
	m_destinations->SetStringSelection(destination);
}

void CFrame::onReceive(wxEvent& event)
{
	CReceiveEvent& rxEvent = dynamic_cast<CReceiveEvent&>(event);

	CReceiveData* data = rxEvent.getData();
	wxASSERT(data != NULL);

	if (!data->getEnd()) {
		wxDateTime dateTime  = wxDateTime::Now();
		wxString hrdDateTime = dateTime.FormatISODate() + wxT(" ") + dateTime.FormatISOTime();
		m_heard->InsertItem(0L, hrdDateTime);

		wxString source = data->getSource();
		m_hrdSource->SetLabel(source);
		m_heard->SetItem(0L, 1, source);

		wxString destination = data->getDestination();
		m_hrdDestination->SetLabel(destination);
		m_heard->SetItem(0L, 2, destination);

		m_hrdText->SetLabel(wxEmptyString);
		m_hrdRSSI->SetLabel(wxEmptyString);
	} else {
		m_hrdSource->SetLabel(wxEmptyString);
		m_hrdDestination->SetLabel(wxEmptyString);
		m_hrdText->SetLabel(wxEmptyString);
		m_hrdRSSI->SetLabel(wxEmptyString);
	}

	delete data;
}

void CFrame::onText(wxEvent& event)
{
	CTextEvent& txtEvent = dynamic_cast<CTextEvent&>(event);

	wxString text = txtEvent.getText();

	m_hrdText->SetLabel(text);
	m_heard->SetItem(0L, 3, text);
}

void CFrame::onRSSI(wxEvent& event)
{
	CRSSIEvent& rssiEvent = dynamic_cast<CRSSIEvent&>(event);

	int rssi = rssiEvent.getRSSI();

	wxString msg;
	msg.Printf(wxT("%d dBm"), rssi);

	m_hrdRSSI->SetLabel(msg);
}

void CFrame::onError(wxEvent& event)
{
	CErrorEvent& errEvent = dynamic_cast<CErrorEvent&>(event);

	wxString text = errEvent.getText();

	wxMessageDialog dialog(this, text, _("M17 GUI Error"), wxICON_ERROR);
	dialog.ShowModal();
}


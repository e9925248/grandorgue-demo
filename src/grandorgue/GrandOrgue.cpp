/*
 * GrandOrgue - free pipe organ simulator
 *
 * MyOrgan 1.0.6 Codebase - Copyright 2006 Milan Digital Audio LLC
 * MyOrgan is a Trademark of Milan Digital Audio LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */


#include "GrandOrgue.h"
#include "OrganView.h"
#include "OrganDocument.h"
#include "GOrgueEvent.h"
#include "GOrgueLCD.h"
#include "GOrgueSettings.h"
#include "GOrgueSound.h"
#include "GrandOrgueFrame.h"

#include <wx/ipc.h>
#include <wx/snglinst.h>
#include <wx/filesys.h>
#include <wx/fs_zip.h>
#include <wx/splash.h>

#ifdef __WXMAC__
#include <ApplicationServices/ApplicationServices.h>
#endif

#ifdef __WIN32__
#include <windows.h>
#endif

IMPLEMENT_APP(GOrgueApp)

#ifdef __WXMAC__
/* On Mac, filenames do not seem to work for server identifiers. */
#define GO_SERVER_NAME wxT("4096")
#else
#define GO_SERVER_NAME wxT(APP_NAME)
#endif

class stConnection : public wxConnection
{
public:
    stConnection() { }
    ~stConnection() { }
    bool OnExecute(const wxString& topic, wxChar* data, int size, wxIPCFormat format)
    {
        GOrgueApp* app = &::wxGetApp();

        app->GetTopWindow()->Raise();
        if (data[0])
            app->AsyncLoadFile(data);

        return true;
    }
};

class stServer : public wxServer
{
public:
    wxConnectionBase* OnAcceptConnection(const wxString& topic)
    {
        GOrgueApp* app = &::wxGetApp();
        if (!app->GetTopWindow())
            return false;

        if (topic == wxT("open"))
            return new stConnection();
        return 0;
    }
};

class stClient : public wxClient
{
public:
    stClient() { }
    wxConnectionBase* OnMakeConnection() { return new stConnection; }
};

class GOrgueDocManager : public wxDocManager
{
public:
    void OnUpdateFileSave(wxUpdateUIEvent& event);
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(GOrgueDocManager, wxDocManager)
    EVT_UPDATE_UI(wxID_SAVE, GOrgueDocManager::OnUpdateFileSave)
END_EVENT_TABLE();

void GOrgueDocManager::OnUpdateFileSave(wxUpdateUIEvent& event)
{
    wxDocument *doc = GetCurrentDocument();
    event.Enable( doc );
}

GOrgueApp::GOrgueApp() :
   m_Frame(NULL),
   m_locale(),
   m_server(NULL),
   m_Settings(NULL),
   m_soundSystem(NULL),
   m_docManager(NULL),
   single_instance(NULL)
{
}

bool GOrgueApp::OnInit()
{
    m_locale.Init(wxLANGUAGE_DEFAULT);
    m_locale.AddCatalog(wxT("GrandOrgue"));

#ifdef __WXMAC__
    ProcessSerialNumber PSN;
    GetCurrentProcess(&PSN);
    TransformProcessType(&PSN, kProcessTransformToForegroundApplication);
#endif

    single_instance = new wxSingleInstanceChecker(GO_SERVER_NAME);
    if (single_instance->IsAnotherRunning())
    {
        wxLogNull logNull;
        stClient* client = new stClient;
        wxConnectionBase* connection =
            client->MakeConnection
                (wxT("localhost")
                ,GO_SERVER_NAME
                ,wxT("open")
                );
        if (connection)
        {
            connection->Execute(argc > 1 ? argv[1] : wxT(""));
            connection->Disconnect();
            delete connection;
        }
        delete client;
        return false;
    }
    else
    {
        m_server = new stServer;
        if (!m_server->Create(GO_SERVER_NAME))
        {
            wxLogError(_("Failed to create IPC service."));
        }
    }

	SetAppName(wxT(APP_NAME));
	SetClassName(wxT(APP_NAME));
	SetVendorName(_("Our Organ"));

	m_Settings = new GOrgueSettings();
	m_Settings->Load();

	wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED);
	wxFileSystem::AddHandler(new wxZipFSHandler);
	wxImage::AddHandler(new wxJPEGHandler);
	wxImage::AddHandler(new wxGIFHandler);
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxBMPHandler);
	wxImage::AddHandler(new wxICOHandler);
	srand(::wxGetUTCTime());

#ifdef __WIN32__
	SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
#endif
#ifdef linux
	wxLog *logger=new wxLogStream(&std::cout);
	wxLog::SetActiveTarget(logger);
#endif
	m_docManager = new GOrgueDocManager;
	new wxDocTemplate(m_docManager, _("Sample set definition files"), _("*.organ"), wxEmptyString, wxT("organ"), _("Organ Doc"), _("Organ View"), CLASSINFO(OrganDocument), CLASSINFO(OrganView));
	m_docManager->SetMaxDocsOpen(1);

	m_soundSystem = new GOrgueSound(*m_Settings);
	m_Frame = new GOrgueFrame(m_docManager, (wxFrame*)NULL, wxID_ANY, wxT(APP_TITLE), wxDefaultPosition, wxDefaultSize, wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN | wxFULL_REPAINT_ON_RESIZE  | wxMAXIMIZE_BOX | wxRESIZE_BORDER, *m_soundSystem);
	SetTopWindow(m_Frame);
	m_Frame->DoSplash();
	m_Frame->Init();

	if (argc > 1 && argv[1][0])
	{
		AsyncLoadFile(argv[1]);
		argv[1][0] = 0;
		argc = 1;
	}
	GOrgueLCD_Open();

	return true;
}

void GOrgueApp::AsyncLoadFile(wxString what)
{
    if (!m_Frame || !m_docManager)
        return;

    wxFileName fn(what);
    fn.Normalize();
    wxCommandEvent event(wxEVT_LOADFILE, 0);
    event.SetString(fn.GetFullPath());
    m_Frame->GetEventHandler()->AddPendingEvent(event);
}

int GOrgueApp::OnExit()
{
	GOrgueLCD_Close();
	delete m_soundSystem;
	if (m_docManager)
	{
		m_docManager->FileHistorySave(m_Settings->GetConfig());
		delete m_docManager;
	}
	delete m_server;
	delete single_instance;
	delete m_Settings;

	return wxApp::OnExit();
}


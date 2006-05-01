// RCS-ID: $Id: TheApp.cpp,v 1.20 2006/03/03 14:59:45 cecilios Exp $
//--------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2006 Cecilio Salmeron
//
//    This program is free software; you can redistribute it and/or modify it under the
//    terms of the GNU General Public License as published by the Free Software Foundation;
//    either version 2 of the License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but WITHOUT ANY
//    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//    PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along with this
//    program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street,
//    Fifth Floor, Boston, MA  02110-1301, USA.
//
//    For any comment, suggestion or feature request, please contact the manager of
//    the project at cecilios@users.sourceforge.net
//
//-------------------------------------------------------------------------------------
/*! @file TheApp.cpp
    @brief Implementation file for class lmTheApp
    @ingroup app_gui
*/
/*! @class lmTheApp
    @ingroup app_gui
    @brief lmTheApp class represents the application itself

    Derived from wxApp class, lmTheApp class represents the application itself. Takes care of
    implementing the main event loop and to handle events not handled by other objects in
    the application. The two main visible responsibilities are:

    - To set and get application-wide properties;
    - To initiate application processing via wxApp::OnInit;

*/
#ifdef __GNUG__
#pragma implementation "TheApp.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/wx.h"
#include "wx/frame.h"
#include "wx/image.h"
#include "wx/xrc/xmlres.h"          // use the xrc resource system
#include "wx/splash.h"

#if !_DEBUG
extern void __cdecl wxAssert(int n, char const * s,int m,char const *s2,char const *s3);
void __cdecl wxAssert(int n, char const * s,int m,char const *s2,char const *s3) {}
#endif

// verify wxWidgets setup
#if !wxUSE_DOC_VIEW_ARCHITECTURE
#error "You must set wxUSE_DOC_VIEW_ARCHITECTURE to 1 in setup.h!"
#endif

#if !wxUSE_MDI_ARCHITECTURE
#error "You must set wxUSE_MDI_ARCHITECTURE to 1 in setup.h!"
#endif

#if !wxUSE_MENUS
#error "You must set wxUSE_MENUS to 1 in setup.h!"
#endif

#ifdef __WXMSW__
// Detecting and isolating memory leaks with Visual C++
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

// CONFIG: Under Windows, change this to 1 to use wxGenericDragImage
#define wxUSE_GENERIC_DRAGIMAGE 1




#include "TheApp.h"
#include "MainFrame.h"
#include "ScoreDoc.h"
#include "scoreView.h"
#include "AboutDialog.h"
#include "LangChoiceDlg.h"
#include "SplashFrame.h"
#include "ArtProvider.h"

// to save config information into a file
#include "wx/confbase.h"
#include "wx/fileconf.h"
#include "wx/filename.h"
// needed to use the zip file system
#include "wx/fs_zip.h"

//access to user preferences
#include "Preferences.h"

//access to Midi configuration
#include "../sound/MidiManager.h"

//access to global objects
#include "../globals/Paths.h"
#include "../globals/Colors.h"

//access to error's logger
#include "Logger.h"

//to delete the LDP tags table
#include "../ldp_parser/LDPTags.h"


//-------------------------------------------------------------------------------------------
// global variables
//-------------------------------------------------------------------------------------------
lmMainFrame *g_pMainFrame = (lmMainFrame*) NULL;
lmTheApp* g_pTheApp = (lmTheApp*) NULL;

#ifdef _DEBUG
bool g_fReleaseVersion = false;       // to enable/disable debug features
#else
bool g_fReleaseVersion = true;        // to enable/disable debug features
#endif

bool g_fReleaseBehaviour = false;    // This flag is only used to force release behaviour when
                                    // in debug mode, and only for some functions (the ones using this flag)
bool g_fShowDebugLinks = false;        // force to add aditional debug ctrols in exercises.
                                    // only operative in debug mode,


// Global print data, to remember settings during the session
wxPrintData* g_pPrintData = (wxPrintData*) NULL;

// Global page setup data
wxPageSetupData* g_pPaperSetupData = (wxPageSetupData*) NULL;

// Paths names
lmPaths* g_pPaths = (lmPaths*) NULL;

// Colors
lmColors* g_pColors = (lmColors*)NULL;

// Error's logger
lmLogger* g_pLogger = (lmLogger*)NULL;



// Create a new application object: this macro will allow wxWindows to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also declares the accessor function
// wxGetApp() which will return the reference of the right type (i.e. lmTheApp and
// not wxApp)
IMPLEMENT_APP(lmTheApp)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

lmTheApp::lmTheApp(void)
{
    m_pDocManager = (wxDocManager *) NULL;
    g_pTheApp = this;
}

bool lmTheApp::OnInit(void)
{
    // Error reporting and trace
    g_pLogger = new lmLogger();

#ifdef _DEBUG
	// For debugging: send log messages to a file
    FILE* pFile;
    pFile = fopen(_T("LenMus_error_log.txt"), _T("w"));
	wxLog *logger = new wxLogStderr(pFile);
    wxLogChain *logChain = new wxLogChain(logger);
	//delete wxLog::SetActiveTarget(logger);
#endif

    //define trace mask to be known by trace system
    g_pLogger->DefineTraceMask(_T("lmKeySignature"));    
    g_pLogger->DefineTraceMask(_T("lmTheoKeySignCtrol"));
    g_pLogger->DefineTraceMask(_T("lmComposer5"));
    g_pLogger->DefineTraceMask(_T("lmXMLParser"));



    // set information about this application
    SetVendorName(_T("LenMus"));
    SetAppName(_T("LenMus"));

        //
        // Get program directory and set up global paths object
        //

    #ifdef __WXMSW__
    // On Windows, the path to the LenMus program is in argv[0]
    wxString sHomeDir = wxPathOnly(argv[0]);
    #endif
    #ifdef __MACOSX__
    // On Mac OS X, the path to the LenMus program is in argv[0]
    wxString sHomeDir = wxPathOnly(argv[0]);
    #endif
    #ifdef __MACOS9__
    // On Mac OS 9, the initial working directory is the one that
    // contains the program.
    wxString sHomeDir = wxGetCwd();
    #endif
    //! @todo Set path on UNIX systems
    g_pPaths = new lmPaths(sHomeDir);


        //
        // Prepare preferences object
        //

    // set path and name for config file
    wxFileName sCfgFile(sHomeDir, _T("lenmus"), _T("ini") );
    wxFileConfig *pConfig = new wxFileConfig(_T("lenmus"), _T("LenMus"), sCfgFile.GetFullPath(),
            _T("lenmus"), wxCONFIG_USE_LOCAL_FILE );
    wxConfigBase::Set(pConfig);

    // force writing back the default values just in case
    // they're not present in the config file
    pConfig->SetRecordDefaults();

        //
        // Now preferences object and root path are set up. We can proceed to initialize
        // global variables that depend on user preferences.
        //

    // Load user preferences or default values if first run
    InitPreferences();

    // colors
    g_pColors = new lmColors();

        //
        // Set up current language
        //

    wxString lang = g_pPrefs->Read(_T("/Locale/Language"), _T(""));
    if (lang == _T("")) {
        // If language is not set, pop up a dialog to choose language.
        // This will only happen the first time the program is run or if
        // lenmus.ini file is deleted
        lang = ChooseLanguage(NULL);
        g_pPrefs->Write(_T("/Locale/Language"), lang);
    }
    // Now that language code is know we can finish lmPaths initialization
    g_pPaths->SetLanguageCode(lang);
    g_pPaths->LoadUserPreferences();

    // Set up locale object
    m_locale.Init(_T(""), lang, _T(""), true, true);
    m_locale.AddCatalogLookupPathPrefix( g_pPaths->GetLocalePath() );
    m_locale.AddCatalog(_T("lenmus"));
    m_locale.AddCatalog(_T("wxwidgets"));
    m_locale.AddCatalog(_T("wxmidi"));

    // open log file and redirec all loging there
    wxFileName oFilename(g_pPaths->GetTempPath(), _T("DataError"), _T("log"), wxPATH_NATIVE);
    g_pLogger->SetDataErrorTarget(oFilename.GetFullPath());


    // Set the art provider and get current user selected background bitmap
    wxImage::AddHandler( new wxPNGHandler );    //wxJPEGHandler );
    wxArtProvider::PushProvider(new lmArtProvider());
    //m_background = wxArtProvider::GetBitmap(_T("backgrnd"));

    //Include support for zip files
    wxFileSystem::AddHandler(new wxZipFSHandler);

        //
        // XRC resource system
        //

    // Initialize all the XRC handlers.
    wxXmlResource::Get()->InitAllHandlers();

    // get path for xrc resources
    wxString sPath = g_pPaths->GetXrcPath();

        //
        // Load all of the XRC files that will be used. You can put everything
        // into one giant XRC file if you wanted, but then they become more
        // difficult to manage, and harder to reuse in later projects.
        //

    // The score generation settings dialog
    wxFileName oXrcFile(sPath, _T("DlgCfgScoreReading"), _T("xrc"), wxPATH_NATIVE);
    wxXmlResource::Get()->Load( oXrcFile.GetFullPath() );

    // Configuration options: toolbars panel
    oXrcFile = wxFileName(sPath, _T("ToolbarsOptPanel"), _T("xrc"), wxPATH_NATIVE);
    wxXmlResource::Get()->Load( oXrcFile.GetFullPath() );

    // Configuration options: languages panel
    oXrcFile = wxFileName(sPath, _T("LangOptionsPanel"), _T("xrc"), wxPATH_NATIVE);
    wxXmlResource::Get()->Load( oXrcFile.GetFullPath() );

    // Ear Interval exercises: configuration dialog
    oXrcFile = wxFileName(sPath, _T("DlgCfgEarIntervals"), _T("xrc"), wxPATH_NATIVE);
    wxXmlResource::Get()->Load( oXrcFile.GetFullPath() );

    // Theo Interval exercises: configuration dialog
    oXrcFile = wxFileName(sPath, _T("DlgCfgTheoIntervals"), _T("xrc"), wxPATH_NATIVE);
    wxXmlResource::Get()->Load( oXrcFile.GetFullPath() );

    // Pattern Editor dialog
    oXrcFile = wxFileName(sPath, _T("DlgPatternEditor"), _T("xrc"), wxPATH_NATIVE);
    wxXmlResource::Get()->Load( oXrcFile.GetFullPath() );

        //
        // Create document manager and templates
        //

    // Create a document manager
    m_pDocManager = new wxDocManager;

    // Create a template relating score documents to their views
    (void) new wxDocTemplate(m_pDocManager, _T("lmScore"), _T("*.txt"), _T(""), _T("txt"), _T("Music lmScore"), _T("lmScore View"),
          CLASSINFO(lmScoreDocument), CLASSINFO(lmScoreView));

        //
        // Create the main frame window
        //

    bool fMaximized;
    wxRect wndRect;
    GetMainWindowPlacement(&wndRect, &fMaximized);

    g_pMainFrame = new lmMainFrame((wxDocManager *) m_pDocManager, (wxFrame *) NULL,
                      _T("LenMus"),                             // title
                      wxPoint(wndRect.x, wndRect.y),            // origin
                      wxSize(wndRect.width, wndRect.height),    // size
                      wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE, m_locale);

    if (fMaximized)  g_pMainFrame->Maximize(true);

        //
        // Create and show the splash window. The splash can have a non-rectangular
        // shape. The color specified as second parameter of lmSplashFrame creation will
        // be used as the mask color to set the shape
        //

    wxBitmap bitmap = wxArtProvider::GetBitmap(_T("app_splash"), wxART_OTHER);
    int nMilliseconds = 3000;   // at least visible for 3 seconds
	long nSplashTime = (long) time( NULL );
    lmSplashFrame* pSplash = (lmSplashFrame*) NULL;
    if (bitmap.Ok() && bitmap.GetHeight() > 100)
	{
		//the bitmap exists and it is not the error bitmap (height > 100 pixels). Show it
        wxColour colorTransparent(255, 0, 255);   //cyan mask
        pSplash = new lmSplashFrame(bitmap, colorTransparent,
            lmSPLASH_CENTRE_ON_PARENT | lmSPLASH_TIMEOUT,
            nMilliseconds, g_pMainFrame, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxSIMPLE_BORDER|wxSTAY_ON_TOP);
    }
    wxSafeYield();


    // Give the main frame an icon (this is ignored in MDI mode: uses resources)
    g_pMainFrame->SetIcon(wxArtProvider::GetIcon(_T("app_icon"), wxART_OTHER));

    // create global data structures for printer settings
    g_pPrintData = new wxPrintData;
    g_pPaperSetupData = new wxPageSetupDialogData;

    ////Make a menubar and associate it with the frame
    //wxMenuBar* menu_bar = g_pMainFrame->CreateMenuBar(NULL, NULL, false, !g_fReleaseVersion);        //fEdit, fDebug
    //g_pMainFrame->SetMenuBar(menu_bar);

    g_pMainFrame->Show(true);

    SetTopWindow(g_pMainFrame);

    //cursor busy
    ::wxBeginBusyCursor();

        //
        //Main frame created and visible. Proceed with initializations
        //


    //Seed the random-number generator with current time so that
    //the numbers will be different every time we run.
    srand( (unsigned)time( NULL ) );

        //
        //Set up MIDI
        //

    g_pMidi = lmMidiManager::GetInstance();

    //if MIDI not set, force to run the MIDI wizard
    if (!g_pMidi->IsConfigured()) {
	    // first of all, remove splash; otherwise, splahs hides the wizard!
        if (pSplash) {
            pSplash->TerminateSplash();
            pSplash = (lmSplashFrame*) NULL;
        }
        //now we can run the wizard
        ::wxEndBusyCursor();
        g_pMainFrame->DoRunMidiWizard();
        ::wxBeginBusyCursor();
    }

    //Set up MIDI devices
    g_pMidi->SetUpCurrentConfig();

    //program sound for metronome
    g_pMidiOut->ProgramChange(g_pMidi->MtrChannel(), g_pMidi->MtrInstr());

    //enable/disable menu items
    g_pMainFrame->UpdateMenuAndToolbar();

    // all initialization finished.
	// check if the splash window display time is ellapsed and wait if not
	nMilliseconds -= ((long)time( NULL ) - nSplashTime);
	if (nMilliseconds > 0) ::wxMilliSleep( nMilliseconds );
	// allow for splash destruction
    if (pSplash) pSplash->AllowDestroy();

//remove this in debug version to start with nothing displayed
#if !_DEBUG
    //force to show book frame
    wxCommandEvent event;       //it is not used, so not need to initialize it
    g_pMainFrame->OnOpenBook(event);
#endif

    //cursor normal and terminate
    ::wxEndBusyCursor();
    return true;
}

int lmTheApp::OnExit(void)
{
        //
        //Save any other user preferences and values, not saved yet
        //


        //
        // delete all objects used by lmTheApp
        //

    // the Midi configuration and related objects
    delete g_pMidi;

    // the docManager
    delete m_pDocManager;

    // the printer setup data
    delete g_pPrintData;
    delete g_pPaperSetupData;

    // path names
    delete g_pPaths;

    // colors object
    delete g_pColors;

    // the wxConfig object
    delete wxConfigBase::Set((wxConfigBase *) NULL);

    // the error's logger
    delete g_pLogger;

    // the LDP tags table
    lmLdpTagsTable::DeleteInstance();

    return 0;
}

// Set up the default size and position of the main window
void lmTheApp::GetDefaultMainWindowRect(wxRect *defRect)
{
    //Get the size of the screen
    wxRect screenRect;
    wxClientDisplayRect(&screenRect.x, &screenRect.y, &screenRect.width, &screenRect.height);

   //the point that a new window should open at.
   defRect->x = 10;
   defRect->y = 10;

   defRect->width = screenRect.width * 0.95;
   defRect->height = screenRect.height * 0.95;

   //These conditional values assist in improving placement and size
   //of new windows on different platforms.
#ifdef __WXMAC__
   defRect->y += 50;
#endif

}


// Calculate where to place the main window
void lmTheApp::GetMainWindowPlacement(wxRect *frameRect, bool *fMaximized)
{
    *fMaximized = false;        // default: not maximized

    // set the default window size
    wxRect defWndRect;
    GetDefaultMainWindowRect(&defWndRect);

    //Read the values from the config file, or use the defaults
    wxConfigBase *pConfig = wxConfigBase::Get();
    frameRect->SetWidth(pConfig->Read(_T("/MainFrame/Width"), defWndRect.GetWidth()));
    frameRect->SetHeight(pConfig->Read(_T("/MainFrame/Height"), defWndRect.GetHeight()));
    frameRect->SetLeft(pConfig->Read(_T("/MainFrame/Left"), defWndRect.GetLeft() ));
    frameRect->SetTop(pConfig->Read(_T("/MainFrame/Top"), defWndRect.GetTop() ));

    pConfig->Read(_T("/MainFrame/Maximized"), fMaximized);

        //--- Make sure that the Window will be completely visible -------------

    //Get the size of the screen
    wxRect screenRect;
    wxClientDisplayRect(&screenRect.x, &screenRect.y, &screenRect.width, &screenRect.height);

    //If we have hit te bottom of the screen restore default position on the screen
    if( (frameRect->y + frameRect->height > screenRect.y + screenRect.height) ) {
      frameRect->x = defWndRect.x;
      frameRect->y = defWndRect.y;
   }

   //if we have hit the right side of the screen restore default position
    if( (frameRect->x+frameRect->width > screenRect.x+screenRect.width) )  {
      frameRect->x = defWndRect.x;
      frameRect->y = defWndRect.y;
   }

   //Next check if the screen is too small for the default width and height
   if( (frameRect->x+frameRect->width > screenRect.x+screenRect.width) ||
       (frameRect->y+frameRect->height > screenRect.y+screenRect.height) )
   {
      //Resize the main window to fit in the screen
      frameRect->width = screenRect.width-frameRect->x;
      frameRect->height = screenRect.height-frameRect->y;
   }

}

//Pop up a language asking the user to choose a language for the user interface.
//Generally only popped up once, the first time the program is run.
wxString lmTheApp::ChooseLanguage(wxWindow *parent)
{
    lmLangChoiceDlg dlog(parent, -1, _("LenMus First Run"));
    dlog.CentreOnParent();
    dlog.ShowModal();
    return dlog.GetLang();

}

// Update all views of document associated to currentView
void lmTheApp::UpdateCurrentDocViews(void)
{
    lmScoreDocument *doc = (lmScoreDocument *)m_pDocManager->GetCurrentDocument();
    doc->UpdateAllViews();

}

//
// Centralised code for creating a document frame.
// Called from scoreview.cpp, when a view is created.
/*! @todo this is no longer needed here as CreateMenuBar is no longer invoked. So move
    this to EditFrame.cpp
    Chage to create frame maximized?
    set icon ?
*/
lmEditFrame* lmTheApp::CreateProjectFrame(wxDocument* doc, wxView* view)
{
    //Get the size of the main frame client area
    wxInt32 nWidth, nHeight;
    GetMainFrame()->GetClientSize(&nWidth, &nHeight);

    // Create a child frame
    wxSize size(nWidth-20, nHeight-20);
    wxPoint pos(10, 10);
    lmEditFrame* subframe = new lmEditFrame(doc, view, GetMainFrame(), pos, size);

//#ifdef __WXMSW__
//  subframe->SetIcon(wxString(_T("chart")));
//#endif
//#ifdef __X__
//  subframe->SetIcon(wxIcon(_T("doc.xbm")));
//#endif

    //Removed: children frames will not have its own menu
    //Make a menubar
    //wxMenuBar* menu_bar = g_pMainFrame->CreateMenuBar(doc, view, true, !g_fReleaseVersion);        //fEdit, fDebug
    //subframe->SetMenuBar(menu_bar);   // Associate it with the frame

    return subframe;
}



//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
// Global functions
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------


lmMainFrame* GetMainFrame(void)
{
    return g_pMainFrame;
}

int lmToLogicalUnits(int nValue, lmUnits nUnits)
{
    switch(nUnits) {
        case lmMICRONS:         return (nValue / 100);      break;
        case lmMILLIMETERS:     return (nValue * 10);       break;
        case lmCENTIMETERS:     return (nValue * 100);      break;
        case lmINCHES:          return (nValue * 254);      break;
        default:
            wxASSERT(false);
            return 10;
    }

}

int lmToLogicalUnits(double rValue, lmUnits nUnits)
{
    switch(nUnits) {
        case lmMICRONS:         return (int)(rValue / 100);      break;
        case lmMILLIMETERS:     return (int)(rValue * 10);       break;
        case lmCENTIMETERS:     return (int)(rValue * 100);      break;
        case lmINCHES:          return (int)(rValue * 254);      break;
        default:
            wxASSERT(false);
            return 10;
    }

}






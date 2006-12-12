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
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "Paths.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "Paths.h"
#include "../app/TheApp.h"

// the config object
extern wxConfigBase *g_pPrefs;


//-------------------------------------------------------------------------------------------
// lmPaths implementation
//-------------------------------------------------------------------------------------------

lmPaths::lmPaths(wxString sBinPath)
{
    //Receives the full path to the LenMus executable folder (/bin) and
    //extracts the root path
    m_sBin = sBinPath;
    m_root.Assign(sBinPath, _T(""), wxPATH_NATIVE);
    m_root.RemoveLastDir();
    //wxMessageBox(wxString::Format(_T("[lmPaths::lmPaths] Bin folder = %s\n Root folder = %s"),
    //    sBinPath, m_root.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) ));

}

lmPaths::~lmPaths()
{
    SaveUserPreferences();
}


void lmPaths::SetLanguageCode(wxString sLangCode)
{
    //
    // Lang code has changed. It is necessary to rebuild paths depending on language
    //
    // IMPORTANT: When this method is invoked wxLocale object is not
    //            yet initialized. DO NOT USE LANGUAGE DEPENDENT STRINGS HERE
    //
    m_sLangCode = sLangCode;
    wxFileName oLocalePath(m_sLocaleRoot, _T(""), wxPATH_NATIVE);
    oLocalePath.AppendDir(m_sLangCode);
    m_sLocale = oLocalePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    wxFileName oBooksPath = m_root;
    oBooksPath.AppendDir(_T("books"));
    oBooksPath.AppendDir(m_sLangCode);
    m_sBooks = oBooksPath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    wxFileName oHelpPath = oLocalePath;
    oHelpPath.AppendDir(_T("help"));
    m_sHelp = oHelpPath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    // When changing language a flag was stored so that at next run the program must
    // clean the temp folder. Otherwise, as books have the same names in English and
    // in Spanish, the new language .hcc and hhk files will not be properly loaded.
    // Here I test this flag and if true, remove all files in temp folder
    bool fClearTemp;
    g_pPrefs->Read(_T("/Locale/LanguageChanged"), &fClearTemp, false );
    if (fClearTemp) {
        wxString sFile = wxFindFirstFile(m_sTemp);
        while ( !sFile.empty() ) {
            if (!::wxRemoveFile(sFile)) {
                wxLogMessage(_T("[lmPaths::LoadUserPreferences] Error deleting %s"), sFile );
            }
            sFile = wxFindNextFile();
        }
        //reset flag
        fClearTemp = false;
        g_pPrefs->Write(_T("/Locale/LanguageChanged"), fClearTemp);
    }

}


void lmPaths::LoadUserPreferences()
{
    /*
    load settings form user congiguration data or default values
    */

    wxFileName path;
    wxString sDefault;

    //! @todo There must be a bug somewhere but Read does not work
    //bool fOK = g_pPrefs->Read(_T("/Paths/Locale"), &m_sLocaleRoot);
    //if (m_sLocaleRoot == _T("")) {
        path = m_root;
        path.AppendDir(_T("locale"));
        m_sLocaleRoot = path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
    //}

    path = m_root;
    path.AppendDir(_T("scores"));
    path.AppendDir(_T("samples"));
    m_sScores = path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
    g_pPrefs->Read(_T("/Paths/Scores"), &m_sScores);   


    path = m_root;
    path.AppendDir(_T("temp"));
    m_sTemp = path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    path = m_root;
    path.AppendDir(_T("data"));
    m_sData = path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    path = m_root;
    path.AppendDir(_T("xrc"));
    m_sXrc = path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    path = m_root;
    path.AppendDir(_T("res"));
    path.AppendDir(_T("icons"));
    m_sImages = path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    path = m_root;
    path.AppendDir(_T("res"));
    path.AppendDir(_T("sounds"));
    m_sSounds = path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    path = m_root;
    path.AppendDir(_T("res"));
    path.AppendDir(_T("vbooks"));
    m_sVBookImages = path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

	//create temp folder if it does not exist. Otherwise the program will
    //fail when the user tries to open an eMusicBook
    if (!::wxDirExists(m_sTemp)) {
        ::wxMkDir(m_sTemp);
    }
    else {
        // When changing language a flag was stored so that at next run the program must
        // clean the temp folder. Otherwise, as books have the same names in English and
        // in Spanish, the new language .hcc and hhk files will not be properly loaded.
        // Here I test this flag and if true, remove all files in temp folder
        bool fClearTemp;
        g_pPrefs->Read(_T("/Locale/LanguageChanged"), &fClearTemp, false );
        if (fClearTemp) {
            wxString sFile = wxFindFirstFile(m_sTemp);
            while ( !sFile.empty() ) {
                if (!::wxRemoveFile(sFile)) {
                    wxLogMessage(_T("[lmPaths::LoadUserPreferences] Error deleting %s"), sFile );
                }
                sFile = wxFindNextFile();
            }
            //reset flag
            fClearTemp = false;
            g_pPrefs->Write(_T("/Locale/LanguageChanged"), fClearTemp);
        }
    }

    //SetLocalePathNames(m_sLangCode);
}

//! save path settings in user configuration data
void lmPaths::SaveUserPreferences()
{
    g_pPrefs->Write(_T("/Paths/Locale"), m_sLocaleRoot);   
    g_pPrefs->Write(_T("/Paths/Scores"), m_sScores);   
    g_pPrefs->Write(_T("/Paths/Temp"), m_sTemp);   
    g_pPrefs->Write(_T("/Paths/Data"), m_sData);   
    g_pPrefs->Write(_T("/Paths/Xrc"), m_sXrc);   
    g_pPrefs->Write(_T("/Paths/Images"), m_sImages);
    g_pPrefs->Write(_T("/Paths/Sounds"), m_sSounds);
    g_pPrefs->Write(_T("/Paths/VBookImages"), m_sVBookImages);

    // bin path is nor user configurable
    //g_pPrefs->Write(_T("/Paths/Bin"), m_sBin);   

}

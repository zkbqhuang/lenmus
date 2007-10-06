//--------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2007 Cecilio Salmeron
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "MusicReadingConstrains.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "MusicReadingConstrains.h"
#include "Generators.h"

// the config object
extern wxConfigBase *g_pPrefs;

//-------------------------------------------------------------------------------------------
// implementation of lmMusicReadingConstrains
//-------------------------------------------------------------------------------------------


lmMusicReadingConstrains::lmMusicReadingConstrains(wxString sSection)
    : lmExerciseOptions(sSection)
{
    //default values for control options
    fPlayCtrol = false;
    fSolfaCtrol = false;
    fBorder = false;
    fSettingsLink = false;
    sPlayLabel = _("Play");
    sStopPlayLabel = _("Stop");
    sSolfaLabel = _("Read");
    sStopSolfaLabel = _("Stop");
    m_pScoreConstrains = (lmScoreConstrains*)NULL;

}

lmMusicReadingConstrains::~lmMusicReadingConstrains()
{
    if (m_pScoreConstrains) delete m_pScoreConstrains;
}


void lmMusicReadingConstrains::SetLabels(wxString& sLabel, wxString* pStart, wxString* pStop)
{
    //find the bar
    int i = sLabel.Find(_T("|"));
    if (i != -1) {
        if (i > 1) *pStart = sLabel.substr(0, i-1);
        if (i < (int)sLabel.length()-1) *pStop = sLabel.substr(i+1);
    }
    else {
         *pStart = sLabel;
    }

}
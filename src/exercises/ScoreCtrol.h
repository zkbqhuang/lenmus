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

#ifndef __SCORECTROL_H__        //to avoid nested includes
#define __SCORECTROL_H__

#ifdef __GNUG__
#pragma interface "ScoreCtrol.cpp"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "Constrains.h"
#include "../score/Score.h"
#include "ScoreAuxCtrol.h"
#include "UrlAuxCtrol.h"
#include "../sound/SoundEvents.h"
#include "ExerciseCtrol.h"


class lmScoreCtrol : public lmEBookCtrol
{
   DECLARE_DYNAMIC_CLASS(lmScoreCtrol)

public:

    // constructor and destructor
    lmScoreCtrol(wxWindow* parent, wxWindowID id, lmScore* pScore,
              lmScoreCtrolOptions* pOptions,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize, int style = 0);

    ~lmScoreCtrol();

    // event handlers
    void OnSolfa(wxCommandEvent& event);
    void OnPlayMeasure(wxCommandEvent& event);

    // event handlers related to debugging
    void OnDebugShowSourceScore(wxCommandEvent& event);
    void OnDebugDumpScore(wxCommandEvent& event);
    void OnDebugShowMidiEvents(wxCommandEvent& event);

    // event handlers related with playing a score
    void OnEndOfPlay(lmEndOfPlayEvent& WXUNUSED(event));

protected:
    //implementation of virtual pure methods
    void InitializeStrings() {}   
    wxDialog* GetSettingsDlg() { return (wxDialog*)NULL; }
    void Play() { DoPlay(ePM_NormalInstrument, m_pPlayLink); }
    void StopSounds();
    void OnSettingsChanged() {}
    void CreateControls();

private:
    void DoPlay(EPlayMode nPlayMode, lmUrlAuxCtrol* pLink, int nMeasure=0);

        // member variables

    lmScoreAuxCtrol*        m_pScoreCtrol;
    lmScoreCtrolOptions*    m_pOptions;
    lmScore*                m_pScore;       //the score to display
    wxSize                  m_nScoreSize;

    //to control play back
    lmUrlAuxCtrol*      m_CurPlayLink;      // link that is being played back
    bool                m_fPlaying;         ///< the score is being played back

    //controls
    lmUrlAuxCtrol*      m_pPlayLink;
    lmUrlAuxCtrol*      m_pSolfaLink;
    lmUrlAuxCtrol*      m_pMeasureLink[10];
    /*! @limit Max number of 'Play measure' links is limited to ten. */

    DECLARE_EVENT_TABLE()
};



#endif  // __SCORECTROL_H__

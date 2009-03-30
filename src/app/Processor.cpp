//--------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2009 LenMus project
//
//    This program is free software; you can redistribute it and/or modify it under the
//    terms of the GNU General Public License as published by the Free Software Foundation,
//    either version 3 of the License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but WITHOUT ANY
//    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//    PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along with this
//    program. If not, see <http://www.gnu.org/licenses/>.
//
//    For any comment, suggestion or feature request, please contact the manager of
//    the project at cecilios@users.sourceforge.net
//
//-------------------------------------------------------------------------------------

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "Processor.h"
#endif

// for (compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#else
    #include <wx/sizer.h>
    #include <wx/panel.h>
#endif


#include "Processor.h"

// access to main frame
#include "../app/MainFrame.h"
extern lmMainFrame* GetMainFrame();

#include "toolbox/ToolsBox.h"
#include "ScoreDoc.h"
#include "../exercises/auxctrols/UrlAuxCtrol.h"
#include "../score/Score.h"
#include "../score/VStaff.h"

//access to error's logger
#include "../app/Logger.h"
extern lmLogger* g_pLogger;
#include "../auxmusic/ChordManager.h"

//-------------------------------------------------------------------------------------------
// Implementation of class lmScoreProcessor
//-------------------------------------------------------------------------------------------

//IDs for controls
const int lmID_DO_PROCESS = wxNewId();
const int lmID_UNDO_PROCESS = wxNewId();

lmScoreProcessor::lmScoreProcessor()
    : wxEvtHandler()
{
}

lmScoreProcessor::~lmScoreProcessor()
{
}

void lmScoreProcessor::DoProcess()
{
    //get the score
    lmMainFrame* pMainFrame = GetMainFrame();
    lmScoreDocument* pDoc = pMainFrame->GetActiveDoc();
    if (!pDoc) return;
    lmScore* pScore = pDoc->GetScore();
    if (!pScore) return;

    if (ProcessScore(pScore))
    {
        //if changes done in the score, update views
	    pDoc->Modify(true);
        pDoc->UpdateAllViews(true, new lmUpdateHint() );

        //enable undo link
        m_pUndoLink->Enable(true);
    }
}

void lmScoreProcessor::UndoProcess()
{
    //get the score
    lmMainFrame* pMainFrame = GetMainFrame();
    lmScoreDocument* pDoc = pMainFrame->GetActiveDoc();
    if (!pDoc) return;
    lmScore* pScore = pDoc->GetScore();
    if (!pScore) return;

    if (UndoChanges(pScore))
    {
        //if changes done in the score, update views
	    pDoc->Modify(true);
        pDoc->UpdateAllViews(true, new lmUpdateHint() );
    }

    //disable undo link
    m_pUndoLink->Enable(false);
}

bool lmScoreProcessor::CreateToolsPanel(wxString sTitle, wxString sDoLink,
                                       wxString sUndoLink)
{
    lmMainFrame* pMainFrame = GetMainFrame();
    lmToolBox* pToolbox = pMainFrame->GetActiveToolBox();
    if (!pToolbox) return false;        //false -> failure

    m_pToolsPanel = new wxPanel(pToolbox, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxSUNKEN_BORDER|wxTAB_TRAVERSAL );

	m_pMainSizer = new wxBoxSizer( wxVERTICAL );

    AddStandardLinks(m_pMainSizer, sDoLink, sUndoLink);

    return true;
}
void lmScoreProcessor::AddStandardLinks(wxBoxSizer* pSizer, wxString sDoLink,
                                        wxString sUndoLink)
{
    // 'Do process' link
    if (sDoLink != wxEmptyString)
    {
        m_pDoLink = new lmUrlAuxCtrol(m_pToolsPanel, lmID_DO_PROCESS, 1.0, sDoLink);
        pSizer->Add(m_pDoLink, 0, wxALL|wxEXPAND, 5);
        m_pToolsPanel->Connect(lmID_DO_PROCESS, lmEVT_URL_CLICK,
                               wxObjectEventFunction(&lmScoreProcessor::DoProcess),
                               NULL, this);
    }

    // 'Do process' link
    if (sDoLink != wxEmptyString)
    {
        m_pUndoLink = new lmUrlAuxCtrol(m_pToolsPanel, lmID_UNDO_PROCESS, 1.0, sUndoLink);
        pSizer->Add(m_pUndoLink, 0, wxALL|wxEXPAND, 5);
        m_pToolsPanel->Connect(lmID_UNDO_PROCESS, lmEVT_URL_CLICK,
                               wxObjectEventFunction(&lmScoreProcessor::UndoProcess),
                               NULL, this);
    }
}

void lmScoreProcessor::RealizePanel()
{
    m_pToolsPanel->SetSizer( m_pMainSizer );
	m_pToolsPanel->Layout();
	m_pMainSizer->Fit( m_pToolsPanel );

    lmMainFrame* pMainFrame = GetMainFrame();
    lmToolBox* pToolbox = pMainFrame->GetActiveToolBox();
    if (pToolbox)
        pToolbox->AddSpecialTools(m_pToolsPanel, this);

    //disable undo link
    m_pUndoLink->Enable(false);
}

//-------------------------------------------------------------------------------------------
// Implementation of class lmHarmonyProcessor
//-------------------------------------------------------------------------------------------

lmHarmonyProcessor::lmHarmonyProcessor()
    : lmScoreProcessor()
{
}

lmHarmonyProcessor::~lmHarmonyProcessor()
{
    //delete all markup pairs
    std::list<lmMarkup*>::iterator it = m_markup.begin();
    while (it != m_markup.end())
    {
        lmMarkup* pError = *it;
        m_markup.erase(it++);
        delete pError;
    }

    assert(nNumChords<lmMAX_NUM_CHORDS);
    for (int i = 0; i <nNumChords; i++)
        delete tChordDescriptor[i].pChord;
}

bool lmHarmonyProcessor::SetTools()
{
    //Create a panel with the exercise buttons, and place it on the ToolBox
    //Returns false if failure.

    if (!CreateToolsPanel(_("Harmony exercise"),
                          _("Check harmony"),
                          _("Clear errors")) )
        return false;

    //No more tools to add. Show panel in toolbox
    RealizePanel();
    return true;
}

#ifdef __WXDEBUG__
void  lmHarmonyProcessor::TestDisplay(lmScore* pScore, lmStaffObj* cpSO, wxColour colour)
{
    // Remember: all 'y' positions are relative to top line (5th line of
    //   first staff). 'x' positions are relative to current object position.
     lmTenths nxStart = 0;  // fijo; relativo al usuario
     static lmTenths nyStart = 40;  // relativo a top line; positivo: abajo
     lmTenths nxEnd = -100; // fijo
     static lmTenths nyEnd = -120;  // negativo: arriba. Se baja en cada uso

     lmTenths nTxPos = nxEnd + 10;
     lmTenths nTyPos = nyEnd + 10;

    wxString sText;

    //define the font to use for texts
    lmFontInfo tFont = {_("Comic Sans MS"), 6, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL};
    lmTextStyle* pStyle = pScore->GetStyleName(tFont);

    cpSO->SetColour(colour);

    // Remember: all 'y' positions are relative to top line (5th line of
    //   first staff). 'x' positions are relative to current object position.
    // Remember: lmScoreLine( xStart, yStart, xEnd,  yEnd, tWidth, nColor)
    lmScoreLine* pLine = new lmScoreLine(nxStart, nyStart, nxEnd, nyEnd, 2, colour);
    cpSO->AttachAuxObj(pLine);
    lmMarkup* pError = new lmMarkup(cpSO, pLine);
    m_markup.push_back(pError);

    // Text at top of the line
    sText =   wxString::Format(_T("[xS:%f, yS:%f, xE:%f,yE:%f  xt%f yt%f ]")  
        ,nxStart, nyStart, nxEnd, nyEnd,  nTxPos, nTyPos);
    lmTextItem* pText = new lmTextItem(sText, lmHALIGN_DEFAULT, pStyle);
    cpSO->AttachAuxObj(pText);


    pText->SetUserLocation(nTxPos, nTyPos);
    pError = new lmMarkup(cpSO, pText);
    m_markup.push_back(pError);

    nyEnd +=15;

}
#endif

// TODO: ESTO ES SOLO PROVISIONAL!!!!!!!!!!!!!!!!!!!!!!
static const int lmDisXstart = 0;
static const int lmDisXend = -200;
static const int lmDisYstart = 40;
static const int lmDisYend = -120;
void  lmHarmonyProcessor::DisplayChordInfo(lmScore* pScore, int nNumChordNotes
                                       , lmChordDescriptor*  pChordDsct
                                           , wxColour colour, wxString &sText, bool reset)
{
    // Remember: all 'y' positions are relative to top line (5th line of
    //   first staff). 'x' positions are relative to current object position.
    lmTenths nxStart = lmDisXstart;  // fijo; relativo al usuario
    lmTenths nxEnd = lmDisXend; // fijo
    static lmTenths nyStart = lmDisYstart;  // relativo a top line; positivo: abajo
    static lmTenths nyEnd = lmDisYend;  // negativo: arriba. Se baja en cada uso

    lmTenths nTxPos = nxEnd + 10;
    lmTenths nTyPos = nyEnd + 10;
    if ( reset )
    {
        // only reset
        nyStart = lmDisYstart;  
        nyEnd = lmDisYend; 
        return;
    }

    //define the font to use for texts
    lmFontInfo tFont = {_("Comic Sans MS"), 6, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL};
    lmTextStyle* pStyle = pScore->GetStyleName(tFont);

    // Display chord info in score with a line and text
    assert(nNumChordNotes > 0);
    assert(nNumChordNotes < 20);
    assert(pChordDsct != NULL);

    for (int i = 0; i<nNumChordNotes; i++)
    {
        assert(pChordDsct->pChordNotes[i] != NULL);
        pChordDsct->pChordNotes[i]->SetColour(colour);
    }

    // Green line pointing to the chord
    // Remember: all 'y' positions are relative to top line (5th line of
    //   first staff). 'x' positions are relative to current object position.
    lmStaffObj* cpSO =pChordDsct->pChordNotes[nNumChordNotes-1];
    // Remember: lmScoreLine( xStart, yStart, xEnd,  yEnd, tWidth, nColor)
    lmScoreLine* pLine = new lmScoreLine(nxStart, nyStart, nxEnd, nyEnd, 2, colour);
    cpSO->AttachAuxObj(pLine);
    lmMarkup* pError = new lmMarkup(cpSO, pLine);
    m_markup.push_back(pError);

    // Text at top of the line
    lmTextItem* pText = new lmTextItem(sText, lmHALIGN_DEFAULT, pStyle);
    cpSO->AttachAuxObj(pText);
    pText->SetUserLocation(nTxPos, nTyPos);
    pError = new lmMarkup(cpSO, pText);
    m_markup.push_back(pError);

    nyEnd +=15; // y positions is NOT relative to use; change each time

}
// All chord processing:
//  analysis of chord notes and intervals
//  chord creation
//  results: display messages...
//  TODO: analyze harmonic progression...
bool lmHarmonyProcessor::ProccessChord(lmScore* pScore, int nNumChordNotes
                                       , lmChordDescriptor* ptChordDescriptor, wxString &sStatusStr)
{
    bool fOk = false;
    bool fCanBeCreated = false;

    lmChordInfo tChordInfo;
    tChordInfo.Initalize();

    assert(ptChordDescriptor != NULL);
    assert(ptChordDescriptor->pChordNotes != NULL);

    // Create Chord
    fCanBeCreated = TryChordCreation(nNumChordNotes, ptChordDescriptor->pChordNotes, &tChordInfo,  sStatusStr);
    
    lmNote* pChordBaseNote = ptChordDescriptor->pChordNotes[0]; //TODO @@@ (confirmar que aun con inversiones la primera nota es la fundamental)

    wxColour colour;
    int nLastChord = nNumChords;


    if (fCanBeCreated)
    {
        ptChordDescriptor[nNumChords].pChord = new lmChordManager(pChordBaseNote, tChordInfo);
        sStatusStr = ptChordDescriptor[nNumChords].pChord->ToString();
        nNumChords++;
        colour = *wxGREEN;
        fOk = true;
    }
    else
    {
        // TODO: mejorar @@@@@@@ ahora aunque no sea valido, lo creamos solo para guardar los punteros a las notas
 //@@@TODO: quitar??       ptChordDescriptor[nNumChords].pChord = new lmChordManager(pChordBaseNote, tChordInfo);
        nNumChords++;
        colour = *wxRED;
        fOk = false;
    }

    wxLogMessage(sStatusStr);

    DisplayChordInfo(pScore, nNumChordNotes, ptChordDescriptor, colour, sStatusStr);

    return fOk;

}

bool lmHarmonyProcessor::ProcessScore(lmScore* pScore)
{
    //This method checks the score and show errors
    //Returns true if it has done any change in the score

    //As an example, I will put red and a green lines pointing to fourth and
    //sixth notes, respectively, and add some texts
    bool fScoreModified = false;
    nNumChords = 0;
    int nBarCount = 0;

    //Get the instrument
    lmInstrument* pInstr = pScore->GetFirstInstrument();
    lmVStaff* pVStaff = pInstr->GetVStaff();

	lmNote* pCurrentNote;
    int nCurrentNotePos = -2;
    int nChordNotePos = -2;
    int nNumChordNotes = 0;
    wxString sStatusStr;

    // TODO: QUITAR; PROVISIONAL
    //  resetear el control de las indicaciones....
    DisplayChordInfo(pScore, nNumChordNotes, &tChordDescriptor[0], *wxGREEN, sStatusStr, true);

    // Loop to process notes/rests in first staff of first instrument
    int nNote = 0;
    lmSOIterator* pIter = pVStaff->CreateIterator();
    while(!pIter->EndOfCollection())
    {
        lmStaffObj* pSO = pIter->GetCurrent();
        if (pSO->IsNoteRest() && ((lmNoteRest*)pSO)->IsNote())
        {
#ifdef __WXDEBUG__
  //          TestDisplay(pScore, pSO, *wxGREEN); //@@@@@@@@@@@
#endif
            // It is a note. Count it
            ++nNote;

			pCurrentNote  = (lmNote*) pSO;
			nCurrentNotePos  = pCurrentNote->GetBeatPosition();
            wxLogMessage(_T("[ProcessScore] note %d: pitch: %d, pos: %d")
                , nNote, pCurrentNote->GetFPitch(), nCurrentNotePos);
            wxLogMessage(_T("  LDP:%s")
                ,  pCurrentNote->SourceLDP(0).c_str());

            // Notes with beat position of lmNOT_ON_BEAT are ignored
            // New beat position means new chord

            if (  nCurrentNotePos  != lmNOT_ON_BEAT)
            {
                // Note candidate to chord

                // To belong the same chord, the note must be in the same beat position
                //  and in the same bar, since 'beat position' is relative to the bar
                if (  nCurrentNotePos != nChordNotePos || nBarCount > 0)
                {
                   // Chord initialization
                    wxLogMessage(_T("Chords:%d,  New chord because: currentPos %d, prev: %d, nBarCount:%d, nNumChordNotes:%d")
                        , nNumChords ,  nCurrentNotePos,  nChordNotePos, nBarCount , nNumChordNotes);

                    // Different beat position: new chord
                    // If at least 3 notes: create chord
                    if (nNumChordNotes > 2)
                    {
                        wxLogMessage(_T("1ProccessChord:%d,  nNumChordNotes: %d")
                            , nNumChords ,  nNumChordNotes);
                        //@@@ DEBUG
                        for (int i = 0; i < nNumChordNotes; i++)
                        {
                            if (tChordDescriptor[nNumChords].pChordNotes[i] != NULL)
                            {
                                wxLogMessage(_T("  CHORD NOTE[%d] : %s")
                                ,i, tChordDescriptor[nNumChords].pChordNotes[i]->SourceLDP(0).c_str() );
                            }
                            else
                            {
                                wxLogMessage(_T("  CHORD NOTE[%d] : NULL!!") ,i );
                            }
                        }

                        wxLogMessage(_T("ProccessChord %d, num CHORD NOTES: %d")
                        , nNumChords, nNumChordNotes);
                        bool fChordOk = ProccessChord(pScore, nNumChordNotes, &tChordDescriptor[nNumChords], sStatusStr);
                    }

                    fScoreModified = true; // repaint

                    // Reset all
                    nNumChordNotes = 0;
        			tChordDescriptor[nNumChords].pChordNotes[0] = pCurrentNote;
                    nChordNotePos = nCurrentNotePos;
                    nNumChordNotes++;
                    wxLogMessage(_T("[ProcessScore] Chord:%d, First chord note, pitch: %d, pos: %d")
                        , nNumChords, pCurrentNote->GetFPitch(), nNumChordNotes);
                    nBarCount = 0;

                } //  if (  nCurrentNotePos != nNumChordNotes )
                // else: in the same beat then add to chord
                else
                {
                    tChordDescriptor[nNumChords].pChordNotes[nNumChordNotes++] = pCurrentNote;
                    wxLogMessage(_T("[ProcessScore] Chord:%d, NEW CHORD NOTE, nNumChordNotes: %d, pitch: %d")
                        , nNumChords ,nNumChordNotes, pCurrentNote->GetFPitch() );
                }


            }
            //  else [ currentNotePos == lmNOT_ON_BEAT]: ignore
        }
        else
        {
        // else [ not IsNote]: ignore
        //  but count barlines
            // bar count > 1 means that equal 'beat position' are no longer valid
            if (pSO->IsBarline()) 
                nBarCount++;
        }

        pIter->MoveNext();

    } // while
    delete pIter;       //Do not forget this. We are not using smart pointers!

    // Last chord?
    if (nNumChordNotes > 2)
    {
        wxLogMessage(_T("2ProccessChord:%d,  nNumChordNotes: %d")
           , nNumChords ,  nNumChordNotes);
        bool fChordOk = ProccessChord(pScore, nNumChordNotes, &tChordDescriptor[nNumChords], sStatusStr);
        fScoreModified = true;
    }

    return fScoreModified;      //true -> score modified
}

bool lmHarmonyProcessor::UndoChanges(lmScore* pScore)
{
    //This method removes all error markup from the score

    //TODO:
    //The base class should provide an standard functionality for doing this.
    //For example, it should maintain a list of changes and it will undo all these
    //changes. This way, derived classes have nothing to implement, unless an
    //special behaviour is desired.

    //This code is just an example to undo what we did at ProcessScore()

    WXUNUSED(pScore);
    std::list<lmMarkup*>::iterator it = m_markup.begin();
    while (it != m_markup.end())
    {
        //undo this markup
        lmMarkup* pError = *it;
        lmStaffObj* pParent = pError->first;
        lmAuxObj* pAttachment = pError->second;
	    pParent->DetachAuxObj(pAttachment);
	    pParent->SetColour(*wxBLACK);
        delete pAttachment;

        //remove used element and move to next item in list
        m_markup.erase(it++);
        delete pError;
    }
    return true;
}
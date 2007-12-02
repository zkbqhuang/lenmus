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
#pragma implementation "Notation.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif


#include "Score.h"
#include "Notation.h"


lmNotation::lmNotation(lmVStaff* pVStaff, int nStaff, bool fVisible, bool fIsDraggable)
    : lmStaffObj(pVStaff, eSFOT_Notation, pVStaff, nStaff, fVisible, fIsDraggable)
{
}


//-----------------------------------------------------------------------------------------
// lmSpacer implementation
//-----------------------------------------------------------------------------------------

lmSpacer::lmSpacer(lmVStaff* pStaff, lmTenths nWidth, int nStaff)
    : lmNotation(pStaff, nStaff, true, (nWidth > 0))
{
    m_nSpacerWidth = nWidth;
}

lmLUnits lmSpacer::LayoutObject(lmBox* pBox, lmPaper* pPaper, wxColour colorC)
{
    // set total width
    return m_pVStaff->TenthsToLogical(m_nSpacerWidth, m_nStaffNum);

}

wxString lmSpacer::Dump()
{
    wxString sDump = wxString::Format(
        _T("%d\tSpacer %.2f\tTimePos=%.2f, fixed=%s\n"),
        m_nId, m_nSpacerWidth, m_rTimePos, (m_fFixedPos ? _T("yes") : _T("no")) );
    return sDump;
            
}

wxString lmSpacer::SourceLDP(int nIndent)
{
    wxString sSource = _T("");
    sSource.append(nIndent * lmLDP_INDENT_STEP, _T(' '));
    sSource += wxString::Format(_T("(spacer %.0f)\n"), m_nSpacerWidth);
    return sSource;

}

wxString lmSpacer::SourceXML(int nIndent)
{
    // THINK:
    // MusicXML doesn't have the spacer concept. Instead, each element have positioning 
    // information. To covert to MusicXML it is enough to save positioning information.

    wxString sSource = _T("");
    return sSource;

}


//-----------------------------------------------------------------------------------------
// lmAnchor implementation
//-----------------------------------------------------------------------------------------

lmAnchor::lmAnchor(lmVStaff* pStaff, int nStaff)
    : lmNotation(pStaff, nStaff, lmVISIBLE, lmNO_DRAGGABLE)
{
}

lmLUnits lmAnchor::LayoutObject(lmBox* pBox, lmPaper* pPaper, wxColour colorC)
{
    // set total width
    return 0;

}

wxString lmAnchor::Dump()
{
    wxString sDump = wxString::Format(
        _T("%d\tAnchor    \tTimePos=%.2f, fixed=%s\n"),
        m_nId, m_rTimePos, (m_fFixedPos ? _T("yes") : _T("no")) );
	sDump += lmStaffObj::Dump();
    return sDump;
            
}

wxString lmAnchor::SourceLDP(int nIndent)
{
    return lmStaffObj::SourceLDP(nIndent);
}

wxString lmAnchor::SourceXML(int nIndent)
{
    // TODO
    wxString sSource = _T("");
    return sSource;

}



//-----------------------------------------------------------------------------------------
// lmScoreAnchor implementation
//-----------------------------------------------------------------------------------------

lmScoreAnchor::lmScoreAnchor(lmVStaff* pStaff, int nStaff)
    : lmNotation(pStaff, nStaff, lmVISIBLE, lmNO_DRAGGABLE)
{
}

lmLUnits lmScoreAnchor::LayoutObject(lmBox* pBox, lmPaper* pPaper, wxColour colorC)
{
    //force paper pos. to page origin
    pPaper->RestartPageCursors();

    // set total width
    return 0;

}

wxString lmScoreAnchor::Dump()
{
    wxString sDump = wxString::Format(
        _T("%d\tScoreAnchor\tTimePos=%.2f, fixed=%s\n"),
        m_nId, m_rTimePos, (m_fFixedPos ? _T("yes") : _T("no")) );
	sDump += lmStaffObj::Dump();
    return sDump;
}

wxString lmScoreAnchor::SourceLDP(int nIndent)
{
    return lmStaffObj::SourceLDP(nIndent);
}

wxString lmScoreAnchor::SourceXML(int nIndent)
{
    // TODO
    wxString sSource = _T("");
    return sSource;

}

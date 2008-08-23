//--------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2008 Cecilio Salmeron
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
#pragma implementation "Text.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "Score.h"
#include "Text.h"
#include "UndoRedo.h"
#include "../graphic/GMObject.h"
#include "../graphic/Shapes.h"
#include "properties/DlgProperties.h"
#include "properties/TextProperties.h"
#include "properties/DlgTextStyles.h"



//Aux. function to convert font pointsize to lmLUnits
lmLUnits PointsToLUnits(int nPoints)
{
    //One point equals 1/72 of an inch
    //One inch equals 2.54 cm = 25.4 mm
    //then 1 pt = 25.4/72 mm
    return lmToLogicalUnits(nPoints * 25.4 / 72.0, lmMILLIMETERS);
}

//Inverse function: convert font points to lmLUnits
int LUnitsToPoints(lmLUnits uUnits)
{
    //One point equals 1/72 of an inch
    //One inch equals 2.54 cm = 25.4 mm
    //then 1 pt = 25.4/72 mm
    //and 1mm = 72/25.4 pt
    return (int)(0.5 + lmLogicalToUserUnits(uUnits * 72.0 / 25.4, lmMILLIMETERS) );
}

//Global variables used as default initializators
lmFontInfo tBasicTextDefaultFont = { _T("Arial"), 12, lmTEXT_NORMAL };



//==========================================================================================
// lmBasicText implementation
//==========================================================================================

lmBasicText::lmBasicText(wxString& sText, lmLocation& tPos, lmFontInfo& tFontData,
                         wxColour color, wxString sLanguage)
{
    m_sText = sText;
    m_sLanguage = sLanguage;
    SetFontInfo(tFontData);
    m_tTextPos = tPos;
	m_color = color;
}

void lmBasicText::SetFontInfo(lmFontInfo& tFont)
{
    m_sFontName = tFont.sFontName;
    m_nFontSize = tFont.nFontSize;
    m_fBold = (tFont.nStyle == lmTEXT_BOLD || tFont.nStyle == lmTEXT_ITALIC_BOLD);
    m_fItalic = (tFont.nStyle == lmTEXT_ITALIC || tFont.nStyle == lmTEXT_ITALIC_BOLD);
}

lmFontInfo lmBasicText::GetFontInfo()
{
    lmFontInfo tFont;
    tFont.nFontSize = m_nFontSize;
    tFont.sFontName = m_sFontName;
    if (m_fBold && m_fItalic)
        tFont.nStyle = lmTEXT_ITALIC_BOLD;
    else if (m_fBold)
        tFont.nStyle = lmTEXT_BOLD;
    else if(m_fItalic)
        tFont.nStyle = lmTEXT_ITALIC;
    else
        tFont.nStyle = lmTEXT_NORMAL;

    return tFont;
}


//==========================================================================================
// lmScoreText implementation
//==========================================================================================

lmScoreText::lmScoreText(wxString& sTitle, lmEAlignment nAlign, lmLocation& tPos,
                         lmFontInfo& tFont, bool fTitle, wxColour colorC)
    : lmAuxObj(lmDRAGGABLE),
      lmBasicText(sTitle, tPos, tFont, colorC)
{
    m_fIsTitle = fTitle;
    m_nAlignment = nAlign;
}

lmShapeText* lmScoreText::CreateShape(lmPaper* pPaper, lmUPoint uPos)
{
    // Creates the shape and returns it

    lmShapeText* pGMObj = new lmShapeText(this, m_sText, GetSuitableFont(pPaper), pPaper,
                           uPos, _T("ScoreText"), lmDRAGGABLE, m_color);
    StoreShape(pGMObj);
    return pGMObj;
}

lmUPoint lmScoreText::ComputeBestLocation(lmUPoint& uOrg, lmPaper* pPaper)
{
	// if no location is specified in LDP source file, this method is invoked from
	// base class to ask derived object to compute a suitable position to
	// place itself.
	// uOrg is the assigned paper position for this object.

	lmUPoint uPos = uOrg;
	return uPos;
}

lmLUnits lmScoreText::LayoutObject(lmBox* pBox, lmPaper* pPaper, lmUPoint uPos, wxColour colorC)
{
    // This method is invoked by the base class (lmStaffObj). It is responsible for
    // creating the shape object and adding it to the graphical model.

	WXUNUSED(colorC);

    //create the shape object
    lmShapeText* pShape = CreateShape(pPaper, uPos);

    //According to LDP specifications [LDP manual, 3.6. The Text element] default text
    //anchor pos is at bottom left corner. But text shape anchor pos is top left corner.
    //Therefore it is necessary to shift the shape upwards by text height.
    lmLUnits uHeight = pShape->GetHeight();
    pShape->Shift(0.0f, -uHeight);

    //add shape to graphic model
	pBox->AddShape(pShape);

	// set total width
	return pShape->GetWidth();
}

wxString lmScoreText::Dump()
{
    wxString sDump = wxString::Format(
        _T("%d\tText '%s'"), m_nId, m_sText.Left(15).c_str() );

    sDump += lmAuxObj::Dump();
    sDump += _T("\n");
    return sDump;
}

wxString lmScoreText::SourceLDP(int nIndent)
{
    wxString sSource = _T("");
    sSource.append(nIndent * lmLDP_INDENT_STEP, _T(' '));
    sSource += (m_fIsTitle ? _T("(title") : _T("(text"));

    //text goes after main tag in 'text' tags
    if (!m_fIsTitle)
    {
        sSource += _T(" \"");
        sSource += m_sText;
        sSource += _T("\"");
    }

    //alignment
    if (m_nAlignment == lmALIGN_CENTER)
        sSource += _T(" center");
    else if (m_nAlignment == lmALIGN_LEFT)
        sSource += _T(" left");
    else
        sSource += _T(" right");

    //text goes after alignment in 'title' tags
    if (m_fIsTitle)
    {
        sSource += _T(" \"");
        sSource += m_sText;
        sSource += _T("\"");
    }

    //font info
    //TODO:: add font info only if font changed since previous (text) element
    sSource += wxString::Format(_T(" (font \"%s\" %dpt"),
                                m_sFontName.c_str(), m_nFontSize );
    if (m_fBold && m_fItalic)
        sSource += _T(" bold-italic");
    else if (m_fBold)
        sSource += _T(" bold");
    else if(m_fItalic)
        sSource += _T(" italic");
    else
        sSource += _T(" normal");
    sSource += _T(")");

	//base class info
    sSource += lmAuxObj::SourceLDP(nIndent);

    //close element
    sSource += _T(")\n");
    return sSource;
}

wxString lmScoreText::SourceXML(int nIndent)
{
    //TODO
    wxString sSource = _T("TODO: lmScoreText XML Source code generation methods");
    return sSource;

////    <direction placement="above">
////      <direction-type>
////        <words xml:lang="la" relative-y="5" relative-x="-5">Angelus
//// dicit:</words>
////      </direction-type>
////    </direction>
//
//    sFuente = "<direction placement=""?"">" & sCrLf & _
//                "  <direction-type>" & sCrLf & _
//                "    <words xml:lang=""??"" relative-y=""??"" relative-x=""??"">" & _
//                    m_sTexto & "</words>" & sCrLf & _
//                "  </direction-type>" & sCrLf & _
//                "<direction>"
//
}

wxFont* lmScoreText::GetSuitableFont(lmPaper* pPaper)
{
    //wxLogMessage(_T("[lmScoreText::GetSuitableFont]: size=%d, name=%s"),
	//             m_nFontSize, m_sFontName );

    int nWeight = (m_fBold ? wxBOLD : wxNORMAL);
    int nStyle = (m_fItalic ? wxITALIC : wxNORMAL);
    wxFont* pFont = pPaper->GetFont((int)PointsToLUnits(m_nFontSize),
                                    m_sFontName, wxDEFAULT, nStyle, nWeight, false);

    if (!pFont) {
        wxMessageBox(_("Sorry, an error has occurred while allocating the font."),
            _T("lmScoreText::GetSuitableFont"), wxOK);
        ::wxExit();
    }
	return pFont;
}

void lmScoreText::OnProperties(lmController* pController, lmGMObject* pGMO)
{
	wxASSERT(pGMO);

    //lmDlgProperties dlg(pController, this);
    //dlg.AddPanel( new lmTextProperties(dlg.GetNotebook(), this), _("Text"));
    //dlg.Layout();

    lmDlgTextStyles dlg((wxWindow*)pController);

    dlg.ShowModal();
}

void lmScoreText::Cmd_ChangeText(lmUndoItem* pUndoItem, wxString& sText, lmEAlignment nAlign,
                                 lmLocation tPos, lmFontInfo& tFont, wxColour colorC)
{
    m_sText = sText;
    m_nAlignment = nAlign;
    m_tTextPos = tPos;
    SetFontInfo(tFont);
	m_color = colorC;
}

void lmScoreText::UndoCmd_ChangeText(lmUndoItem* pUndoItem, wxString& sText,
                                     lmEAlignment nAlign, lmLocation tPos,
                                     lmFontInfo& tFont, wxColour colorC)
{
    Cmd_ChangeText(pUndoItem, sText, nAlign, tPos, tFont, colorC);
}

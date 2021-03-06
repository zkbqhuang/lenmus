//---------------------------------------------------------------------------------------
// This file is part of the Lomse library.
// Lomse is copyrighted work (c) 2010-2016. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice, this
//      list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright notice, this
//      list of conditions and the following disclaimer in the documentation and/or
//      other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// For any comment, suggestion or feature request, please contact the manager of
// the project at cecilios@users.sourceforge.net
//---------------------------------------------------------------------------------------

#include "lomse_lyric_engraver.h"

#include "lomse_glyphs.h"
#include "lomse_score_meter.h"
#include "lomse_engraving_options.h"
#include "lomse_im_note.h"
#include "lomse_shape_note.h"
#include "lomse_shape_line.h"
#include "lomse_internal_model.h"
#include "lomse_calligrapher.h"
#include "lomse_gm_basic.h"
#include "lomse_shape_text.h"
#include "lomse_instrument_engraver.h"

#include <cmath>        // fabs


namespace lomse
{

//=======================================================================================
// LyricEngraver implementation
//=======================================================================================
LyricEngraver::LyricEngraver(LibraryScope& libraryScope, ScoreMeter* pScoreMeter,
                             InstrumentEngraver* pInstrEngrv)
    : AuxRelObjEngraver(libraryScope, pScoreMeter)
    , m_pLyricsShape(NULL)
    , m_pInstrEngrv(pInstrEngrv)
{
}

//---------------------------------------------------------------------------------------
LyricEngraver::~LyricEngraver()
{
    m_lyrics.clear();
}

//---------------------------------------------------------------------------------------
void LyricEngraver::set_start_staffobj(ImoAuxRelObj* pARO, ImoStaffObj* UNUSED(pSO),
                                       GmoShape* pStaffObjShape, int iInstr, int iStaff,
                                       int iSystem, int iCol, LUnits UNUSED(xRight),
                                       LUnits UNUSED(xLeft), LUnits yTop)
{
    m_iInstr = iInstr;
    m_iStaff = iStaff;
    ImoLyric* pLyric = dynamic_cast<ImoLyric*>(pARO);
    m_lyrics.push_back( make_pair(pLyric, pStaffObjShape) );

    ShapeBoxInfo* pInfo = LOMSE_NEW ShapeBoxInfo(NULL, iSystem, iCol, iInstr);
    m_shapesInfo.push_back(pInfo);
    m_staffTops.push_back(yTop - pStaffObjShape->get_top());
}

//---------------------------------------------------------------------------------------
void LyricEngraver::set_middle_staffobj(ImoAuxRelObj* pARO, ImoStaffObj* UNUSED(pSO),
                                        GmoShape* pStaffObjShape, int iInstr,
                                        int UNUSED(iStaff), int iSystem, int iCol,
                                        LUnits UNUSED(xRight), LUnits UNUSED(xLeft),
                                        LUnits yTop)
{
    ImoLyric* pLyric = dynamic_cast<ImoLyric*>(pARO);
    m_lyrics.push_back( make_pair(pLyric, pStaffObjShape) );

    ShapeBoxInfo* pInfo = LOMSE_NEW ShapeBoxInfo(NULL, iSystem, iCol, iInstr);
    m_shapesInfo.push_back(pInfo);
    m_staffTops.push_back(yTop - pStaffObjShape->get_top());
}

//---------------------------------------------------------------------------------------
void LyricEngraver::set_end_staffobj(ImoAuxRelObj* pARO, ImoStaffObj* UNUSED(pSO),
                                     GmoShape* pStaffObjShape, int iInstr,
                                     int UNUSED(iStaff), int iSystem, int iCol,
                                     LUnits UNUSED(xRight), LUnits UNUSED(xLeft),
                                     LUnits yTop)
{
    ImoLyric* pLyric = dynamic_cast<ImoLyric*>(pARO);
    m_lyrics.push_back( make_pair(pLyric, pStaffObjShape) );

    ShapeBoxInfo* pInfo = LOMSE_NEW ShapeBoxInfo(NULL, iSystem, iCol, iInstr);
    m_shapesInfo.push_back(pInfo);
    m_staffTops.push_back(yTop - pStaffObjShape->get_top());
}

//---------------------------------------------------------------------------------------
int LyricEngraver::create_shapes(Color color)
{
    m_color = color;

    list< pair<ImoLyric*, GmoShape*> >::iterator it;
    int i = 0;
    for(it=m_lyrics.begin(); it != m_lyrics.end(); ++it, ++i)
	{
        ImoLyric* pLyric = dynamic_cast<ImoLyric*>(it->first);
        GmoShapeNote* pNoteShape = dynamic_cast<GmoShapeNote*>(it->second);

        GmoShapeNote* pNextNoteShape = NULL;
        list< pair<ImoLyric*, GmoShape*> >::iterator nextIt = it;
        ++nextIt;
        if (nextIt != m_lyrics.end())
            pNextNoteShape = dynamic_cast<GmoShapeNote*>(nextIt->second);

        create_shape(i, pNoteShape, pLyric, pNextNoteShape);
    }
    return int(m_lyrics.size());

};

//---------------------------------------------------------------------------------------
void LyricEngraver::create_shape(int iNote, GmoShapeNote* pNoteShape, ImoLyric* pLyric,
                                 GmoShapeNote* pNextNoteShape)
{
    LUnits x = pNoteShape->get_left();

    //TODO: base position should be the maximum notes bottom y + 5.0f instead
    //      of a fixed amount (60.0f)
    //TODO: line increment should be text shape height + 0.5f instead o a fixed
    //      amount (23.0f)
    int lineNum = pLyric->get_number();
    int tenths = 60.0f + 23.0f * float(lineNum);
    LUnits y = m_staffTops[iNote] + pNoteShape->get_top() + tenths_to_logical(tenths);

    //get text
    ImoLyricsTextInfo* pText = pLyric->get_text_item(0);
    const string& text = pText->get_syllable_text();
    const string& language = pText->get_syllable_language();
    ImoStyle* pStyle = pText->get_syllable_style();
    if (pStyle == NULL)
        pStyle = m_pMeter->get_lyrics_style_info();
    Color color = pStyle->color();


    //create container shape
    ShapeId idx = 0;
    m_pLyricsShape = LOMSE_NEW GmoShapeLyrics(pLyric, idx, Color(0,0,0) /*unused*/,
                                              m_libraryScope);

    //add shape for text
    GmoShape* pSyllableShape = LOMSE_NEW GmoShapeText(pLyric, idx, text, pStyle,
                                                      language, x, y, m_libraryScope);

    //center syllable on note head
    LUnits shift = (pNoteShape->get_width() - pSyllableShape->get_width()) / 2.0f;
    pSyllableShape->shift_origin(shift, 0.0f);
    m_pLyricsShape->shift_origin(USize(shift, 0.0f));

	m_pLyricsShape->add(pSyllableShape);
    x = pSyllableShape->get_right();


    //add shape for hyphenation, if needed
    if (pLyric->has_hyphenation() && !pLyric->has_melisma())
    {
        GmoShape* pShape = LOMSE_NEW GmoShapeText(pLyric, idx, "-", pStyle,
                                                  language, x, y, m_libraryScope);

        //shift shape to center between this and next syllable
        if (pNextNoteShape)
        {
            LUnits space = pNextNoteShape->get_left() - pSyllableShape->get_right();
            LUnits shift = (space - pShape->get_width()) / 2.0f;
            pShape->shift_origin(shift, 0.0f);
        }

        m_pLyricsShape->add(pShape);
        x = pShape->get_left() + pShape->get_width();
    }

    //add shape for melisma line, if needed
    if (pLyric->has_melisma())
    {
        LUnits xStart = x + tenths_to_logical(3.0f);;
        LUnits xEnd;
        if (pNextNoteShape)
        {
            xEnd = pNextNoteShape->get_left();
            if (xEnd < xStart)
            {
                //note is in next system. Melisma line to end of current system
                xEnd = m_pInstrEngrv->get_staves_right();
            }
            else
            {
                xEnd -= tenths_to_logical(20.0f);
                if (xEnd < xStart)
                    xEnd = pNextNoteShape->get_left();
            }
        }
        else
            //TODO: Melisma line must extend until last note in this voice
            xEnd = xStart + tenths_to_logical(60.0f);     //TODO

        LUnits width = tenths_to_logical(1.0f);     //TODO engraving option
        LUnits boundsExtraWidth = tenths_to_logical(0.5f);
        GmoShape* pShape = LOMSE_NEW GmoShapeLine(pLyric, idx, xStart, y, xEnd, y,
                                        width, boundsExtraWidth, k_line_solid, color,
                                        k_edge_normal, k_cap_none, k_cap_none);

        m_pLyricsShape->add(pShape);

        x += pShape->get_width();
    }

    m_pShape = m_pLyricsShape;
    ShapeBoxInfo* pInfo = m_shapesInfo[iNote];
    pInfo->pShape = m_pLyricsShape;
}

//---------------------------------------------------------------------------------------
void LyricEngraver::decide_placement()
{
}


}  //namespace lomse

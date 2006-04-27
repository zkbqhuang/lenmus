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
/*! @file GraphicManager.h
    @brief Header file for class lmGraphicManager
    @ingroup graphic_management
*/
#ifndef __GRAPHICMANAGER_H__        //to avoid nested includes
#define __GRAPHICMANAGER_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "../score/score.h"
#include "Formatter4.h"
#include "BoxScore.h"


//Class lmGraphicManager stores and manages all score renderization issues
class lmGraphicManager
{
public:
    lmGraphicManager(lmScore* pScore, lmPaper* pPaper);
    ~lmGraphicManager();

    void Layout();      // measure phase
    void Render(lmPaper* pPaper);      // drawing phase


private:
    lmScore*        m_pScore;       //score to be rendered
    lmPaper*        m_pPaper;       //paper to use

    lmFormatter4    m_oFormatter;   //the formatter object
    lmBoxScore*     m_pBoxScore;    //the main container


};

#endif  // __GRAPHICMANAGER_H__


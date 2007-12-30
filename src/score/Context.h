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

#ifndef __LM_CONTEXT_H__        //to avoid nested includes
#define __LM_CONTEXT_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "Context.cpp"
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


class lmContext
{
public:
    lmContext(lmClef* pClef, lmKeySignature* pKey, lmTimeSignature* pTime, int nStaff);
    lmContext(lmContext* pContext);
    ~lmContext() {}

    void SetAccidental(int i, int nValue) { m_nAccidentals[i] = nValue; }
    int GetAccidentals(int i) { return m_nAccidentals[i]; }
    void CopyAccidentals(lmContext* pContext);

    inline lmClef* GetClef() const { return m_pClef; }
    inline lmKeySignature* GeyKey() const { return m_pKey; }
    inline lmTimeSignature* GetTime() const { return m_pTime; }
	inline int GetNumStaff() { return m_nStaff; }

	//navigation and list management
	inline lmContext* GetPrev() const { return m_pPrev; }
	inline lmContext* GetNext() const { return m_pNext; }
	inline void SetPrev(lmContext* pPrev) { m_pPrev = pPrev; }
	inline void SetNext(lmContext* pNext) { m_pNext = pNext; }

	//other
	bool AppliesTo(int nStaff) { return (m_nStaff==0 || nStaff==m_nStaff); }


private:
    void InitializeAccidentals();

        // member variables

    //information about current clef, key and time signature
    lmClef*             m_pClef;
    lmKeySignature*     m_pKey;
    lmTimeSignature*    m_pTime;

    //the next array keeps information about the accidentals applicable to each
    //note. Each element refers to one note: 0=Do, 1=Re, 2=Mi, 3=Fa, ... , 6=Si
    int     m_nAccidentals[7];

    //staff to which this context applies (1..n). 0 if applicable to all
    int     m_nStaff;				

	//Contexts are organized as a double linked list. Here are the links
	lmContext*		m_pNext;		//pointer to next context 
	lmContext*		m_pPrev;		//pointer to previous context 

};



// this defines the type ArrayOfContexts as an array of lmContext pointers
WX_DEFINE_ARRAY(lmContext*, ArrayOfContexts);


#endif  // __LM_CONTEXT_H__

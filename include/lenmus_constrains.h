//---------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2011 LenMus project
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
//---------------------------------------------------------------------------------------

#ifndef __LENMUS_CONSTRAINS_H__        //to avoid nested includes
#define __LENMUS_CONSTRAINS_H__

//lenmus
#include "lenmus_standard_header.h"
#include "lenmus_injectors.h"
#include "lenmus_string.h"

//wxWidgets
#include <wx/wxprec.h>
#include <wx/wx.h>

//lomse
#include "lomse_internal_model.h"
using namespace lomse;


namespace lenmus
{

//---------------------------------------------------------------------------------------
// lomse enums for clefs, key signatures, etc. can not be directly used in the
// exercises. Instead exercise specific enums are defined, as well as 'translation'
// methods'

//clefs available for exercises
enum EClefExercise
{
    lmE_Undefined = -1,
    lmE_G,
    lmE_Fa4,
    lmE_Fa3,
    lmE_Do1,
    lmE_Do2,
    lmE_Do3,
    lmE_Do4,
    lmE_Percussion,
};
// AWARE enum constats EClefExercise are going to be ramdomly generated in object
// Generators. The following constants define de range.
#define lmMIN_CLEF        lmE_G
#define lmMAX_CLEF        lmE_Percussion
// AWARE enum constats EClefExercise are going to be used as indexes in ClefConstrains


//---------------------------------------------------------------------------------------
//Time signatures used in exercises
enum ETimeSignature
{
    k_time_2_4 = 1,  //  2/4
    k_time_3_4,      //  3/4
    k_time_4_4,      //  4/4
    k_time_6_8,      //  6/8
    k_time_9_8,      //  9/8
    k_time_12_8,     // 12/8
    k_time_2_8,      //  2/8
    k_time_3_8,      //  3/8
    k_time_2_2,      //  2/2
    k_time_3_2,      //  3/2
};
// AWARE enum constats ETimeSignature are going to be ramdomly generated in object
// RandomGenerator. The following constants define de maximum and minimum values.
#define k_min_time_signature  k_time_2_4
#define k_max_time_signature  k_time_3_2


//---------------------------------------------------------------------------------------
// name of the intervals considered in exercises
enum EIntervalName
{
    ein_1 = 0,      // unison       // must start with 0. Used as index for arrays
    ein_2min,
    ein_2maj,
    ein_3min,
    ein_3maj,
    ein_4,
    ein_4aug,
    ein_5,
    ein_6min,
    ein_6maj,
    ein_7min,
    ein_7maj,
    ein_8,
    ein_9min,
    ein_9maj,
    ein_10min,
    ein_10maj,
    ein_11,
    ein_11aug,
    ein_12,
    ein_13min,
    ein_13maj,
    ein_14min,
    ein_14maj,
    ein_2oct,
    ein_Max_Item        //Must be the last one. Just to know how many items
};

#define lmNUM_INTVALS  ein_Max_Item     //num intervals considered in constraints


//---------------------------------------------------------------------------------------
//problem generation and evaluation modes
enum
{
    k_learning_mode = 0,
    k_practise_mode,
    k_exam_mode,
    k_quiz_mode,
    //
    k_num_generation_modes
};

extern const wxString& get_generation_mode_name(long nMode);


//---------------------------------------------------------------------------------------
class ClefConstrains
{
public:
    ClefConstrains();
    ~ClefConstrains() {}
    bool IsValid(EClefExercise nClef) { return m_fValidClefs[nClef-lmMIN_CLEF]; }
    void SetValid(EClefExercise nClef, bool fValid) { m_fValidClefs[nClef-lmMIN_CLEF] = fValid; }

    //pitch scope
    wxString GetLowerPitch(EClefExercise nClef)  { return m_aLowerPitch[nClef-lmMIN_CLEF]; }
    wxString GetUpperPitch(EClefExercise nClef)  { return m_aUpperPitch[nClef-lmMIN_CLEF]; }
    void SetLowerPitch(EClefExercise nClef, wxString sPitch)  {
                m_aLowerPitch[nClef-lmMIN_CLEF] = sPitch;
            }
    void SetUpperPitch(EClefExercise nClef, wxString sPitch)  {
                m_aUpperPitch[nClef-lmMIN_CLEF] = sPitch;
            }


private:
    bool m_fValidClefs[lmMAX_CLEF - lmMIN_CLEF + 1];
    wxString m_aLowerPitch[lmMAX_CLEF - lmMIN_CLEF + 1];
    wxString m_aUpperPitch[lmMAX_CLEF - lmMIN_CLEF + 1];
};

//---------------------------------------------------------------------------------------
class KeyConstrains
{
public:
    KeyConstrains();
    ~KeyConstrains() {}
    bool IsValid(EKeySignature nKey) { return m_fValidKeys[nKey-k_min_key]; }
    void SetValid(EKeySignature nKey, bool fValid) { m_fValidKeys[nKey-k_min_key] = fValid; }
    EKeySignature GetRandomKeySignature();

private:
    bool m_fValidKeys[k_max_key - k_min_key + 1];
};

//---------------------------------------------------------------------------------------
class TimeSignConstrains
{
public:
    TimeSignConstrains();
    ~TimeSignConstrains() {}
    bool IsValid(ETimeSignature nTime) { return m_fValidTimes[nTime-k_min_time_signature]; }
    void SetValid(ETimeSignature nTime, bool fValid) {
            m_fValidTimes[nTime-k_min_time_signature] = fValid;
        }
    bool SetConstrains(wxString sTimeSigns);

private:
    bool m_fValidTimes[k_max_time_signature - k_min_time_signature + 1];
};


//---------------------------------------------------------------------------------------
// Abstract class to create constrains/options objects for EBookCtrolOptions
class EBookCtrolOptions
{
protected:
    ApplicationScope& m_appScope;
    wxString  m_sSection;       //section name to save the constraints
    string  m_sGoBackURL;       //URL for "Go back" link of empty string if no link
    bool    m_fPlayLink;        //In theory mode the score could be not playable
    bool    m_fSettingsLink;    //include 'settings' link

public:
    EBookCtrolOptions(const wxString& sSection, ApplicationScope& appScope);
    virtual ~EBookCtrolOptions() {}

    virtual void save_settings() = 0;
    virtual void load_settings() = 0;

    inline void SetSettingsLink(bool fValue) { m_fSettingsLink = fValue; }
    inline bool IncludeSettingsLink() { return m_fSettingsLink; }
    void set_section(const string& sSection);
    inline wxString& GetSection() { return m_sSection; }

    inline void set_go_back_link(const string& sURL) { m_sGoBackURL = sURL; }
    inline bool IncludeGoBackLink() { return m_sGoBackURL != ""; }
    inline string& GetGoBackURL() { return m_sGoBackURL; }

    inline void SetPlayLink(bool fValue) { m_fPlayLink = fValue; }
    inline bool IncludePlayLink() { return m_fPlayLink; }

};

//---------------------------------------------------------------------------------------
class ExerciseOptions : public EBookCtrolOptions
{
protected:
    //The Ctrol could be used both for ear training exercises and for theory exercises.
    //Following variables are used for configuration
    bool    m_fTheoryMode;

    //options
    bool    m_fButtonsEnabledAfterSolution;
    bool    m_fUseCounters;     //option to not use counters
    bool    m_fSolutionLink;    //include 'show solution' link
    long    m_nGenerationMode;  //problem generation & evaluation mode
    bool    m_fSupportedMode[k_num_generation_modes];

public:
    ExerciseOptions(const wxString& sSection, ApplicationScope& appScope);
    virtual ~ExerciseOptions() {}

    inline void set_theory_mode(bool fValue) { m_fTheoryMode = fValue; }
    inline bool is_theory_mode() { return m_fTheoryMode; }
    inline bool IsEarTrainingMode() { return !m_fTheoryMode; }

    inline void SetButtonsEnabledAfterSolution(bool fValue) {
        m_fButtonsEnabledAfterSolution = fValue;
    }
    inline bool ButtonsEnabledAfterSolution() { return m_fButtonsEnabledAfterSolution; }

    inline void SetSolutionLink(bool fValue) { m_fSolutionLink = fValue; }
    inline bool IncludeSolutionLink() { return m_fSolutionLink; }

    inline void SetUsingCounters(bool fValue) { m_fUseCounters = fValue; }
    inline bool IsUsingCounters() { return m_fUseCounters; }

    inline void SetGenerationMode(long nMode) { m_nGenerationMode = nMode; }
    inline long GetGenerationMode() { return m_nGenerationMode; }
    inline bool IsGenerationModeSupported(long nMode) { return m_fSupportedMode[nMode]; }
    inline void SetGenerationModeSupported(long nMode, bool fValue) {
                                           m_fSupportedMode[nMode] = fValue; }
};


//---------------------------------------------------------------------------------------
// Options for ScoreCtrol control
class ScoreCtrolOptions : public EBookCtrolOptions
{
public:
    ScoreCtrolOptions(const wxString& sSection, ApplicationScope& appScope);
    ~ScoreCtrolOptions() {}

    void SetControlPlay(bool fValue, wxString sLabels = _T(""))
        {
            fPlayCtrol = fValue;
            if (sLabels != _T(""))
                SetLabels(sLabels, &sPlayLabel, &sStopPlayLabel);
        }
    void SetControlSolfa(bool fValue, wxString sLabels = _T(""))
        {
            fSolfaCtrol = fValue;
            if (sLabels != _T(""))
                SetLabels(sLabels, &sSolfaLabel, &sStopSolfaLabel);
        }
    void SetControlMeasures(bool fValue, wxString sLabels = _T(""))
        {
            fMeasuresCtrol = fValue;
            if (sLabels != _T(""))
                SetLabels(sLabels, &sMeasuresLabel, &sStopMeasureLabel);
        }

    void SetMetronomeMM(long nValue) { m_nMM = nValue; }
    long GetMetronomeMM() { return m_nMM; }


    bool        fPlayCtrol;             //Instert "Play" link
    wxString    sPlayLabel;             //label for "Play" link
    wxString    sStopPlayLabel;         //label for "Stop playing" link

    bool        fSolfaCtrol;            //insert a "Sol-fa" link
    wxString    sSolfaLabel;            //label for "Sol-fa" link
    wxString    sStopSolfaLabel;        //label for "Stop sol-fa" link

    bool        fMeasuresCtrol;         //insert "play-measure" links
    wxString    sMeasuresLabel;
    wxString    sStopMeasureLabel;

    bool        fBorder;                // border around control
    bool        fMusicBorder;           // border around music

    double      rTopMargin;             //top margin for score, in millimeters

    double      rScale;

private:
    void SetLabels(wxString& sLabel, wxString* pStart, wxString* pStop);

    long        m_nMM;                  // metronome setting

};


}   //namespace lenmus

#endif  // __LENMUS_CONSTRAINS_H__

//--------------------------------------------------------------------------------------
//    LenMus project: free software for music theory and language
//    Copyright (c) 2002-2009 Cecilio Salmeron
//
//    This program is free software; you can redistribute it and/or modify it under the 
//    terms of the GNU General Public License as published by the Free Software Foundation;
//    either version 3 of the License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but WITHOUT ANY 
//    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
//    PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along with this 
//    program. If not, see <http://www.gnu.org/licenses/>. 
//
//
//    For any comment, suggestion or feature request, please contact the manager of 
//    the project at cecilios@users.sourceforge.net
//
//-------------------------------------------------------------------------------------
#ifdef __GNUG__
#pragma implementation ebook_processor.h
#endif

// for (compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/filename.h"
#include "wx/txtstrm.h"
#include "wx/zipstrm.h"
#include "wx/textfile.h"
#include "wx/arrstr.h"

#include "ebook_processor.h"
#include "wx/xml2.h"            // include libxml2 wrapper definitions
#include "wx/dtd.h"				// include libxml2 wrapper definitions
#include "Paths.h"


#define ltNO_INDENT false

enum {
    eNOTHING = 0,
    eTOC = 1,
    eHTML = 2,
    eIDX = 4,
    eTRANSLATE = 8,
    // enable to change flag value
    eTOC_ENABLED = 256,
    eHTML_ENABLED = 512,
    eIDX_ENABLED = 1024,
    eTRANSLATE_ENABLED = 2048,
    eENABLE_ALL = eTOC_ENABLED | eHTML_ENABLED | eIDX_ENABLED | eTRANSLATE_ENABLED,

};

// Constant strings that need translation
static const wxString m_sFooter1 = 
    _T("Send your comments and suggestions to the LenMus team (www.lenmus.org)");
static const wxString m_sFooter2 = 
    _T("Licensed under the terms of the GNU Free Documentation License v1.3");
static const wxString m_sPhonascus =
    _T("the teacher of music");
static const wxString m_sCoverPage =
    _T("Cover page");


lmEbookProcessor::lmEbookProcessor(int nDbgOptions, wxTextCtrl* pUserLog)
{
    m_pTocFile = (wxFile*) NULL;
    m_pHtmlFile = (wxFile*) NULL;
    m_pLangFile = (wxFile*)NULL;
    m_pLmbFile = (wxTextOutputStream*)NULL;
    m_pZipFile = (wxZipOutputStream*)NULL;

    m_pLog = pUserLog;

    //options. TODO: dialog to change options
    m_fGenerateLmb = true;

    //debug options
    m_fLogTree = (nDbgOptions & eLogTree) != 0;
    m_fDump = (nDbgOptions & eDumpTree) != 0;
}

lmEbookProcessor::~lmEbookProcessor()
{
    if (m_pTocFile) delete m_pTocFile;
    if (m_pHtmlFile) delete m_pHtmlFile;
    if (m_pLangFile) delete m_pLangFile;
    if (m_pLmbFile) delete m_pLmbFile;
    if (m_pZipFile) delete m_pZipFile;

}

bool lmEbookProcessor::GenerateLMB(wxString sFilename, wxString sLangCode, 
                                   wxString sCharCode, int nOptions)
{
    // returns false if error
    // PO and lang.cpp are created in sDestName (langtool\locale\)
    // book.lmb is created in sDestName (lenmus\locale\xx\books\)
    // book.toc is temporarily created in sDestName (lenmus\locale\xx\books\)

    // Prepare for a new XML file processing
    m_fProcessingBookinfo = false;
    m_fOnlyLangFile = nOptions & lmLANG_FILE;
    m_fGenerateLmb = !m_fOnlyLangFile;
    m_sFilename = sFilename;
    m_sCharCode = sCharCode;
    m_sLangCode = sLangCode;

    //add layout files
    m_aFilesToPack.Empty();


    // load the XML file as tree of nodes
    LogMessage(_T("Loading file %s"), sFilename);
    wxXml2Document oDoc;
    wxString sError;
    if (!oDoc.Load(sFilename, &sError)) {
        LogMessage(_T("Error parsing file %s\nError:%s"), sFilename, sError);
        return false;
    }
    wxXml2Node oRoot = oDoc.GetRoot();

    if (m_fDump) DumpXMLTree(oRoot);        //for debugging

    m_fExtEntity = false;                   //not waiting for an external entity
    m_sExtEntityName = wxEmptyString;       //so, no name

    // Prepare Lang file
    if (m_fOnlyLangFile)
 {
        if (!StartLangFile( sFilename )) {
            LogMessage(_T("Error: Lang file '%s' can not be created."), sFilename);
            oRoot.DestroyIfUnlinked();
            oDoc.DestroyIfUnlinked();
            return false;        //error
        }
    }

    // Prepare the TOC file
    if (!StartTocFile( sFilename )) {
        LogMessage(_T("Error: toc file '%s' can not be created."), sFilename);
        oRoot.DestroyIfUnlinked();
        oDoc.DestroyIfUnlinked();
        return false;        //error
    }

    // prepare de LMB file
    if (m_fGenerateLmb) {
        if (!StartLmbFile(sFilename, sLangCode, m_sCharCode)) {
            LogMessage(_T("Error: LMB file '%s' can not be created."), sFilename);
            oRoot.DestroyIfUnlinked();
            oDoc.DestroyIfUnlinked();
            return false;        //error
        }
    }

    //Process the document. Root node must be <book> or <leaflet>
    bool fError = false;
    if (oRoot.GetName() == _T("book"))
        fError = BookTag(oRoot);
    else if (oRoot.GetName() == _T("leaflet"))
        fError = LeafletTag(oRoot);
    else {
        wxLogMessage(
            _T("Error. First tag is neither <book> nor <leaflet> but <%s>"),
            oRoot.GetName() );
        oRoot.DestroyIfUnlinked();
        oDoc.DestroyIfUnlinked();
        return false;
    }

    // Close files
    TerminateTocFile();
    TerminateLmbFile();

    oRoot.DestroyIfUnlinked();
    oDoc.DestroyIfUnlinked();

    if (fError)
        LogMessage(_T("There are errors in compilation."));
    else
        LogMessage(_T("Compilation done successfully."));

    return !fError;

}

bool lmEbookProcessor::ProcessChildAndSiblings(const wxXml2Node& oNode, int nOptions,
                                               wxString* pText)
{
    m_fExtEntity = false;                   //not waiting for an external entity
    m_sExtEntityName = wxEmptyString;       //so, no name

    wxXml2Node oCurr(oNode.GetFirstChild());
    bool fError = false;
    while (oCurr != wxXml2EmptyNode) {
        fError |= ProcessChildren(oCurr, nOptions, pText);
        oCurr = oCurr.GetNext();
    }
    return fError;
}

bool lmEbookProcessor::ProcessChildren(const wxXml2Node& oNode, int nOptions,
                                       wxString* pText)
{
    bool fError = false;

    if (oNode == wxXml2EmptyNode) return false;
    if (m_fLogTree) wxLogMessage(_T("[ProcessChildren] nodeType=%d. Name='%s', content='%s'"),
                                 oNode.GetType(), oNode.GetName(), oNode.GetContent() );
        
    if (oNode.GetType() == wxXML_TEXT_NODE)
    {
        // it is a text node: write its contents to output
        wxString sContent = oNode.GetContent();
        if (sContent != wxEmptyString && sContent.Last() == wxT('\n')) sContent.RemoveLast();
        if (sContent != wxEmptyString && sContent.GetChar(0) == wxT('\n')) sContent.Remove(0, 1);

        // libxml2 creates text nodes associated to all elements, even 
        // when no content is present in the xml file. In these cases
        // the text node contains just a simple '\n'. Next code lines
        // are for filtering ou these lines
        wxString tmp = sContent.Trim();
        sContent.Replace(_T("\n"), _T(" "));
        while (sContent.Replace(_T("  "), _T(" ")) > 0);    //more than one consecutive space
        
        //replace common entities used in Spanish
        sContent.Replace(_T("\""), _T("&quot;"), true);
        //sContent.Replace(_T("�"), _T("&aacute;"), true);
        //sContent.Replace(_T("�"), _T("&eacute;"), true);
        //sContent.Replace(_T("�"), _T("&iacute;"), true);
        //sContent.Replace(_T("�"), _T("&oacute;"), true);
        //sContent.Replace(_T("�"), _T("&uacute;"), true); 
        //sContent.Replace(_T("�"), _T("&ntilde;"), true); 
        //sContent.Replace(_T("�"), _T("&Ntilde;"), true); 

        if (!tmp.IsEmpty()) {
            wxString sTrans = wxGetTranslation(sContent);
            if (nOptions & eTOC) WriteToToc(sTrans, ltNO_INDENT);        //text not indented
            if (nOptions & eHTML) WriteToHtml(sTrans);
            if (m_fOnlyLangFile && (nOptions & eTRANSLATE)) WriteToLang(sContent);
            //if (fIdx) WriteToIdx(sContent);
            if (pText) {
                if (nOptions & eTRANSLATE)
                    *pText += sTrans;
                else
                    *pText += sContent;
            }
        }

    }
    else if (oNode.GetType() == wxXML_ELEMENT_NODE)
    {
        // it is an element. Process it (recursive, as ProcessTags call ProcessChildAndSiblings)
        fError |= ProcessTag(oNode, nOptions, pText);
    }
    else if (oNode.GetType() == wxXML_ENTITY_DECL)
    {
	    // A DTD node which declares an entity.
	    // This value is used to identify a node as a wxXml2EntityDecl.
	    // Looks like:     <!ENTITY myentity "entity's replacement">
        wxXml2EntityDecl* pNode = (wxXml2EntityDecl*)&oNode;
        //wxLogMessage(_T("[ProcessChildren] External entity (17). Name='%s', SystemID='%s'"),
        //             oNode.GetName(), pNode->GetSystemID() );

        if (m_fExtEntity &&  m_sExtEntityName == oNode.GetName())
        {
            // Insert here the referenced xml tree

            // prepare full URL
            // TODO: For now, assume that external entities are in the same folder than
            //       main xml file. Need to improve this.
            //wxString sOldFilename = m_sFilename;
            wxFileName oFN(m_sFilename);
            oFN.SetFullName(pNode->GetSystemID());
            //m_sFilename = oFN.GetFullPath();

            wxXml2Document oDoc;
            wxString sError;
            LogMessage(_T("Processing %s."), oFN.GetFullName());
            if (!oDoc.Load(oFN.GetFullPath(), &sError)) {
                LogMessage(_T("Error parsing file %s\nError:%s"), oFN.GetFullPath(), sError);
                return true;    //error
            }
            //Verify type of document. Must be <book>
            wxXml2Node oRoot = oDoc.GetRoot();
            ProcessChildren(oRoot, nOptions, pText);
            //m_sFilename = sOldFilename;
            
            //done
            m_fExtEntity = false;
            m_sExtEntityName = wxEmptyString;
        }
    }
    else if (oNode.GetType() == wxXML_ENTITY_REF_NODE) {
	    //! Like a text node, but this node contains only an "entity". Entities
	    //! are strings like: &amp; or &quot; or &lt; ....
        // It must have an wxXML_ENTITY_DECL child. Process it.

        // Save the name of the node
        m_fExtEntity = true;
        m_sExtEntityName = oNode.GetName();

        // and process children (must be the external entities definitions table)
        // the first one is the desired one. Ignore the remaining.
        wxXml2Node oChild(oNode.GetFirstChild());
        while (oChild != wxXml2EmptyNode) {
            ProcessChildren(oChild, nOptions, pText);
            oChild = oChild.GetNext();
        }
    }
    else {
        // ignore it
        //wxLogMessage(_T("[ProcessChildren] NodeType=%d, Name='%s'"),
        //                oNode.GetType(), oNode.GetName() );
    }
    
    return fError;

}

bool lmEbookProcessor::ProcessTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    if (oNode == wxXml2EmptyNode) return false;
    //wxLogMessage(_T("[ProcessTag] NodeType=%d, Name='%s'"),
    //                oNode.GetType(), oNode.GetName() );

    if (oNode.GetType() != wxXML_ELEMENT_NODE) return false;

    wxString sElement = oNode.GetName();
    //wxString spaces(wxT(' '), m_xx);
    //wxLogMessage(spaces + _T("[ProcessTag] - element [%s]"), sElement.c_str());

    if (sElement == _T("bookinfo")) {
        return BookinfoTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("abstract")) {
        return AbstractTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("author")) {
        return AuthorTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("chapter")) {
        return ChapterTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("content")) {
        return ContentTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("copyright")) {
        return CopyrightTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("emphasis")) {
        return EmphasisTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("exercise")) {
        return ExerciseTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("imagedata")) {
        return ImagedataTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("itemizedlist")) {
        return ItemizedlistTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("holder")) {
        return HolderTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("leafletcontent")) {
        return LeafletcontentTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("legalnotice")) {
        return LegalnoticeTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("link")) {
        return LinkTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("listitem")) {
        return ListitemTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("orderedlist")) {
        return OrderedlistTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("para")) {
        return ParaTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("part")) {
        return PartTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("score")) {
        return ScoreTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("section")) {
        return SectionTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("simplelist")) {
        return SimplelistTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("theme")) {
        return ThemeTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("title")) {
        return TitleTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("titleabbrev")) {
        return TitleabbrevTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("tocimage")) {
        return TocimageTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("ulink")) {
        return UlinkTag(oNode, nOptions, pText);
    }
    else if (sElement == _T("year")) {
        return YearTag(oNode, nOptions, pText);
    }
    else {
        //check for exercises related param tags

        // a) Non-translatable params
        if (   (sElement == _T("accidentals"))
            || (sElement == _T("cadences"))
            || (sElement == _T("cadence_buttons"))
            || (sElement == _T("chords"))
            || (sElement == _T("clef"))
            || (sElement == _T("control_go_back"))
            || (sElement == _T("control_settings"))
            || (sElement == _T("fragment"))
            || (sElement == _T("inversions"))
            || (sElement == _T("key"))          //keys
            || (sElement == _T("keys"))
            || (sElement == _T("max_accidentals"))
            || (sElement == _T("max_interval"))
            || (sElement == _T("metronome"))
            || (sElement == _T("mode"))
            || (sElement == _T("music_border"))
            || (sElement == _T("problem_level"))
            || (sElement == _T("problem_type"))
            || (sElement == _T("play_key"))
            || (sElement == _T("play_mode"))
            || (sElement == _T("scales"))
            || (sElement == _T("score_type"))
            || (sElement == _T("show_key"))
            || (sElement == _T("time"))
           )
        {
            return ExerciseParamTag(oNode, false);
        }
        // b) translatable params
        else if ((sElement == _T("control_measures"))
              || (sElement == _T("control_play"))
              || (sElement == _T("control_solfa"))
           )
        {
            return ExerciseParamTag(oNode, true);
        }
        // c) special cases for translation
        else if (sElement == _T("music"))
        {
            return ExerciseMusicTag(oNode);
        }
        else {
            wxLogMessage(_T("Error: Found tag <%s>. No treatment defined."), sElement);
            return true;
        }
    }

}

wxString lmEbookProcessor::GetLibxml2Version() 
{
    return wxXml2::GetLibxml2Version(); 
}


//------------------------------------------------------------------------------------
// Tags' processors
// Processing model:
//  - Each tag processor receives Options. These specify where the output must go.
//      Options can be superseded when so specified in the options themselves
//  - Only input inside a <theme> tags go to html output. So HTML output is enabled
//      in <theme> tag and disabled when closing it
//------------------------------------------------------------------------------------


bool lmEbookProcessor::AbstractTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    m_sBookAbstract = wxEmptyString;
    return ProcessChildAndSiblings(oNode, eTRANSLATE, &m_sBookAbstract);
}

bool lmEbookProcessor::AuthorTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    m_sAuthorName = wxEmptyString;
    return ProcessChildAndSiblings(oNode, eTRANSLATE, &m_sAuthorName);
}

bool lmEbookProcessor::BookTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // receives and processes a <book> node and its children.
    // return true if error

    m_fIsLeaflet = false;
    m_aFilesToPack.Add( g_pPaths->GetLayoutPath() + _T("ebook_banner_left1.png"));
    m_aFilesToPack.Add( g_pPaths->GetLayoutPath() + _T("ebook_banner_right2.png"));
    m_aFilesToPack.Add( g_pPaths->GetLayoutPath() + _T("ebook_line_orange.png"));
    m_aFilesToPack.Add( g_pPaths->GetLayoutPath() + _T("navbg7.jpg"));
    m_aFilesToPack.Add( g_pPaths->GetLayoutPath() + _T("emusicbook_title.png"));

    //get book id and add it to the pages table. This id will be used for the
    //book cover page
    m_sBookId = oNode.GetPropVal(_T("id"), _T(""));
    if (m_sBookId == _T("")) {
        wxLogMessage(_T("Node <book> has no id property"));
    }

    // reset titles numbering counters
    m_nTitleLevel = -1;
    for (int i=0; i < lmMAX_TITLE_LEVEL; i++)
        m_nNumTitle[i] = 0;

    m_sBookTitleAbbrev = wxEmptyString;
    m_sBookTitle = wxEmptyString;

    //process tag's children
    bool fError = ProcessChildAndSiblings(oNode, nOptions, pText);

    return fError;

}

bool lmEbookProcessor::BookinfoTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // receives and processes a <bookinfo> node and its children.
    // return true if error

    // convert tag: output to open html tags
    // -- no output implied by this tag --

    // tag processing implications
    m_nHeaderLevel = 0;
    m_fProcessingBookinfo = true;
    m_fTitleToToc = true;
    m_nParentType = lmPARENT_BOOKINFO;

    //process tag's children
    bool fError = ProcessChildAndSiblings(oNode, nOptions, pText);

    //end tag: processing implications
    m_fProcessingBookinfo = false;

    // create the cover page
    if (!m_fIsLeaflet) CreateBookCover();

    return fError;

}

bool lmEbookProcessor::ChapterTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // convert tag: output to open html tags
    wxString sId = oNode.GetPropVal(_T("id"), _T(""));
    if (sId != _T(""))
        WriteToToc(_T("<entry id=\"") + sId + _T("\">\n"));
    else
        WriteToToc(_T("<entry>\n"));
    m_nTocIndentLevel++;
    IncrementTitleCounters();
 
    // tag processing implications
    m_fTitleToToc = true;
    m_nParentType = lmPARENT_CHAPTER;
    m_sChapterTitleAbbrev = wxEmptyString;
    m_sChapterTitle = wxEmptyString;

    //process tag's children
    bool fError = ProcessChildAndSiblings(oNode, nOptions, pText);

    //convert tag: output to close html tags
    m_nTocIndentLevel--;
    WriteToToc(_T("</entry>\n"));

    DecrementTitleCounters();
    m_sChapterTitleAbbrev = wxEmptyString;
    m_sChapterTitle = wxEmptyString;

    return fError;
}

bool lmEbookProcessor::ContentTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    //process tag's children
    return ProcessChildAndSiblings(oNode, nOptions, pText);
}

bool lmEbookProcessor::CopyrightTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    //process tag's children
    return ProcessChildAndSiblings(oNode, nOptions, pText);
}

bool lmEbookProcessor::EmphasisTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // openning tag
    if (nOptions & eHTML) WriteToHtml(_T(" <b>"));

    //process tag's children and write note content to html
    bool fError = ProcessChildAndSiblings(oNode, nOptions, pText);
    
    //convert tag
    if (nOptions & eHTML) WriteToHtml(_T("</b>"));

    return fError;
}

bool lmEbookProcessor::ExerciseTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // get attributes
    wxString sType = oNode.GetPropVal(_T("type"), _T(""));
    wxString sWidth = oNode.GetPropVal(_T("width"), _T(""));
    wxString sHeight = oNode.GetPropVal(_T("height"), _T(""));
    wxString sBorder = oNode.GetPropVal(_T("border"), _T(""));

    // convert tag: output to open html tags
    WriteToHtml(_T("<object type=\"Application/LenMus\" classid=\"") + sType +
        _T("\" width=\"") + sWidth +
        _T("\" height=\"") + sHeight +
        _T("\" border=\"") + sBorder +
        _T("\">\n") );
 
    //process tag's children. Output to HTML, do not attach text
    bool fError = ProcessChildAndSiblings(oNode, eHTML);

    //convert tag: output to close html tags
    WriteToHtml(_T("</object>\n"));

    return fError;
}

bool lmEbookProcessor::ExerciseParamTag(const wxXml2Node& oNode, bool fTranslate)
{
    // convert tag
    WriteToHtml(_T("<param name=\"") + oNode.GetName() + _T("\" value=\"") );

    // complete link address
    if (oNode.GetName() == _T("control_go_back")) {
        wxFileName oFN(m_sFilename);
        WriteToHtml(oFN.GetName() + _T("_"));
    }

    //params have no more xml content, just the value. Get it
    bool fError = ProcessChildAndSiblings(oNode, eHTML | (fTranslate ? eTRANSLATE : 0));

    // complete link address
    if (oNode.GetName() == _T("control_go_back")) {
        WriteToHtml(_T(".htm"));
    }
    WriteToHtml(_T("\">\n"));

    return fError;
}

bool lmEbookProcessor::ExerciseMusicTag(const wxXml2Node& oNode)
{
    // convert tag
    WriteToHtml(_T("<param name=\"") + oNode.GetName() + _T("\" value=\"") );

    //params have no more xml content, just the value. Get it
    wxString sMusic;
    bool fError = ProcessChildAndSiblings(oNode, 0, &sMusic);

    //locate all translatable strings (start with underscore)
    wxString sNoTrans, sTrans;
    int iStart, iEnd, nQuoteLength;

    iStart = sMusic.Find(_T("_&quot;"));
    nQuoteLength = 7;   //length of string "_&quot;"
    if (iStart == wxNOT_FOUND) {
        iStart = sMusic.Find(_T("_''"));
        nQuoteLength = 3;   //length of string "_''"
    }

    while (sMusic.Length() > 0 && iStart != wxNOT_FOUND)
    {
        if (iStart != wxNOT_FOUND) {
            //start of translatable string
            sNoTrans = sMusic.Left(iStart);
            WriteToHtml( sNoTrans );

            sMusic = sMusic.Mid(iStart+nQuoteLength);

            // find end of string
            iEnd = sMusic.Find(_T("&quot;"));
            nQuoteLength = 6;   //length of string "&quot;"
            if (iEnd == wxNOT_FOUND) {
                iEnd = sMusic.Find(_T("''"));
                nQuoteLength = 2;       //length of string "''"
            }

            //end of translatable string
            sTrans = sMusic.Left(iEnd);
            if (m_fOnlyLangFile) {
                //todo: filter out strings containing only numbers, spaces and symbols but
                //      no characters
                WriteToLang(sTrans);
            }
            sTrans = ::wxGetTranslation(sTrans);
            WriteToHtml( _T("''") + sTrans + _T("''") );

            //remaining
            sMusic = sMusic.Mid(iEnd+nQuoteLength);
            iStart = sMusic.Find(_T("_&quot;"));
            nQuoteLength = 7;   //length of string "&quot;"
            if (iStart == wxNOT_FOUND) {
                iStart = sMusic.Find(_T("_''"));
                nQuoteLength = 3;   //length of string "''"
            }
        }
    }

    WriteToHtml( sMusic + _T("\">\n"));

    return fError;
}

bool lmEbookProcessor::HolderTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    m_sCopyrightHolder = wxEmptyString;
    return ProcessChildAndSiblings(oNode, eTRANSLATE, &m_sCopyrightHolder);
}

bool lmEbookProcessor::ImagedataTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // get attributes
    wxString sFileref = oNode.GetPropVal(_T("fileref"), _T(""));
    wxString sAlign = oNode.GetPropVal(_T("align"), _T(""));
    wxString sValign = oNode.GetPropVal(_T("valign"), _T(""));
    if (sFileref == _T("")) {
        LogError(_T("Node <image> has no 'fileref' property"));
        return true;    //error
    }

    // convert tag: output to open html tags
    if (nOptions & eHTML) {
        WriteToHtml( _T("<img src=\"") + sFileref + _T("\"") );
        if (sAlign != _T("")) WriteToHtml( _T(" align=\"") + sAlign + _T("\"") );
        if (sValign != _T("")) WriteToHtml( _T(" valign=\"") + sValign + _T("\"") );
        WriteToHtml( _T(" >\n") );
    }

    //add to list of files to pack in lmb file
    wxFileName oFN(m_sFilename);
    oFN.AppendDir(_T("figures"));
    oFN.SetFullName( sFileref );
    m_aFilesToPack.Add(oFN.GetFullPath());

    return false;   //no error
}

bool lmEbookProcessor::ItemizedlistTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // openning tag
    WriteToHtml(_T("<ul>\n"));

    // tag processing implications
    // no

    //process tag's children
    bool fError = ProcessChildAndSiblings(oNode, nOptions, pText);

    // closing tag
    WriteToHtml(_T("</ul>\n"));

    return fError;
}

bool lmEbookProcessor::LeafletTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // receives and processes a <leaflet> root node and its children.
    // return true if error

    m_fIsLeaflet = true;
    m_aFilesToPack.Add( g_pPaths->GetLayoutPath() + _T("ebook_banner_left1.png"));
    m_aFilesToPack.Add( g_pPaths->GetLayoutPath() + _T("ebook_line_orange.png"));
    //m_aFilesToPack.Add( g_pPaths->GetLayoutPath() + _T("leaflet_banner_right.png"));
    m_aFilesToPack.Add( g_pPaths->GetLayoutPath() + _T("leaflet_banner_right_") +
        m_sLangCode + _T(".png"));

    //get book id and add it to the pages table. This id will be used for the
    //book cover page
    m_sBookId = oNode.GetPropVal(_T("id"), _T(""));
    if (m_sBookId == _T("")) {
        LogError(_T("Node <leaflet> has no id property"));
    }

    // reset titles numbering counters
    m_nTitleLevel = -1;
    for (int i=0; i < lmMAX_TITLE_LEVEL; i++)
        m_nNumTitle[i] = 0;

    m_sBookTitleAbbrev = wxEmptyString;
    m_sBookTitle = wxEmptyString;

    //process tag's children
    bool fError = ProcessChildAndSiblings(oNode, nOptions, pText);

    return fError;

}

bool lmEbookProcessor::LeafletcontentTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // get its 'id' and 'header' properties
    wxString sId = oNode.GetPropVal(_T("id"), _T(""));
    wxString sHeader = oNode.GetPropVal(_T("header"), _T(""));
    wxString sToToc = oNode.GetPropVal(_T("toc"), _T("yes"));
    m_fThemeInToc = false;

    // openning tag
    m_sThemeTitleAbbrev = wxEmptyString;
    // HTML:
    StartHtmlFile(m_sFilename, sId);
    // TOC
    WriteToToc(_T("<coverpage>") + m_sHtmlPagename + _T("</coverpage>\n"));
 
    // tag processing implications
    m_nHeaderLevel = 1;
    m_fTitleToToc = false;
    m_nParentType = lmPARENT_THEME;

    //process tag's children
    bool fError = ProcessChildAndSiblings(oNode, nOptions | eHTML, pText);

    //closing tag:
    // HTML:
    TerminateLeafletFile();    // Close previous html page
    // TOC:

    return fError;
}

bool lmEbookProcessor::LegalnoticeTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    m_sLegalNotice = wxEmptyString;
    return ProcessChildAndSiblings(oNode, eTRANSLATE, &m_sLegalNotice);
}

bool lmEbookProcessor::LinkTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // openning tag

    // get its 'linkend' property and find the associated page
    wxString sId = oNode.GetPropVal(_T("linkend"), _T(""));
    if (sId != _T("")) {
        wxFileName oFN( m_sFilename );
        wxString sName = oFN.GetName() + _T("_") + sId;
        oFN.SetName(sName);
        oFN.SetExt(_T("htm"));
        wxString sLink = _T("<a href=\"#LenMusPage/") + oFN.GetFullName() + _T("\">");
        WriteToHtml( sLink );
    }
    else {
        WriteToHtml( _T("<a href=\"#\">") );
    }

    //process tag's children and write note content to html
    bool fError = ProcessChildAndSiblings(oNode, eHTML | eTRANSLATE);

    // closing tag
    if (sId != _T("")) WriteToHtml(_T("</a>"));

    return fError;
}

bool lmEbookProcessor::ListitemTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // openning tag
    WriteToHtml(_T("<li>"));

    // tag processing implications

    //process tag's children and write note content to html
    bool fError = ProcessChildAndSiblings(oNode, eHTML | eTRANSLATE);

    // closing tag
    WriteToHtml(_T("</li>\n"));

    return fError;
}

bool lmEbookProcessor::OrderedlistTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // openning tag
    WriteToHtml(_T("<ol>"));

    // tag processing implications

    //process tag's children and write note content to html
    bool fError = ProcessChildAndSiblings(oNode, nOptions, pText);

    // closing tag
    WriteToHtml(_T("</ol>\n"));

    return fError;
}

bool lmEbookProcessor::ParaTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // get attributes
    wxString sRole = oNode.GetPropVal(_T("role"), _T(""));

    // openning tag
    if (nOptions & eHTML) {
        if (sRole == _T("center")) WriteToHtml(_T("<center>"));
        WriteToHtml(_T("<p>"));
    }

    //process tag's children and write note content to html
    if (pText) *pText += _T("<p>");
    bool fError = ProcessChildAndSiblings(oNode, nOptions | eTRANSLATE, pText);
    if (pText) *pText += _T("</p>\n");
    
    //convert tag: output to close html tags
    if (nOptions & eHTML) {
        WriteToHtml(_T("</p>\n"));
        if (sRole == _T("center")) WriteToHtml(_T("</center>"));
    }

    return fError;
}

bool lmEbookProcessor::PartTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // openning tag
    if (m_fThemeInToc) IncrementTitleCounters();
    // HTML:
    wxString sId = oNode.GetPropVal(_T("id"), _T(""));
    if (sId != _T(""))
        WriteToHtml(_T("<div id=\"") + sId + _T("\">\n"));
    else
        WriteToHtml(_T("<div>\n"));
    // TOC:
    // no toc output
 
    // tag processing implications
    m_fTitleToToc = false;
    m_nParentType = lmPARENT_PART;
    m_nHeaderLevel++;

    //process tag's children
    bool fError = ProcessChildAndSiblings(oNode, nOptions, pText);

    //closing tag:
    // HTML:
    WriteToHtml(_T("</div>\n"));
    // TOC:
    // no toc content
    // processing implications:
    m_nHeaderLevel--;

    if (m_fThemeInToc) DecrementTitleCounters();
    return fError;
}

bool lmEbookProcessor::ScoreTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // get attributes
    wxString sWidth = oNode.GetPropVal(_T("width"), _T(""));
    wxString sHeight = oNode.GetPropVal(_T("height"), _T(""));
    wxString sBorder = oNode.GetPropVal(_T("border"), _T(""));

    // convert tag: output to open html tags
    WriteToHtml(_T("<object type=\"Application/LenMus\" classid=\"Score")
        _T("\" width=\"") + sWidth +
        _T("\" height=\"") + sHeight +
        _T("\" border=\"") + sBorder +
        _T("\">\n") );
 
    //process tag's children
    bool fError = ProcessChildAndSiblings(oNode, eHTML);

    //convert tag: output to close html tags
    WriteToHtml(_T("</object>\n"));

    return fError;
}

bool lmEbookProcessor::SectionTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // openning tag
    wxString sId = oNode.GetPropVal(_T("id"), _T(""));
    if (sId != _T(""))
        WriteToToc(_T("<entry id=\"") + sId + _T("\">\n"));
    else
        WriteToToc(_T("<entry>\n"));
    m_nTocIndentLevel++;
    IncrementTitleCounters();
 
    // tag processing implications
    m_fTitleToToc = true;
    m_nParentType = lmPARENT_SECTION;
    m_sChapterTitleAbbrev = wxEmptyString;
    m_sChapterTitle = wxEmptyString;

    //process tag's children
    bool fError = ProcessChildAndSiblings(oNode, nOptions, pText);

    //convert tag: output to close tags
    m_nTocIndentLevel--;
    WriteToToc(_T("</entry>\n"));

    DecrementTitleCounters();

    return fError;
}

bool lmEbookProcessor::SimplelistTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // openning tag
    WriteToHtml(_T("<ul>"));

    // tag processing implications

    //process tag's children and write note content to html
    bool fError = ProcessChildAndSiblings(oNode, nOptions, pText);

    // closing tag
    WriteToHtml(_T("</ul>\n"));

    return fError;
}

bool lmEbookProcessor::ThemeTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // get its 'id' and 'header' properties
    wxString sId = oNode.GetPropVal(_T("id"), _T(""));
    wxString sHeader = oNode.GetPropVal(_T("header"), _T(""));
    wxString sToToc = oNode.GetPropVal(_T("toc"), _T("yes"));
    m_fThemeInToc = (sToToc != _T("no"));

    // openning tag
    m_sThemeTitleAbbrev = wxEmptyString;
    if (m_fThemeInToc) IncrementTitleCounters();
    // HTML:
    StartHtmlFile(m_sFilename, sId);
    // TOC
    if (m_fThemeInToc) {
        if (sId != _T(""))
            WriteToToc(_T("<entry id=\"") + sId + _T("\">\n"));
        else
            WriteToToc(_T("<entry>\n"));
        m_nTocIndentLevel++;
        WriteToToc(_T("<page>") + m_sHtmlPagename + _T("</page>\n"));
    }
 
    // tag processing implications
    m_nHeaderLevel = 1;
    m_fTitleToToc = true;
    m_nParentType = lmPARENT_THEME;

    //process tag's children
    bool fError = ProcessChildAndSiblings(oNode, nOptions | eHTML, pText);

    //closing tag:
    // HTML:
    TerminateHtmlFile();    // Close previous html page
    // TOC:
    if (m_fThemeInToc) {
        m_nTocIndentLevel--;
        WriteToToc(_T("</entry>\n"));
        DecrementTitleCounters();
    }

    return fError;
}

bool lmEbookProcessor::TitleTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // openning tag
    wxString sTitleNum = wxEmptyString;
    bool fTitleToToc = ((m_nParentType == lmPARENT_BOOKINFO) && m_fTitleToToc) ||
                       ((m_nParentType == lmPARENT_CHAPTER) && m_fTitleToToc) ||
                       ((m_nParentType == lmPARENT_SECTION) && m_fTitleToToc) ||
                       ((m_nParentType == lmPARENT_THEME) && m_fThemeInToc);

    if (fTitleToToc) {
        sTitleNum = GetTitleCounters();
        if (m_nParentType != lmPARENT_BOOKINFO)
            WriteToToc(_T("<titlenum>") + sTitleNum + _T("</titlenum>\n"));
        WriteToToc(_T("<title>"));
    }

    //process tag's children and write title content to toc
    wxString sTitle;
    bool fError = ProcessChildAndSiblings(oNode, eTRANSLATE, &sTitle);

    //save the title
    if (m_nParentType == lmPARENT_BOOKINFO) {
        m_sBookTitle = sTitle;
    }
    else if (m_nParentType == lmPARENT_CHAPTER) {
        m_sChapterTitle = sTitle;
        m_sChapterNum = sTitleNum;
    }
    else if (m_nParentType == lmPARENT_THEME) {
        //determine which title to use for page headers
        wxString sHeaderTitle;
        if (m_sChapterTitleAbbrev != wxEmptyString)
            sHeaderTitle = m_sChapterTitleAbbrev;
        else if (m_sChapterTitle != wxEmptyString)
            sHeaderTitle = m_sChapterTitle;
        else if (m_sThemeTitleAbbrev != wxEmptyString)
            sHeaderTitle = m_sThemeTitleAbbrev;
        else
            sHeaderTitle = sTitle;

        // replace %d by parent number or preceed header by parent number
        wxString sParentNum = GetParentNumber();
        if (sHeaderTitle.Find(_T("%d")) != wxNOT_FOUND)
            sHeaderTitle.Replace(_T("%d"), sParentNum);
        else
            sHeaderTitle = sParentNum + _T(". ") + sHeaderTitle;

        //create page headers
        if (sParentNum == wxEmptyString) sParentNum = sTitleNum;
        if (!m_fIsLeaflet)
            CreatePageHeaders(m_sBookTitle, sHeaderTitle, sParentNum);
        else
            CreateLeafletHeaders(m_sBookTitle, sHeaderTitle, sParentNum);
    }
    //write title to html file
    WriteToHtml( wxString::Format(_T("<h%d>"), m_nHeaderLevel));
    WriteToHtml( sTitleNum + sTitle );
    WriteToHtml( wxString::Format(_T("</h%d>\n"), m_nHeaderLevel));

    // End of tag processing implications
    //TOC:
    if (fTitleToToc) {
        WriteToToc(sTitle + _T("</title>\n"), ltNO_INDENT );
    }

    return fError;
}

bool lmEbookProcessor::TitleabbrevTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    //process tag's children. Do not write content
    wxString sTitle;
    bool fError = ProcessChildAndSiblings(oNode, eTRANSLATE, &sTitle);
    //WriteToLang(sTitle);
    //sTitle = ::wxGetTranslation(sTitle);

    // get theme number
    int nTheme = m_nNumTitle[0];

    //save the title
    if (m_nParentType == lmPARENT_BOOKINFO) {
        m_sBookTitleAbbrev = sTitle;
    }
    else if (m_nParentType == lmPARENT_CHAPTER) {
        m_sChapterTitleAbbrev = sTitle;
    }
    else if (m_nParentType == lmPARENT_THEME) {
        m_sThemeTitleAbbrev = sTitle;
    }
    return fError;
}

bool lmEbookProcessor::TocimageTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    //process tag's children. Do not write content
    wxString sImageName;
    bool fError = ProcessChildAndSiblings(oNode, 0, &sImageName);

    //add to list of files to pack in lmb file
    m_aFilesToPack.Add(sImageName);

    // End of tag processing implications
    //TOC:
    wxFileName oFN(sImageName);
    WriteToToc(_T("<image>") + oFN.GetFullName() + _T("</image>\n"));

    return fError;
}

bool lmEbookProcessor::UlinkTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    // openning tag

    // get its 'url' property and find the associated page
    wxString sUrl = oNode.GetPropVal(_T("url"), _T(""));
    wxString sLink;
    if (sUrl != _T("")) {
        sLink = _T(" <a href=\"") + sUrl + _T("\">");
    }
    else {
        sLink = _T(" <a href=\"#\">");
    }
    if (pText) 
        *pText += sLink;
    else
        if (nOptions & eHTML) WriteToHtml( sLink );

    //process tag's children and write note content to html
    bool fError = ProcessChildAndSiblings(oNode, nOptions, pText);

    // closing tag
    sLink = _T("</a>");
    if (pText) 
        *pText += sLink;
    else
        if (nOptions & eHTML) WriteToHtml( sLink );

    return fError;
}

bool lmEbookProcessor::YearTag(const wxXml2Node& oNode, int nOptions, wxString* pText)
{
    m_sCopyrightYear = wxEmptyString;
    return ProcessChildAndSiblings(oNode, 0, &m_sCopyrightYear);
}


//
// Auxiliary
//

wxString lmEbookProcessor::GetTitleCounters()
{
    // Returns current theme number. For example, if current
    // theme is '2.4.7' this method returns '2.4.7. '

    wxString sTitleNum = wxEmptyString;
    for (int i=0; i <= m_nTitleLevel; i++) {
        sTitleNum += wxString::Format(_T("%d."), m_nNumTitle[i] );
    }
    if (sTitleNum != wxEmptyString) sTitleNum += _T(" ");
    
    return  sTitleNum;

}

wxString lmEbookProcessor::GetParentNumber()
{
    // Returns current theme number minus one level. For example, if current
    // theme is '2.4.7' this method returns '2.4'

    wxString sTitleNum = wxEmptyString;
    if (m_nTitleLevel >= 0) {
        sTitleNum = wxString::Format(_T("%d"), m_nNumTitle[0] );
        for (int i=1; i < m_nTitleLevel; i++) {
            sTitleNum += wxString::Format(_T(".%d"), m_nNumTitle[i] );
        }
    }
    else {
        // title level is < 0. This is because it is a theme not in toc.
        // lets return previous theme number
        sTitleNum = wxString::Format(_T("%d"), m_nNumTitle[0] );
    }
    
    return  sTitleNum;

}


void lmEbookProcessor::IncrementTitleCounters()
{
    m_nTitleLevel++;
    if ( m_nTitleLevel >= lmMAX_TITLE_LEVEL) {
        wxLogMessage(_T("Too many nested levels for sections/themes/parts"));
        m_nTitleLevel--;
    }
    m_nNumTitle[m_nTitleLevel]++;

    for (int i=m_nTitleLevel+1; i < lmMAX_TITLE_LEVEL; i++) {
        m_nNumTitle[i] = 0;
    }
}

void lmEbookProcessor::DecrementTitleCounters()
{
    m_nTitleLevel--;
    if (m_nTitleLevel < 0) return;

}

void lmEbookProcessor::CreateBookCover()
{
    StartHtmlFile(m_sFilename, _T("cover"));

    //Generate header
    wxString sNil = _T("");
    wxString sHtml = sNil +
        _T("<html>\n<head>\n")
        _T("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=") + m_sCharCode +
        _T("\">\n")
        _T("<title>") + m_sBookTitle + _T("</title>\n")
        _T("</head>\n\n");

    WriteToHtml(sHtml);

    //initializations
    m_nHtmlIndentLevel = 1;
    //WriteToHtml(
    //    _T("<body bgcolor='#808080'>\n")
    //    _T("\n")
    //    _T("<center>\n")
    //    _T("<table width='720' bgcolor='#ffffff' cellpadding='0' cellspacing='0'>\n")
    //    _T("<tr><td bgcolor='#ffffff'>\n")
    //    _T("\n")
    //    _T("<!-- banner -->\n")
    //    _T("<table width='100%' cellpadding='0' cellspacing='0'>\n")
    //    _T("<tr><td width='300'><img src='ebook_banner_left1.png'></td>\n")
    //    _T("<td bgcolor='#7f8adc'>&nbsp;</td>\n")
    //    _T("<td width='250'><img src='ebook_banner_right2.png'></td></tr>\n")
    //    _T("<tr><td bgcolor='#ff8800' colspan='3'>.</td></tr>\n")
    //    _T("</table>\n\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("<center>\n")
    //    _T("<table width='70%'>\n")
    //    _T("<tr><td>\n")
	   //     _T("<h1>") + m_sBookTitle + _T("</h1></td></tr>\n")
    //    _T("<tr><td>\n") + m_sBookAbstract +
	   //     _T("</td></tr>\n")
    //    _T("<tr><td></td></tr>\n")
    //    _T("<tr><td></td></tr>\n")
    //    _T("</table>\n")
    //    _T("\n")
    //    _T("</center>\n")
    //    _T("</font>\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("\n")
    //    _T("<blockquote>\n")
    //    _T("<font size='-1' face='Arial'>\n")
	   //     _T("<p><b>Copyright � ") + m_sCopyrightYear + _T(" ") + m_sCopyrightHolder 
    //        + _T("</b></p>\n")
    //    _T("\n") + m_sLegalNotice +
    //    _T("</blockquote>\n")
    //    _T("\n")
    //    _T("\n")
    //    _T("</font>\n")
    //    _T("\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("</td></tr></table>\n")
    //    _T("</body>\n")
    //    _T("</html>\n") );

    WriteToHtml(
        _T("<body bgcolor='#808080'>\n")
        _T("<center>\n")
        _T("<table width='720' bgcolor='#ffffff' cellpadding='0' cellspacing='0'>\n")
        _T("<tr><td bgcolor='#ffffff'>\n")
        _T("\n")
        _T("<!-- banner -->\n")
        _T("<table width='100%' cellpadding='0' cellspacing='0'>\n")
        _T("<tr><td bgcolor='#7f8adc' width='23%' valign='top'><img src='navbg7.jpg'></td>\n")
	    _T("    <td bgcolor='#ffffff' width='6%'>&nbsp;</td>\n")
        _T("<td valign='top' bgcolor='#ffffff'>\n")
        _T("\n")
        _T("<!-- content -->\n")
        _T("<table width='100%' cellpadding='0' cellspacing='0' border='0'>\n")
        _T("<tr><td colspan='2' valign='top' align='right'><img src='emusicbook_title.png'></td></tr>\n")
        _T("<tr><td>\n")
	    _T("    <br /><br /><br /><br />\n")
	    _T("    <br /><br /><br /><br />\n")
        _T("</td><td width='30'>&nbsp;</td></tr>\n")
        _T("<tr><td><h1>") + m_sBookTitle + _T("</h1></td><td>&nbsp;</td></tr>\n")
        _T("<tr><td><h3>") + m_sAuthorName + _T("</h3></td><td>&nbsp;</td></tr>\n")
        _T("<tr><td>\n") + m_sBookAbstract +
	        _T("</td><td>&nbsp;</td></tr>\n")
        _T("\n")
        _T("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n")
        _T("<tr><td>\n")
        _T("    <font size='-1' face='Arial'>\n")
	    _T("    <p><b>Copyright � ") + m_sCopyrightYear + _T(" ") + m_sCopyrightHolder +
            _T("</b></p>\n")
        _T("\n") + m_sLegalNotice +
        _T("    </font>\n")
        _T("</td><td>&nbsp;</td></tr>\n")
        _T("</table>\n")
        _T("</td></tr>\n")
        _T("</table>\n")
        _T("</td></tr></table>\n")
        _T("</body>\n")
        _T("</html>\n") );

    //    _T("<body bgcolor='#808080'>\n")
    //    _T("\n")
    //    _T("<center>\n")
    //    _T("<table width='720' bgcolor='#ffffff' cellpadding='0' cellspacing='0'>\n")
    //    _T("<tr><td bgcolor='#ffffff'>\n")
    //    _T("\n")
    //    _T("<!-- banner -->\n")
    //    _T("<table width='100%' cellpadding='0' cellspacing='0'>\n")
    //    _T("<tr><td width='300'><img src='ebook_banner_left1.png'></td>\n")
    //    _T("<td bgcolor='#7f8adc'>&nbsp;</td>\n")
    //    _T("<td width='250'><img src='ebook_banner_right2.png'></td></tr>\n")
    //    _T("<tr><td bgcolor='#ff8800' colspan='3'>.</td></tr>\n")
    //    _T("</table>\n\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("<center>\n")
    //    _T("<table width='70%'>\n")
    //    _T("<tr><td>\n")
	   //     _T("<h1>") + m_sBookTitle + _T("</h1></td></tr>\n")
    //    _T("<tr><td>\n") + m_sBookAbstract +
	   //     _T("</td></tr>\n")
    //    _T("<tr><td></td></tr>\n")
    //    _T("<tr><td></td></tr>\n")
    //    _T("</table>\n")
    //    _T("\n")
    //    _T("</center>\n")
    //    _T("</font>\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("\n")
    //    _T("<blockquote>\n")
    //    _T("<font size='-1' face='Arial'>\n")
	   //     _T("<p><b>Copyright � ") + m_sCopyrightYear + _T(" ") + m_sCopyrightHolder 
    //        + _T("</b></p>\n")
    //    _T("\n") + m_sLegalNotice +
    //    _T("</blockquote>\n")
    //    _T("\n")
    //    _T("\n")
    //    _T("</font>\n")
    //    _T("\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("<br /><br /><br /><br />\n")
    //    _T("</td></tr></table>\n")
    //    _T("</body>\n")
    //    _T("</html>\n") );

    CloseHtmlFile();

    //prepare page name
    wxFileName oHTM( m_sFilename );
    wxString sName = oHTM.GetName() + _T("_cover.htm");

    //write it to toc
    WriteToToc(_T("<coverpage>") + sName + _T("</coverpage>\n"));
}

//------------------------------------------------------------------------------------
// File managing
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
// Toc file
//------------------------------------------------------------------------------------

bool lmEbookProcessor::StartTocFile(wxString sFilename)
{
    // returns false if error

    wxFileName oTOC( sFilename );
    oTOC.SetExt(_T("toc"));
    m_sTocFilename = oTOC.GetFullName();
    m_pTocFile = new wxFile(m_sTocFilename, wxFile::write);
    if (!m_pTocFile->IsOpened()) {
        wxLogMessage(_T("Error: File %s can not be created"), oTOC.GetFullName());
        return false;        //error
    }

    //initializations
    m_nTocIndentLevel = 0;

    //Generate header
    wxString sNil = _T("");
    wxString sHeader = sNil +
        _T("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n")
        _T("<lmBookTOC>\n");

    WriteToToc(sHeader);
    m_nTocIndentLevel++;

    return true;

}

void lmEbookProcessor::TerminateTocFile()
{
    if (!m_pTocFile) return;

    wxString sNil = _T("");
    wxString sHeader = sNil +
        _T("</lmBookTOC>\n");

    WriteToToc(sHeader);
    m_pTocFile->Close();

    delete m_pTocFile;
    m_pTocFile = (wxFile*) NULL;

}

void lmEbookProcessor::WriteToToc(wxString sText, bool fIndent)
{
    if (!m_pTocFile) return;

    if (fIndent) {
        wxString sIndent;
        sIndent.Append(_T(' '), 3 * m_nTocIndentLevel);
        m_pTocFile->Write(sIndent + sText);
    }
    else {
        m_pTocFile->Write(sText);
    }

}

//------------------------------------------------------------------------------------
// Html file managing
//------------------------------------------------------------------------------------

bool lmEbookProcessor::StartHtmlFile(wxString sFilename, wxString sId)
{
    // returns false if error

    if (sId == _T("")) return false;
    wxFileName oHTM( sFilename );
    wxString sName = oHTM.GetName() + _T("_") + sId;

    oHTM.SetName(sName);
    oHTM.SetExt(_T("htm"));
    m_sHtmlPagename = oHTM.GetFullName();

    if (m_fGenerateLmb) {
        m_pZipFile->PutNextEntry( m_sHtmlPagename );
    }
    else {
        m_pHtmlFile = new wxFile(oHTM.GetFullName(), wxFile::write);
        if (!m_pHtmlFile->IsOpened()) {
            wxLogMessage(_T("Error: File %s can not be created"), oHTM.GetFullName());
            return false;        //error
        }
    }


    return true;

}

void lmEbookProcessor::TerminateHtmlFile()
{
    if (!((m_fGenerateLmb && m_pLmbFile) || m_pHtmlFile)) return;

    // Write footer
    WriteToHtml(
        _T("</td><td bgcolor='#ffffff' width='20'></td></tr></table>\n")
        _T("\n")
        _T("<br /><br /><br /><br />\n")
        _T("<br /><br /><br /><br />\n")
        _T("<br /><br /><br /><br />\n")
        _T("<br /><br /><br /><br />\n")
        _T("<br /><br /><br /><br />\n")
        _T("<table width='100%' cellpadding='0' cellspacing='0'>\n")
        _T("<tr><td bgcolor='#ff8800'><img src='ebook_line_orange.png'></td></tr>\n")
        _T("<tr><td bgcolor='#7f8adc' align='center'>\n")
        _T("    <font size='-1' color='#ffffff'><br /><br />\n"));
     WriteToHtml( ::wxGetTranslation(m_sFooter1) );
	 WriteToHtml( _T("<br />\n") );
     WriteToHtml( ::wxGetTranslation(m_sFooter2) );
     WriteToHtml( _T("<br />\n")
	    _T("    </font>\n")
        _T("</td></tr>\n")
        _T("</table>\n")
        _T("\n")
        _T("</td></tr></table></center>\n")
        _T("\n")
        _T("</body>\n")
        _T("</html>\n") );

     //   _T("<br /><br /><br /><br />\n")
     //   _T("<table width='100%' cellpadding='0' cellspacing='0'>\n")
     //   _T("<tr><td bgcolor='#ff8800'><img src='ebook_line_orange.png'></td></tr>\n")
     //   _T("<tr><td bgcolor='#7f8adc' align='center'>\n")
     //   _T("    <font size='-1' color='#ffffff'><br /><br />\n") + m_sFooter1 +
	    //_T("<br />\n") + m_sFooter2 +
     //   _T("<br />\n")
	    //_T("    </font>\n")
     //   _T("</td></tr>\n")
     //   _T("</table>\n"));

    //WriteToHtml(_T("\n</body>\n"));

    CloseHtmlFile();

}

void lmEbookProcessor::TerminateLeafletFile()
{
    if (!((m_fGenerateLmb && m_pLmbFile) || m_pHtmlFile)) return;

    // Write footer
    WriteToHtml(
        _T("</td><td bgcolor='#ffffff' width='10'></td></tr></table>\n")
        _T("\n")
        _T("<br /><br /><br /><br />\n")
        _T("<br /><br /><br /><br />\n")
        _T("<br /><br /><br /><br />\n")
        _T("<br /><br /><br /><br />\n")
        _T("<br /><br /><br /><br />\n")
        _T("<table width='100%' cellpadding='0' cellspacing='0'>\n")
        _T("<tr><td bgcolor='#ff8800'><img src='ebook_line_orange.png'></td></tr>\n")
        _T("<tr><td bgcolor='#7f8adc' align='center'>\n")
        _T("    <font size='-1' color='#ffffff'><br /><br />\n"));
     WriteToHtml( ::wxGetTranslation(m_sFooter1) );
	 WriteToHtml( _T("<br />\n") );
     WriteToHtml( ::wxGetTranslation(m_sFooter2) );
     WriteToHtml( _T("<br />\n")
	    _T("    </font>\n")
        _T("</td></tr>\n")
        _T("</table>\n")
        _T("\n")
        _T("</td></tr></table></center>\n")
        _T("\n")
        _T("</body>\n")
        _T("</html>\n") );

     //   _T("<br /><br /><br /><br />\n")
     //   _T("<table width='100%' cellpadding='0' cellspacing='0'>\n")
     //   _T("<tr><td bgcolor='#ff8800'><img src='ebook_line_orange.png'></td></tr>\n")
     //   _T("<tr><td bgcolor='#7f8adc' align='center'>\n")
     //   _T("    <font size='-1' color='#ffffff'><br /><br />\n") + m_sFooter1 +
	    //_T("<br />\n") + m_sFooter2 +
     //   _T("<br />\n")
	    //_T("    </font>\n")
     //   _T("</td></tr>\n")
     //   _T("</table>\n"));

    //WriteToHtml(_T("\n</body>\n"));

    CloseHtmlFile();

}

void lmEbookProcessor::CloseHtmlFile()
{
    if (!((m_fGenerateLmb && m_pLmbFile) || m_pHtmlFile)) return;
    if (m_pHtmlFile) {
        m_pHtmlFile->Close();
        delete m_pHtmlFile;
        m_pHtmlFile = (wxFile*) NULL;
    }
    else {
        m_pZipFile->CloseEntry();
    }

}

void lmEbookProcessor::WriteToHtml(wxString sText)
{
    if (!((m_fGenerateLmb && m_pLmbFile) || m_pHtmlFile)) return;

    //wxString sIndent;
    //sIndent.Append(_T(' '), 3 * m_nHtmlIndentLevel);
    //m_pHtmlFile->Write(sIndent + sText);

    if (m_fGenerateLmb) {
        m_pLmbFile->WriteString( sText );
    }
    else {
        m_pHtmlFile->Write(sText);
    }
}

void lmEbookProcessor::CreatePageHeaders(wxString sBookTitle, wxString sHeaderTitle,
                                         wxString sTitleNum)
{
    if (!((m_fGenerateLmb && m_pLmbFile) || m_pHtmlFile)) return;

    //create page headers
    WriteToHtml(
        _T("<html>\n<head>\n")
        _T("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=") );
    WriteToHtml(m_sCharCode + _T("\">\n<title>") );
    WriteToHtml( ::wxGetTranslation(sBookTitle) );
    WriteToHtml(_T(": "));
    WriteToHtml( ::wxGetTranslation(sHeaderTitle) );
    WriteToHtml(_T("</title>\n"));
    WriteToHtml(_T("</head>\n\n"));

    m_nHtmlIndentLevel = 1;     //indent

    WriteToHtml(
        _T("<body bgcolor='#808080'>\n")
        _T("\n")
        _T("<center>\n")
        _T("<table width='720' bgcolor='#ffffff' cellpadding='0' cellspacing='0'>\n")
        _T("<tr><td bgcolor='#ffffff'>\n")
        _T("\n")
        _T("<table width='100%' cellpadding='0' cellspacing='0'>\n")
        _T("<tr><td bgcolor='#7f8adc' align='left'>\n")
        _T("	<font size='-1' color='#ffffff'>&nbsp;&nbsp;") );
    WriteToHtml( ::wxGetTranslation(sBookTitle) );
    WriteToHtml(
        _T("</font></td></tr>\n")
        _T("<tr><td bgcolor='#7f8adc' align='right'><br />\n")
        _T("	<b><font size='+4' color='#ffffff'>") );
    WriteToHtml( ::wxGetTranslation(sHeaderTitle) );
    WriteToHtml(
        _T("&nbsp;</font></b><br /></td></tr>\n")
        _T("<tr><td bgcolor='#ff8800'><img src='ebook_line_orange.png'></td></tr>\n")
        _T("</table>\n")
        _T("\n")
        _T("<br />\n")
        _T("<table width='100%' cellpadding='0' cellspacing='0'>\n")
        _T("<tr><td bgcolor='#ffffff' width='30'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>\n")
        _T("<td>\n") );

}

void lmEbookProcessor::CreateLeafletHeaders(wxString sBookTitle, wxString sHeaderTitle,
                                         wxString sTitleNum)
{
    if (!((m_fGenerateLmb && m_pLmbFile) || m_pHtmlFile)) return;

    //create page headers
    WriteToHtml(
        _T("<html>\n<head>\n")
        _T("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=") );
    WriteToHtml(m_sCharCode + _T("\">\n<title>") );
    WriteToHtml( ::wxGetTranslation(sBookTitle) );
    WriteToHtml(_T(": "));
    WriteToHtml( ::wxGetTranslation(m_sCoverPage) );
    WriteToHtml(_T("</title>\n</head>\n\n"));

    m_nHtmlIndentLevel = 1;     //indent

    WriteToHtml(
        _T("<body bgcolor='#808080'>\n")
        _T("\n")
        _T("<center>\n")
        _T("<table width='720' bgcolor='#ffffff' cellpadding='0' cellspacing='0'>\n")
        _T("<tr><td bgcolor='#ffffff'>\n")
        _T("\n")
        _T("<!-- banner -->\n")
        _T("<table width='100%' cellpadding='0' cellspacing='0'>\n")
        _T("<tr><td width='42%' bgcolor='#7f8adc'><img src='ebook_banner_left1.png'></td>\n")
        _T("<td bgcolor='#7f8adc'>&nbsp;</td>\n")
        _T("<td width='28%' bgcolor='#7f8adc' align='right'><img src='leaflet_banner_right_") );
        WriteToHtml(m_sLangCode);
        WriteToHtml(
        _T(".png'></td>\n")
        _T("<td width='20' bgcolor='#7f8adc' rowspan='2'>&nbsp;</td>\n")
        _T("</tr>\n")
    //    _T("<tr height='30'><td bgcolor='#7f8adc' align='right' valign='top'>\n")
    //    _T("	<font color='#ffffff' size=5 face='Monotype Corsiva'>") );
    //WriteToHtml( ::wxGetTranslation(m_sPhonascus) );
    //WriteToHtml(
    //    _T("</font></td></tr>\n")
        _T("<tr><td bgcolor='#ff8800' colspan='4'>.</td></tr>\n")
        _T("</table>\n\n")
        _T("\n")
        _T("<br />\n")
        _T("<table width='720' cellpadding='0' cellspacing='0'>\n")
        _T("<tr><td width='14' bgcolor='#ffffff'></td>\n")
        _T("<td>\n") );

}

//------------------------------------------------------------------------------------
// Lmb file management
//------------------------------------------------------------------------------------


bool lmEbookProcessor::StartLmbFile(wxString sFilename, wxString sLangCode,
                                    wxString sCharCode)
{
    // returns true if success

    wxFileName oFNP( sFilename );
    wxFileName oFDest( g_pPaths->GetBooksRootPath() );
    oFDest.AppendDir(sLangCode);
    oFDest.AppendDir(_T("books"));
    oFDest.SetName( oFNP.GetName() );
    oFDest.SetExt(_T("lmb"));
    m_pZipOutFile = new wxFFileOutputStream( oFDest.GetFullPath() );
    m_pZipFile = new wxZipOutputStream(*m_pZipOutFile);
    m_pLmbFile = new wxTextOutputStream(*m_pZipFile, wxEOL_NATIVE, wxCSConv(sCharCode));


    return true;

}

void lmEbookProcessor::TerminateLmbFile()
{
    if (!(m_fGenerateLmb && m_pLmbFile)) return;

    // copy toc file
    CopyToLmb( m_sTocFilename );
    if (!::wxRemoveFile(m_sTocFilename)) {
        wxLogMessage(_T("Error: File %s could not be deleted"), m_sTocFilename);
    }

    //copy other files (i.e.: images)
    wxFileName oRoot(m_sFilename);      //to get the root path
    for (int i=0; i < (int)m_aFilesToPack.GetCount(); i++)
    {
        wxFileName oFN( m_aFilesToPack.Item(i) );
        if (!oFN.IsAbsolute()) {
            oFN.SetPath(oRoot.GetPath());
        }
        CopyToLmb( oFN.GetFullPath() );
    }

    //terminate
    delete m_pZipFile;
    m_pLmbFile = (wxTextOutputStream*)NULL;
    m_pZipFile = (wxZipOutputStream*)NULL;

}

void lmEbookProcessor::CopyToLmb(wxString sFilename)
{
    wxFFileInputStream inFile( sFilename, _T("rb") );
    if (!inFile.IsOk()) {
        wxLogMessage(_T("Error: File %s can not be merged into LMB"), sFilename);
        return;
    }
    wxFileName oFN(sFilename);
    m_pZipFile->PutNextEntry( oFN.GetFullName() );
    m_pZipFile->Write( inFile );
    m_pZipFile->CloseEntry(); 

}



//------------------------------------------------------------------------------------
// Lang (.cpp) file management
//------------------------------------------------------------------------------------


bool lmEbookProcessor::StartLangFile(wxString sFilename)
{
    // returns true if success

    wxFileName oFNP( sFilename );
    wxFileName oFDest( g_pPaths->GetLocalePath() );
    oFDest.AppendDir(_T("src"));
    ::wxSetWorkingDirectory(  oFDest.GetPath() );
    oFDest.AppendDir( oFNP.GetName() );
    //if dir does not exist, create it
    ::wxMkDir( oFNP.GetName() );

    oFDest.SetName( oFNP.GetName() );
    oFDest.SetExt(_T("cpp"));
    LogMessage(_T("Creating file '%s'"), oFDest.GetFullPath());
    m_pLangFile = new wxFile(oFDest.GetFullPath(), wxFile::write);
    if (!m_pLangFile->IsOpened()) {
        LogMessage(_T("Error: File %s can not be created"), oFDest.GetFullPath());
        m_pLangFile = (wxFile*)NULL;
        return false;        //error
    }

    m_pLangFile->Write(_T("wxString sTxt;\n"));

    // add texts included by Langtool (footers, headers, etc.)
    WriteToLang( m_sFooter1 );
    WriteToLang( m_sFooter2 );
    WriteToLang( m_sPhonascus );
    WriteToLang( m_sCoverPage );

    return true;

}

void lmEbookProcessor::WriteToLang(wxString sText)
{
    //add text to Lang file

    if (!m_pLangFile || sText == _T("")) return;

    //change /n by //n
    wxString sContent = sText;
    sContent.Replace(_T("\n"), _T("\\n"));
    m_pLangFile->Write(_T("sTxt = _(\""));
    m_pLangFile->Write(sContent + _T("\");\n"));

}

bool lmEbookProcessor::CreatePoFile(wxString sFilename, wxString& sCharSet,
                                    wxString& sLangName, wxString& sLangCode,
                                    wxString& sFolder)
{
    // returns true if success

    //wxFileName oFNP( sFilename );
    //wxFileName oFDest( g_pPaths->GetLocalePath() );
    //oFDest.AppendDir(sLangCode);
    //oFDest.SetName( oFNP.GetName() );
    //oFDest.SetExt(_T("po"));
    //wxFile oFile(oFDest.GetFullPath(), wxFile::write);
    wxFile oFile(sFilename, wxFile::write);
    if (!m_pLangFile->IsOpened())
    {
        wxLogMessage(_T("Error: File %s can not be created"), sFilename);
        m_pLangFile = (wxFile*)NULL;
        return false;        //error
    }

    //Generate Po header
    wxString sNil = _T("");
    wxString sHeader = sNil +
        _T("msgid \"\"\n") 
        _T("msgstr \"\"\n")
        _T("\"Project-Id-Version: LenMus 3.4\\n\"\n")
        _T("\"POT-Creation-Date: \\n\"\n")
        _T("\"PO-Revision-Date: 2006-08-25 12:19+0100\\n\"\n")
        _T("\"Last-Translator: \\n\"\n")
        _T("\"Language-Team:  <cecilios@gmail.com>\\n\"\n")
        _T("\"MIME-Version: 1.0\\n\"\n")
        _T("\"Content-Type: text/plain; charset=utf-8\\n\"\n")
        _T("\"Content-Transfer-Encoding: 8bit\\n\"\n")
        _T("\"X-Poedit-Language: ") + sLangName + _T("\\n\"\n")
        _T("\"X-Poedit-SourceCharset: utf-8\\n\"\n")
        _T("\"X-Poedit-Basepath: c:\\usr\\desarrollo_wx\\lenmus\\langtool\\locale\\src\\n\"\n")
        _T("\"X-Poedit-SearchPath-0: ") + sFolder + _T("\\n\"\n\n");


    oFile.Write(sHeader);
    oFile.Close();
    return true;

}


//------------------------------------------------------------------------------------
// Debug methods
//------------------------------------------------------------------------------------


void lmEbookProcessor::DumpXMLTree(const wxXml2Node& oNode)
{
    wxString sTree;
    sTree.Alloc(1024);
    int nIndent = 3;

    // get a string with the tree structure...
    DumpNodeAndSiblings(oNode, sTree, nIndent);
    wxLogMessage(sTree);

}

void lmEbookProcessor::DumpNodeAndSiblings(const wxXml2Node& oNode, wxString& sTree, int n)
{
    wxXml2Node oCurr(oNode);

    do {
        DumpNode(oCurr, sTree, n);
        oCurr = oCurr.GetNext();
    } while (oCurr != wxXml2EmptyNode);
}

void lmEbookProcessor::DumpNode(const wxXml2Node& oNode, wxString& sTree, int n)
{
#define STEP            4

    if (oNode == wxXml2EmptyNode) return;
    wxString toadd, spaces(wxT(' '), n);

    // concatenate the name of this node
    toadd = oNode.GetName();
        
    // if this is a text node, then add also the contents...
    if (oNode.GetType() == wxXML_TEXT_NODE ||
        oNode.GetType() == wxXML_COMMENT_NODE || 
        oNode.GetType() == wxXML_CDATA_SECTION_NODE) {

        wxString content = oNode.GetContent();
        if (content != wxEmptyString && content.Last() == wxT('\n')) content.RemoveLast();
        if (content != wxEmptyString && content.GetChar(0) == wxT('\n')) content.Remove(0, 1);

        // a little exception: libxml2 when loading a document creates a
        // lot of text nodes containing just a simple \n;
        // in this cases, just show "[null]"
        wxString tmp = content.Trim();
        if (tmp.IsEmpty())
            toadd += wxT(" node: [null]");
        else 
            toadd += wxT(" node: ") + content;


    } else {        // if it's not a text node, then add the properties...

        wxXml2Property prop(oNode.GetProperties());
        while (prop != wxXml2EmptyProperty) {
            toadd += wxT(" ") + prop.GetName() + wxT("=");
            toadd += prop.GetValue();
            prop = prop.GetNext();
        }
    }
        
    sTree += spaces;

#define SHOW_ANNOYING_NEWLINES
#ifdef SHOW_ANNOYING_NEWLINES   

    // text nodes with newlines and/or spaces will be shown as [null]
    sTree += toadd;
#else

    // text nodes with newlines won't be shown at all
    if (toadd != wxT("textnode: [null]")) sTree += toadd;
#endif

    // go one line down
    sTree += wxT("\n");

    // do we must add the close tag ?
    bool bClose = FALSE;

    // and then, a step more indented, its children
    wxXml2Node child(oNode.GetFirstChild());
    while (child != wxXml2EmptyNode) {

        DumpNode(child, sTree, n+STEP);
        child = child.GetNext();

        // add a close tag because at least one child is present...
        bClose = TRUE;
    }

    if (bClose) sTree += wxString(wxT(' '), n) + wxT("/") + oNode.GetName() + wxT("\n");
}


void lmEbookProcessor::LogMessage(const wxChar* szFormat, ...)
{
    if (!m_pLog) return;

    va_list argptr;
    va_start(argptr, szFormat);
    wxString sMsg = wxString::FormatV(szFormat, argptr) + _T("\n");
    m_pLog->AppendText(sMsg);
    va_end(argptr);

}

void lmEbookProcessor::LogError(const wxChar* szFormat, ...)
{
    if (!m_pLog) return;

    va_list argptr;
    va_start(argptr, szFormat);
    wxString sMsg = _T("*** Error ***: ");
    sMsg += wxString::FormatV(szFormat, argptr) + _T("\n");
    m_pLog->AppendText(sMsg);
    va_end(argptr);

}



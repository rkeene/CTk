/* 
 * tkTextTag.c (CTk) --
 *
 *	This module implements the "tag" subcommand of the widget command
 *	for text widgets, plus most of the other high-level functions
 *	related to tags.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 * Copyright (c) 1995 Cleveland Clinic Foundation
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Id: ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
 */

#include "default.h"
#include "tkPort.h"
#include "tk.h"
#include "tkText.h"

/*
 * Information used for parsing tag configuration information:
 */

static Tk_ConfigSpec tagConfigSpecs[] = {
    {TK_CONFIG_STRING, "-justify", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextTag, justifyString), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-lmargin1", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextTag, lMargin1String), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-lmargin2", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextTag, lMargin2String), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-offset", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextTag, offsetString), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-rmargin", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextTag, rMarginString), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-spacing1", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextTag, spacing1String), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-spacing2", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextTag, spacing2String), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-spacing3", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextTag, spacing3String), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-tabs", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextTag, tabString), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-underline", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextTag, underlineString),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_UID, "-wrap", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(TkTextTag, wrapMode),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		ChangeTagPriority _ANSI_ARGS_((TkText *textPtr,
			    TkTextTag *tagPtr, int prio));
static TkTextTag *	FindTag _ANSI_ARGS_((Tcl_Interp *interp,
			    TkText *textPtr, char *tagName));
static void		SortTags _ANSI_ARGS_((int numTags,
			    TkTextTag **tagArrayPtr));
static int		TagSortProc _ANSI_ARGS_((CONST VOID *first,
			    CONST VOID *second));

/*
 *--------------------------------------------------------------
 *
 * TkTextTagCmd --
 *
 *	This procedure is invoked to process the "tag" options of
 *	the widget command for text widgets. See the user documentation
 *	for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
TkTextTagCmd(textPtr, interp, argc, argv)
    register TkText *textPtr;	/* Information about text widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings.  Someone else has already
				 * parsed this command enough to know that
				 * argv[1] is "tag". */
{
    int c, i, addTag;
    size_t length;
    char *fullOption;
    register TkTextTag *tagPtr;
    TkTextIndex first, last, index1, index2;

    if (argc < 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " tag option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    c = argv[2][0];
    length = strlen(argv[2]);
    if ((c == 'a') && (strncmp(argv[2], "add", length) == 0)) {
	fullOption = "add";
	addTag = 1;

	addAndRemove:
	if (argc < 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " tag ", fullOption,
		    " tagName index1 ?index2 index1 index2 ...?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	tagPtr = TkTextCreateTag(textPtr, argv[3]);
	for (i = 4; i < argc; i += 2) {
	    if (TkTextGetIndex(interp, textPtr, argv[i], &index1) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (argc > (i+1)) {
		if (TkTextGetIndex(interp, textPtr, argv[i+1], &index2)
			!= TCL_OK) {
		    return TCL_ERROR;
		}
		if (TkTextIndexCmp(&index1, &index2) >= 0) {
		    return TCL_OK;
		}
	    } else {
		index2 = index1;
		TkTextIndexForwChars(&index2, 1, &index2);
	    }
    
	    if (tagPtr->affectsDisplay) {
		TkTextRedrawTag(textPtr, &index1, &index2, tagPtr, !addTag);
	    }
	    TkBTreeTag(&index1, &index2, tagPtr, addTag);
	}
    } else if ((c == 'b') && (strncmp(argv[2], "bind", length) == 0)) {
        return Ctk_Unsupported(interp, "textWidget bind");
    } else if ((c == 'c') && (strncmp(argv[2], "cget", length) == 0)
	    && (length >= 2)) {
	if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " tag cget tagName option\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	tagPtr = FindTag(interp, textPtr, argv[3]);
	if (tagPtr == NULL) {
	    return TCL_ERROR;
	}
	return Tk_ConfigureValue(interp, textPtr->tkwin, tagConfigSpecs,
		(char *) tagPtr, argv[4], 0);
    } else if ((c == 'c') && (strncmp(argv[2], "configure", length) == 0)
	    && (length >= 2)) {
	if (argc < 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " tag configure tagName ?option? ?value? ",
		    "?option value ...?\"", (char *) NULL);
	    return TCL_ERROR;
	}
	tagPtr = TkTextCreateTag(textPtr, argv[3]);
	if (argc == 4) {
	    return Tk_ConfigureInfo(interp, textPtr->tkwin, tagConfigSpecs,
		    (char *) tagPtr, (char *) NULL, 0);
	} else if (argc == 5) {
	    return Tk_ConfigureInfo(interp, textPtr->tkwin, tagConfigSpecs,
		    (char *) tagPtr, argv[4], 0);
	} else {
	    int result;

	    result = Tk_ConfigureWidget(interp, textPtr->tkwin, tagConfigSpecs,
		    argc-4, argv+4, (char *) tagPtr, 0);
	    /*
	     * Some of the configuration options, like -underline
	     * and -justify, require additional translation (this is
	     * needed because we need to distinguish a particular value
	     * of an option from "unspecified").
	     */

	    if (tagPtr->justifyString != NULL) {
		if (Tk_GetJustify(interp, tagPtr->justifyString,
			&tagPtr->justify) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->lMargin1String != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin,
			tagPtr->lMargin1String, &tagPtr->lMargin1) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->lMargin2String != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin,
			tagPtr->lMargin2String, &tagPtr->lMargin2) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->offsetString != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin, tagPtr->offsetString,
			&tagPtr->offset) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->rMarginString != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin,
			tagPtr->rMarginString, &tagPtr->rMargin) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->spacing1String != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin,
			tagPtr->spacing1String, &tagPtr->spacing1) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (tagPtr->spacing1 < 0) {
		    tagPtr->spacing1 = 0;
		}
	    }
	    if (tagPtr->spacing2String != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin,
			tagPtr->spacing2String, &tagPtr->spacing2) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (tagPtr->spacing2 < 0) {
		    tagPtr->spacing2 = 0;
		}
	    }
	    if (tagPtr->spacing3String != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin,
			tagPtr->spacing3String, &tagPtr->spacing3) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (tagPtr->spacing3 < 0) {
		    tagPtr->spacing3 = 0;
		}
	    }
	    if (tagPtr->tabArrayPtr != NULL) {
		ckfree((char *) tagPtr->tabArrayPtr);
		tagPtr->tabArrayPtr = NULL;
	    }
	    if (tagPtr->tabString != NULL) {
	    	tagPtr->tabArrayPtr = TkTextGetTabs(interp, textPtr->tkwin,
	    		tagPtr->tabString);
		if (tagPtr->tabArrayPtr == NULL) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->underlineString != NULL) {
		if (Tcl_GetBoolean(interp, tagPtr->underlineString,
			&tagPtr->underline) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if ((tagPtr->wrapMode != NULL)
		    && (tagPtr->wrapMode != tkTextCharUid)
		    && (tagPtr->wrapMode != tkTextNoneUid)
		    && (tagPtr->wrapMode != tkTextWordUid)) {
		Tcl_AppendResult(interp, "bad wrap mode \"", tagPtr->wrapMode,
			"\":  must be char, none, or word", (char *) NULL);
		tagPtr->wrapMode = NULL;
		return TCL_ERROR;
	    }

	    tagPtr->affectsDisplay = 0;
	    if ((tagPtr->justifyString != NULL)
		    || (tagPtr->lMargin1String != NULL)
		    || (tagPtr->lMargin2String != NULL)
		    || (tagPtr->offsetString != NULL)
		    || (tagPtr->rMarginString != NULL)
		    || (tagPtr->spacing1String != NULL)
		    || (tagPtr->spacing2String != NULL)
		    || (tagPtr->spacing3String != NULL)
		    || (tagPtr->tabString != NULL)
		    || (tagPtr->underlineString != NULL)
		    || (tagPtr->wrapMode != NULL)) {
		tagPtr->affectsDisplay = 1;
	    }
	    TkTextRedrawTag(textPtr, (TkTextIndex *) NULL,
		    (TkTextIndex *) NULL, tagPtr, 1);
	    return result;
	}
    } else if ((c == 'd') && (strncmp(argv[2], "delete", length) == 0)) {
	Tcl_HashEntry *hPtr;

	if (argc < 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " tag delete tagName tagName ...\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	for (i = 3; i < argc; i++) {
	    hPtr = Tcl_FindHashEntry(&textPtr->tagTable, argv[i]);
	    if (hPtr == NULL) {
		continue;
	    }
	    tagPtr = (TkTextTag *) Tcl_GetHashValue(hPtr);
	    if (tagPtr == textPtr->selTagPtr) {
		continue;
	    }
	    if (tagPtr->affectsDisplay) {
		TkTextRedrawTag(textPtr, (TkTextIndex *) NULL,
			(TkTextIndex *) NULL, tagPtr, 1);
	    }
	    TkBTreeTag(TkTextMakeIndex(textPtr->tree, 0, 0, &first),
		    TkTextMakeIndex(textPtr->tree,
			    TkBTreeNumLines(textPtr->tree), 0, &last),
		    tagPtr, 0);
	    Tcl_DeleteHashEntry(hPtr);
	
	    /*
	     * Update the tag priorities to reflect the deletion of this tag.
	     */

	    ChangeTagPriority(textPtr, tagPtr, textPtr->numTags-1);
	    textPtr->numTags -= 1;
	    TkTextFreeTag(textPtr, tagPtr);
	}
    } else if ((c == 'l') && (strncmp(argv[2], "lower", length) == 0)) {
	TkTextTag *tagPtr2;
	int prio;

	if ((argc != 4) && (argc != 5)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " tag lower tagName ?belowThis?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	tagPtr = FindTag(interp, textPtr, argv[3]);
	if (tagPtr == NULL) {
	    return TCL_ERROR;
	}
	if (argc == 5) {
	    tagPtr2 = FindTag(interp, textPtr, argv[4]);
	    if (tagPtr2 == NULL) {
		return TCL_ERROR;
	    }
	    if (tagPtr->priority < tagPtr2->priority) {
		prio = tagPtr2->priority - 1;
	    } else {
		prio = tagPtr2->priority;
	    }
	} else {
	    prio = 0;
	}
	ChangeTagPriority(textPtr, tagPtr, prio);
	TkTextRedrawTag(textPtr, (TkTextIndex *) NULL, (TkTextIndex *) NULL,
		tagPtr, 1);
    } else if ((c == 'n') && (strncmp(argv[2], "names", length) == 0)
	    && (length >= 2)) {
	TkTextTag **arrayPtr;
	int arraySize;

	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " tag names ?index?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    Tcl_HashSearch search;
	    Tcl_HashEntry *hPtr;

	    arrayPtr = (TkTextTag **) ckalloc((unsigned)
		    (textPtr->numTags * sizeof(TkTextTag *)));
	    for (i = 0, hPtr = Tcl_FirstHashEntry(&textPtr->tagTable, &search);
		    hPtr != NULL; i++, hPtr = Tcl_NextHashEntry(&search)) {
		arrayPtr[i] = (TkTextTag *) Tcl_GetHashValue(hPtr);
	    }
	    arraySize = textPtr->numTags;
	} else {
	    if (TkTextGetIndex(interp, textPtr, argv[3], &index1)
		    != TCL_OK) {
		return TCL_ERROR;
	    }
	    arrayPtr = TkBTreeGetTags(&index1, &arraySize);
	    if (arrayPtr == NULL) {
		return TCL_OK;
	    }
	}
	SortTags(arraySize, arrayPtr);
	for (i = 0; i < arraySize; i++) {
	    tagPtr = arrayPtr[i];
	    Tcl_AppendElement(interp, tagPtr->name);
	}
	ckfree((char *) arrayPtr);
    } else if ((c == 'n') && (strncmp(argv[2], "nextrange", length) == 0)
	    && (length >= 2)) {
	TkTextSearch tSearch;
	char position[TK_POS_CHARS];

	if ((argc != 5) && (argc != 6)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " tag nextrange tagName index1 ?index2?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	tagPtr = FindTag((Tcl_Interp *) NULL, textPtr, argv[3]);
	if (tagPtr == NULL) {
	    return TCL_OK;
	}
	if (TkTextGetIndex(interp, textPtr, argv[4], &index1) != TCL_OK) {
	    return TCL_ERROR;
	}
	TkTextMakeIndex(textPtr->tree, TkBTreeNumLines(textPtr->tree),
		0, &last);
	if (argc == 5) {
	    index2 = last;
	} else if (TkTextGetIndex(interp, textPtr, argv[5], &index2)
		!= TCL_OK) {
	    return TCL_ERROR;
	}

	/*
	 * The search below is a bit tricky.  Rather than use the B-tree
	 * facilities to stop the search at index2, let it search up
	 * until the end of the file but check for a position past index2
	 * ourselves.  The reason for doing it this way is that we only
	 * care whether the *start* of the range is before index2;  once
	 * we find the start, we don't want TkBTreeNextTag to abort the
	 * search because the end of the range is after index2.
	 */

	TkBTreeStartSearch(&index1, &last, tagPtr, &tSearch);
	if (TkBTreeCharTagged(&index1, tagPtr)) {
	    TkTextSegment *segPtr;
	    int offset;

	    /*
	     * The first character is tagged.  See if there is an
	     * on-toggle just before the character.  If not, then
	     * skip to the end of this tagged range.
	     */

	    for (segPtr = index1.linePtr->segPtr, offset = index1.charIndex; 
		    offset >= 0;
		    offset -= segPtr->size, segPtr = segPtr->nextPtr) {
		if ((offset == 0) && (segPtr->typePtr == &tkTextToggleOnType)
			&& (segPtr->body.toggle.tagPtr == tagPtr)) {
		    goto gotStart;
		}
	    }
	    if (!TkBTreeNextTag(&tSearch)) {
		 return TCL_OK;
	    }
	}

	/*
	 * Find the start of the tagged range.
	 */

	if (!TkBTreeNextTag(&tSearch)) {
	    return TCL_OK;
	}
	gotStart:
	if (TkTextIndexCmp(&tSearch.curIndex, &index2) >= 0) {
	    return TCL_OK;
	}
	TkTextPrintIndex(&tSearch.curIndex, position);
	Tcl_AppendElement(interp, position);
	TkBTreeNextTag(&tSearch);
	TkTextPrintIndex(&tSearch.curIndex, position);
	Tcl_AppendElement(interp, position);
    } else if ((c == 'r') && (strncmp(argv[2], "raise", length) == 0)
	    && (length >= 3)) {
	TkTextTag *tagPtr2;
	int prio;

	if ((argc != 4) && (argc != 5)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " tag raise tagName ?aboveThis?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	tagPtr = FindTag(interp, textPtr, argv[3]);
	if (tagPtr == NULL) {
	    return TCL_ERROR;
	}
	if (argc == 5) {
	    tagPtr2 = FindTag(interp, textPtr, argv[4]);
	    if (tagPtr2 == NULL) {
		return TCL_ERROR;
	    }
	    if (tagPtr->priority <= tagPtr2->priority) {
		prio = tagPtr2->priority;
	    } else {
		prio = tagPtr2->priority + 1;
	    }
	} else {
	    prio = textPtr->numTags-1;
	}
	ChangeTagPriority(textPtr, tagPtr, prio);
	TkTextRedrawTag(textPtr, (TkTextIndex *) NULL, (TkTextIndex *) NULL,
		tagPtr, 1);
    } else if ((c == 'r') && (strncmp(argv[2], "ranges", length) == 0)
	    && (length >= 3)) {
	TkTextSearch tSearch;
	char position[TK_POS_CHARS];

	if (argc != 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " tag ranges tagName\"", (char *) NULL);
	    return TCL_ERROR;
	}
	tagPtr = FindTag((Tcl_Interp *) NULL, textPtr, argv[3]);
	if (tagPtr == NULL) {
	    return TCL_OK;
	}
	TkTextMakeIndex(textPtr->tree, 0, 0, &first);
	TkTextMakeIndex(textPtr->tree, TkBTreeNumLines(textPtr->tree),
		0, &last);
	TkBTreeStartSearch(&first, &last, tagPtr, &tSearch);
	if (TkBTreeCharTagged(&first, tagPtr)) {
	    TkTextPrintIndex(&first, position);
	    Tcl_AppendElement(interp, position);
	}
	while (TkBTreeNextTag(&tSearch)) {
	    TkTextPrintIndex(&tSearch.curIndex, position);
	    Tcl_AppendElement(interp, position);
	}
    } else if ((c == 'r') && (strncmp(argv[2], "remove", length) == 0)
	    && (length >= 2)) {
	fullOption = "remove";
	addTag = 0;
	goto addAndRemove;
    } else {
	Tcl_AppendResult(interp, "bad tag option \"", argv[2],
		"\":  must be add, bind, cget, configure, delete, lower, ",
		"names, nextrange, raise, ranges, or remove",
		(char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextCreateTag --
 *
 *	Find the record describing a tag within a given text widget,
 *	creating a new record if one doesn't already exist.
 *
 * Results:
 *	The return value is a pointer to the TkTextTag record for tagName.
 *
 * Side effects:
 *	A new tag record is created if there isn't one already defined
 *	for tagName.
 *
 *----------------------------------------------------------------------
 */

TkTextTag *
TkTextCreateTag(textPtr, tagName)
    TkText *textPtr;		/* Widget in which tag is being used. */
    char *tagName;		/* Name of desired tag. */
{
    register TkTextTag *tagPtr;
    Tcl_HashEntry *hPtr;
    int new;

    hPtr = Tcl_CreateHashEntry(&textPtr->tagTable, tagName, &new);
    if (!new) {
	return (TkTextTag *) Tcl_GetHashValue(hPtr);
    }

    /*
     * No existing entry.  Create a new one, initialize it, and add a
     * pointer to it to the hash table entry.
     */

    tagPtr = (TkTextTag *) ckalloc(sizeof(TkTextTag));
    tagPtr->name = Tcl_GetHashKey(&textPtr->tagTable, hPtr);
    tagPtr->priority = textPtr->numTags;
    tagPtr->justifyString = NULL;
    tagPtr->justify = TK_JUSTIFY_LEFT;
    tagPtr->lMargin1String = NULL;
    tagPtr->lMargin1 = 0;
    tagPtr->lMargin2String = NULL;
    tagPtr->lMargin2 = 0;
    tagPtr->offsetString = NULL;
    tagPtr->offset = 0;
    tagPtr->rMarginString = NULL;
    tagPtr->rMargin = 0;
    tagPtr->spacing1String = NULL;
    tagPtr->spacing1 = 0;
    tagPtr->spacing2String = NULL;
    tagPtr->spacing2 = 0;
    tagPtr->spacing3String = NULL;
    tagPtr->spacing3 = 0;
    tagPtr->tabString = NULL;
    tagPtr->tabArrayPtr = NULL;
    tagPtr->underlineString = NULL;
    tagPtr->underline = 0;
    tagPtr->wrapMode = NULL;
    tagPtr->affectsDisplay = 0;
    textPtr->numTags++;
    Tcl_SetHashValue(hPtr, tagPtr);
    return tagPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FindTag --
 *
 *	See if tag is defined for a given widget.
 *
 * Results:
 *	If tagName is defined in textPtr, a pointer to its TkTextTag
 *	structure is returned.  Otherwise NULL is returned and an
 *	error message is recorded in interp->result unless interp
 *	is NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static TkTextTag *
FindTag(interp, textPtr, tagName)
    Tcl_Interp *interp;		/* Interpreter to use for error message;
				 * if NULL, then don't record an error
				 * message. */
    TkText *textPtr;		/* Widget in which tag is being used. */
    char *tagName;		/* Name of desired tag. */
{
    Tcl_HashEntry *hPtr;

    hPtr = Tcl_FindHashEntry(&textPtr->tagTable, tagName);
    if (hPtr != NULL) {
	return (TkTextTag *) Tcl_GetHashValue(hPtr);
    }
    if (interp != NULL) {
	Tcl_AppendResult(interp, "tag \"", tagName,
		"\" isn't defined in text widget", (char *) NULL);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextFreeTag --
 *
 *	This procedure is called when a tag is deleted to free up the
 *	memory and other resources associated with the tag.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory and other resources are freed.
 *
 *----------------------------------------------------------------------
 */

void
TkTextFreeTag(textPtr, tagPtr)
    TkText *textPtr;			/* Info about overall widget. */
    register TkTextTag *tagPtr;		/* Tag being deleted. */
{
    if (tagPtr->justifyString != NULL) {
	ckfree(tagPtr->justifyString);
    }
    if (tagPtr->lMargin1String != NULL) {
	ckfree(tagPtr->lMargin1String);
    }
    if (tagPtr->lMargin2String != NULL) {
	ckfree(tagPtr->lMargin2String);
    }
    if (tagPtr->offsetString != NULL) {
	ckfree(tagPtr->offsetString);
    }
    if (tagPtr->rMarginString != NULL) {
	ckfree(tagPtr->rMarginString);
    }
    if (tagPtr->spacing1String != NULL) {
	ckfree(tagPtr->spacing1String);
    }
    if (tagPtr->spacing2String != NULL) {
	ckfree(tagPtr->spacing2String);
    }
    if (tagPtr->spacing3String != NULL) {
	ckfree(tagPtr->spacing3String);
    }
    if (tagPtr->tabString != NULL) {
	ckfree(tagPtr->tabString);
    }
    if (tagPtr->tabArrayPtr != NULL) {
	ckfree((char *) tagPtr->tabArrayPtr);
    }
    if (tagPtr->underlineString != NULL) {
	ckfree(tagPtr->underlineString);
    }
    ckfree((char *) tagPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * SortTags --
 *
 *	This procedure sorts an array of tag pointers in increasing
 *	order of priority, optimizing for the common case where the
 *	array is small.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
SortTags(numTags, tagArrayPtr)
    int numTags;		/* Number of tag pointers at *tagArrayPtr. */
    TkTextTag **tagArrayPtr;	/* Pointer to array of pointers. */
{
    int i, j, prio;
    register TkTextTag **tagPtrPtr;
    TkTextTag **maxPtrPtr, *tmp;

    if (numTags < 2) {
	return;
    }
    if (numTags < 20) {
	for (i = numTags-1; i > 0; i--, tagArrayPtr++) {
	    maxPtrPtr = tagPtrPtr = tagArrayPtr;
	    prio = tagPtrPtr[0]->priority;
	    for (j = i, tagPtrPtr++; j > 0; j--, tagPtrPtr++) {
		if (tagPtrPtr[0]->priority < prio) {
		    prio = tagPtrPtr[0]->priority;
		    maxPtrPtr = tagPtrPtr;
		}
	    }
	    tmp = *maxPtrPtr;
	    *maxPtrPtr = *tagArrayPtr;
	    *tagArrayPtr = tmp;
	}
    } else {
	qsort((VOID *) tagArrayPtr, (unsigned) numTags, sizeof (TkTextTag *),
		    TagSortProc);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TagSortProc --
 *
 *	This procedure is called by qsort when sorting an array of
 *	tags in priority order.
 *
 * Results:
 *	The return value is -1 if the first argument should be before
 *	the second element (i.e. it has lower priority), 0 if it's
 *	equivalent (this should never happen!), and 1 if it should be
 *	after the second element.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TagSortProc(first, second)
    CONST VOID *first, *second;		/* Elements to be compared. */
{
    TkTextTag *tagPtr1, *tagPtr2;

    tagPtr1 = * (TkTextTag **) first;
    tagPtr2 = * (TkTextTag **) second;
    return tagPtr1->priority - tagPtr2->priority;
}

/*
 *----------------------------------------------------------------------
 *
 * ChangeTagPriority --
 *
 *	This procedure changes the priority of a tag by modifying
 *	its priority and the priorities of other tags that are affected
 *	by the change.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Priorities may be changed for some or all of the tags in
 *	textPtr.  The tags will be arranged so that there is exactly
 *	one tag at each priority level between 0 and textPtr->numTags-1,
 *	with tagPtr at priority "prio".
 *
 *----------------------------------------------------------------------
 */

static void
ChangeTagPriority(textPtr, tagPtr, prio)
    TkText *textPtr;			/* Information about text widget. */
    TkTextTag *tagPtr;			/* Tag whose priority is to be
					 * changed. */
    int prio;				/* New priority for tag. */
{
    int low, high, delta;
    register TkTextTag *tagPtr2;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;

    if (prio < 0) {
	prio = 0;
    }
    if (prio >= textPtr->numTags) {
	prio = textPtr->numTags-1;
    }
    if (prio == tagPtr->priority) {
	return;
    } else if (prio < tagPtr->priority) {
	low = prio;
	high = tagPtr->priority-1;
	delta = 1;
    } else {
	low = tagPtr->priority+1;
	high = prio;
	delta = -1;
    }
    for (hPtr = Tcl_FirstHashEntry(&textPtr->tagTable, &search);
	    hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	tagPtr2 = (TkTextTag *) Tcl_GetHashValue(hPtr);
	if ((tagPtr2->priority >= low) && (tagPtr2->priority <= high)) {
	    tagPtr2->priority += delta;
	}
    }
    tagPtr->priority = prio;
}

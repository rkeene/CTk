/* 
 * ctkRegion.c (CTk) --
 *
 *	Geometry manipulation routines - regions (free form 2-D shapes),
 *	rectangles, and spans (horizontal segments).
 *
 *	Beware, some of theses routines have special constraints that
 *	are not obvious from their title.  Be sure to examine headers
 *	for constraints.
 *
 * Copyright (c) 1994-1995 Cleveland Clinic Foundation
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Id: ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
 */

#include "tkPort.h"
#include "tkInt.h"

/*
 * Notes on Geometry Types.
 *
 * Spans -
 *	Horizontal line segment.  Between left point (inclusive)
 *	and right point (exclusive).  Any span with right <= left
 *	is considered empty.
 *
 * Rectangles -
 *	Analogous to spans top and left points are include,
 *	and right and bottom points are excluded from area
 *	rectangle.  Any rectangle with right <= left or
 *	bottom <= top is considered empty.
 *
 * Region -
 *	Free form area.  Contents of structure are opaque.
 *	!!! Vertical bounds of region cannot increase !!!
 *	The rectangle used with CtkCreateRegion() must have the
 *	maximum top and bottom for the region (the rectangle
 *	may still be empty if right <= left).
 */

/*
 *  Limits for x and y coordinates
 *  (choose these to work with systems where int is 16-bit).
 */
#define COORD_MAX           32767
#define COORD_MIN           -32768


/*
 * Component of a region
 */
typedef struct {
    int left;
    int right;
    int next;           /* Index of next span */
} RegionSpan;
#define NO_SPAN (-1)    /* Invalid index (indicates end of span list) */

struct CtkRegion {
    int top;
    int bottom;
    int free;
    int num_spans;
    RegionSpan *spans;
};

#define CopySpan(dst, src) (memcpy((dst), (src), sizeof(RegionSpan)))
#define FreeSpan(rgnPtr, index) \
	((rgnPtr)->spans[(index)].next \
	= (rgnPtr)->free, (rgnPtr)->free = (index))

/*
 * Private Function Declarations
 */
static void	PseudoUnionSpans _ANSI_ARGS_((int *leftPtr, int *rightPtr,
		    int left2, int right2));
static int	DeleteSpan _ANSI_ARGS_((CtkRegion * rgnPtr, int index,
		    int priorIndex));
static void	AppendSpan _ANSI_ARGS_((CtkRegion * rgnPtr, int index,
		    int left, int right));
static void	PrependSpan _ANSI_ARGS_((CtkRegion * rgnPtr, int index,
		    int left, int right));
static int	AllocSpan _ANSI_ARGS_((CtkRegion * rgnPtr));
static void	MergeSpan _ANSI_ARGS_((CtkRegion * rgnPtr, int left,
		    int right, int y));


/*
 *----------------------------------------------------------------------
 *
 * CtkIntersectSpans -- compute intersection of 2 spans
 *
 *	Compute the intersection of the span (*leftPtr,*rightPtr)
 *	and the span (left2,right2).
 *
 * Results:
 *	Stores the resulting span in `leftPtr' and `rightPtr'.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
CtkIntersectSpans(leftPtr, rightPtr, left2, right2)
    int *leftPtr;
    int *rightPtr;
    int left2;
    int right2;
{
    if (*leftPtr < left2)  *leftPtr = left2;
    if (*rightPtr > right2)  *rightPtr = right2;
}

/*
 *----------------------------------------------------------------------
 *
 * PseudoUnionSpans -- compute union of 2 spans
 *
 *	Compute the union of the span (*leftPtr,*rightPtr)
 *	and the span (left2,right2).  Assumes that the spans overlap.
 *	If they don't, the result will contain the area between the
 *	spans also.  (A real union would have to be capable of
 *	returning two disjoint spans.)
 *
 * Results:
 *	Stores the resulting span in `leftPtr' and `rightPtr'.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
PseudoUnionSpans(leftPtr, rightPtr, left2, right2)
    int *leftPtr;
    int *rightPtr;
    int left2;
    int right2;
{
    if (*leftPtr > left2)  *leftPtr = left2;
    if (*rightPtr < right2)  *rightPtr = right2;
}

/*
 *----------------------------------------------------------------------
 *
 * CtkSpanMinusSpan - compute difference of 2 spans
 *
 *	Substract span (subL, subR) from span (srcL, srcR).
 *	(Find segment(s) in first span that do not overlap with
 *	second span.)
 *
 * Results:
 *	Returns the number of resultin spans (0-2).
 *	Stores the resulting span(s) in remsL[] and remsR[].
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
CtkSpanMinusSpan(srcL, srcR, subL, subR, remsL, remsR)
    int srcL;
    int srcR;
    int subL;
    int subR;
    int *remsL;
    int *remsR;
{
    int numRems = 0;

    if (srcR <= subL || srcL >= subR)  return (numRems);
    if (srcL < subL) {
	remsL[numRems] = srcL;
	remsR[numRems] = subL;
	numRems += 1;
    }
    if (srcR > subR) {
	remsL[numRems] = subR;
	remsR[numRems] = srcR;
	numRems += 1;
    }
    return (numRems);

}

/*
 *----------------------------------------------------------------------
 *
 * CtkIntersectRects -- compute intersection of two rectangles
 *
 *	Computer overlap between rectangles pointed to by
 *	r1Ptr and r2Ptr.
 *
 * Results:
 *	Stores clipped down rectangle in `r1Ptr'.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
CtkIntersectRects(r1Ptr, r2Ptr)
    Ctk_Rect * r1Ptr;
    CONST Ctk_Rect * r2Ptr;
{

    if (r1Ptr->left < r2Ptr->left)  r1Ptr->left = r2Ptr->left;
    if (r1Ptr->top < r2Ptr->top)  r1Ptr->top = r2Ptr->top;
    if (r1Ptr->right > r2Ptr->right)  r1Ptr->right = r2Ptr->right;
    if (r1Ptr->bottom > r2Ptr->bottom)  r1Ptr->bottom = r2Ptr->bottom;

}

/*
 *----------------------------------------------------------------------
 *
 * CtkCreateRegion -- create a new region
 *
 *	Create a new region and initialize it to the area of
 *	`rect'.
 *
 * Results:
 *	Returns pointer to new region.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

CtkRegion *
CtkCreateRegion(rect)
    Ctk_Rect * rect;
{
    CtkRegion * rgnPtr;
    int i;
    rgnPtr = (CtkRegion *) ckalloc(sizeof(CtkRegion));

    rgnPtr->top = rect->top;
    rgnPtr->bottom = rect->bottom;
    rgnPtr->free = NO_SPAN;
    rgnPtr->num_spans = rgnPtr->bottom - rgnPtr->top;
    if (rgnPtr->num_spans <= 0) {
    	rgnPtr->num_spans = 0;
    	rgnPtr->spans = (RegionSpan *) NULL;
    } else {
	rgnPtr->spans = (RegionSpan *)
		ckalloc(rgnPtr->num_spans * sizeof(RegionSpan));
	for (i=0; i < rgnPtr->num_spans; i++) {
	    rgnPtr->spans[i].left = rect->left;
	    rgnPtr->spans[i].right = rect->right;
	    rgnPtr->spans[i].next = NO_SPAN;
	}
    }
    return rgnPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * CtkDestroyRegion - release resources held by a region
 *
 *	Free resources for a region - region may not be referenced
 *	again.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed.
 *
 *----------------------------------------------------------------------
 */

void
CtkDestroyRegion(rgnPtr)
    CtkRegion *rgnPtr;
{
    if (rgnPtr->spans) {
	ckfree((char *) rgnPtr->spans);
    }
    ckfree((char *) rgnPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * CtkRegionMinusRect - remove rectangular area from region
 *
 *	Substract area of rectangle `rectPtr' from region `rgnPtr'.
 *
 * Results:
 *	If `wantInter', returns the intersection of the region and
 *	the rectangle (as a new region - use CtkDestroyRegion() to
 *	get rid of it).
 *	Otherwise, returns NULL.
 *
 * Side effects:
 *	Contents of `rgnPtr' is changed.
 *
 *----------------------------------------------------------------------
 */

CtkRegion *
CtkRegionMinusRect(rgnPtr, rectPtr, wantInter)
    CtkRegion *rgnPtr;
    Ctk_Rect *rectPtr;
    int wantInter;
{
    int itop = rectPtr->top;
    int ibottom = rectPtr->bottom;
    RegionSpan *spans = rgnPtr->spans;
    int y;
    int idx;
    int lastIdx;
    int rems;
    int newLefts[2];
    int newRights[2];
    Ctk_Rect emptyRect;
    CtkRegion * intersection = NULL;

    CtkIntersectSpans(&itop, &ibottom, rgnPtr->top, rgnPtr->bottom);
    if (wantInter) {
	emptyRect.left = 0;
	emptyRect.top = itop;
	emptyRect.right = 0;
	emptyRect.bottom = ibottom;
	intersection = (CtkRegion *) CtkCreateRegion(&emptyRect);
    }

    for (y = itop; y < ibottom; y++) {
	lastIdx = NO_SPAN;
	idx = y - rgnPtr->top;
	while (idx != NO_SPAN) {
	    if (spans[idx].left >= rectPtr->right) {
		/*
		 * Remaining spans on this line are right of `rectPtr'
		 */
		break;
	    }
	    if (spans[idx].right <= rectPtr->left) {
		/*
		 * No overlap
		 */
		lastIdx = idx;
		idx = spans[idx].next;
	    } else {
		/*
		 * Rect and span overlap
		 */
		rems = CtkSpanMinusSpan(
		    spans[idx].left, spans[idx].right,
		    rectPtr->left, rectPtr->right,
		    newLefts, newRights);
		if (wantInter) {
		    CtkIntersectSpans(
			&spans[idx].left, &spans[idx].right,
			rectPtr->left, rectPtr->right );
		    MergeSpan(intersection,
			spans[idx].left, spans[idx].right, y);
		}
		switch (rems) {
		case 0:
		    idx = DeleteSpan(rgnPtr, idx, lastIdx);
		    break;
		case 1:
		    spans[idx].left = newLefts[0];
		    spans[idx].right = newRights[0];
		    lastIdx = idx;
		    idx = spans[idx].next;
		    break;
		case 2:
		    spans[idx].left = newLefts[0];
		    spans[idx].right = newRights[0];
		    AppendSpan(rgnPtr, idx, newLefts[1], newRights[1]);
		    spans = rgnPtr->spans;
		    lastIdx = spans[idx].next;
		    idx = spans[lastIdx].next;
		    break;
		}
	    }
	} /* for (idx) */
    } /* for (y) */
    return intersection;
}

/*
 *----------------------------------------------------------------------
 *
 * CtkUnionRegions - merge one region into another
 *
 *	Computes the union of the regions `rgn1Ptr' and `rgn2Ptr',
 *	and stores it in `rgn1Ptr'.
 *	!!! The union cannot increase the top and bottom of `rgn1' !!!
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	`rgn1Ptr' was is (possibly) expanded.
 *
 *----------------------------------------------------------------------
 */

void
CtkUnionRegions(rgn1Ptr, rgn2Ptr)
    CtkRegion *rgn1Ptr;
    CtkRegion *rgn2Ptr;
{
    RegionSpan *spans2 = rgn2Ptr->spans;
    int top2 = rgn2Ptr->top;
    int bottom2 = rgn2Ptr->bottom;
    int y;
    int idx;

    for (y = top2; y < bottom2; y++) {
	idx = y - top2;
	if (spans2[idx].left >= spans2[idx].right) {
	    /* Empty scan line */
	    continue;
	}
	/*
	 * Could eventually expand (repack) region 1 here,
	 * if line is not within the vertical bounds of the
	 * first region.
	 */
	while (idx != NO_SPAN) {
	    MergeSpan(rgn1Ptr, spans2[idx].left, spans2[idx].right, y);
	    idx = spans2[idx].next;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CtkForEachSpan -- perform function on every span in a region
 *
 *	Executes function `spanProcPtr' for each span in region.
 *	Passes `clientData' and the span as arguments to `spanProcPtr'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	`spanProcPtr' is executed.
 *
 *----------------------------------------------------------------------
 */

void
CtkForEachSpan(spanProcPtr, clientData, rgnPtr)
    CtkSpanProc *spanProcPtr;
    ClientData clientData;
    CtkRegion *rgnPtr;
{
    RegionSpan *spans = rgnPtr->spans;
    int top = rgnPtr->top;
    int bottom = rgnPtr->bottom;
    int y;
    int idx;

    for (y = top; y < bottom; y++) {
	idx = y - top;
	if (spans[idx].left >= spans[idx].right) {
	    /* Empty scan line */
	    continue;
	}
	while (idx != NO_SPAN) {
	    (*spanProcPtr)(spans[idx].left, spans[idx].right, y, clientData);
	    idx = spans[idx].next;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CtkForEachIntersectingSpan -- perform function on region/span intersection
 *
 *	Computes the intersection of region `rgnPtr' and the
 *	span `left',`right' at vertical position `y'.  Executes
 *	function `spanProcPtr' for each span in the intersection.
 *	Passes `clientData' and the span as arguments to `spanProcPtr'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	`spanProcPtr' is executed.
 *
 *----------------------------------------------------------------------
 */

void
CtkForEachIntersectingSpan(spanProcPtr, clientData, left, right, y, rgnPtr)
    CtkSpanProc *spanProcPtr;
    ClientData clientData;
    int left;
    int right;
    int y;
    CtkRegion *rgnPtr;
{
    RegionSpan *spans = rgnPtr->spans;
    int idx;
    int ileft;
    int iright;

    if (y < rgnPtr->top || y >= rgnPtr->bottom)  return;

    for (idx = y - rgnPtr->top; idx != NO_SPAN; idx = spans[idx].next) {
	if (spans[idx].left >= right)  break;
	if (spans[idx].right > left) {
	    /*
	     * Spans overlap.
	     */
	    ileft = spans[idx].left;
	    iright = spans[idx].right;
	    CtkIntersectSpans(&ileft, &iright, left, right);
	    (*spanProcPtr)(ileft, iright, y, clientData);
	}
    } /* for (idx) */
}

/*
 *----------------------------------------------------------------------
 *
 * CtkPointInRegion -- check if point is contained in region
 *
 *	Check if point (x,y) is in the region `rgnPtr'.
 *
 * Results:
 *	Returns 1 if point is in region, otherwise returns 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
CtkPointInRegion(x, y, rgnPtr)
    int x, y;
    CtkRegion *rgnPtr;
{
    RegionSpan *spans = rgnPtr->spans;
    int idx;

    if (y >= rgnPtr->top && y < rgnPtr->bottom) {
	for (idx = y - rgnPtr->top; idx != NO_SPAN; idx = spans[idx].next) {
	    if (spans[idx].left > x)  break;
	    if (spans[idx].right > x) {
		return 1;
	    }
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * CtkRegionGetRect -- compute enclosing rectangle of a region
 *
 *	Compute the smallest rectangle that will enclose the
 *	area of `rgnPtr'.
 *
 * Results:
 *	Stores the resulting rectangle in `rectPtr'.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
CtkRegionGetRect(rgnPtr, rectPtr)
    CtkRegion *rgnPtr;
    Ctk_Rect *rectPtr;
{
    RegionSpan *spans = rgnPtr->spans;
    int top = rgnPtr->top;
    int numLines = rgnPtr->bottom - top;
    int topLine;           /* Index of top non-empty scan line */
    int bottomLine;        /* Index of bottom non-empty scan line */
    int left = COORD_MAX;
    int right = COORD_MIN;
    int line;
    int i;

    for (topLine = 0; topLine < numLines; topLine++) {
	if (spans[topLine].left < spans[topLine].right) {
	    /* Non-empty scan line */
	    break;
	}
    }
    for (bottomLine = numLines-1 ; bottomLine >= topLine; bottomLine--) {
	if (spans[bottomLine].left < spans[bottomLine].right) {
	    /* Non-empty scan line */
	    break;
	}
    }
    bottomLine++;

    for (line = topLine; line < bottomLine; line++)
    {
	if (spans[line].left < spans[line].right) {
	    /*
	     *	Non-empty scan line, if it goes outside the current
	     *	left and right bounds, then expand the bounds.
	     */
	    if (spans[line].left < left) {
		left = spans[line].left;
	    }
	    for (i = line; spans[i].next != NO_SPAN; i++);
	    if (spans[i].right > right) {
		right = spans[i].right;
	    }
	}
    }

    if (left < right && topLine < bottomLine) {
	CtkSetRect(rectPtr, left, top+topLine, right, top+bottomLine);
    } else {
	CtkSetRect(rectPtr, 0, 0, 0, 0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteSpan -- remove a span from a region
 *
 *	Removes the span at `index' from `rgnPtr'.  `priorIndex'
 *	must point to the preceding span, or be NO_SPAN if this
 *	is the first span of a line.
 *
 * Results:
 *	Index of the next span (one after the deleted one).
 *
 * Side effects:
 *	The span at the specified index is removed, unless it is the
 *	first span of scan line in which case is is set to empty.
 *
 *----------------------------------------------------------------------
 */

static int
DeleteSpan(rgnPtr, index, priorIndex)
    CtkRegion *rgnPtr;
    int index;
    int priorIndex;
{
    int nextIndex = rgnPtr->spans[index].next;

    if (priorIndex == NO_SPAN) {
	if (nextIndex == NO_SPAN) {
	    rgnPtr->spans[index].left = rgnPtr->spans[index].right;
	} else {
	    CopySpan(&rgnPtr->spans[index], &rgnPtr->spans[nextIndex]);
	    FreeSpan(rgnPtr, nextIndex);
	    nextIndex = index;
	}
    } else {
	rgnPtr->spans[priorIndex].next = nextIndex;
	FreeSpan(rgnPtr, index);
    }
    return nextIndex;
}

/*
 *----------------------------------------------------------------------
 *
 * AppendSpan -- add a span to a region
 *
 *	Adds span `left',`right' to region `rgnPtr'
 *	after the span at `index'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes contents of `rgnPtr'.
 *	!!! May change the value of rgnPtr->spans !!!
 *
 *----------------------------------------------------------------------
 */

static void
AppendSpan(rgnPtr, index, left, right)
    CtkRegion * rgnPtr;
    int index;
    int left;
    int right;
{
    int newIndex = AllocSpan(rgnPtr);
    RegionSpan *spans = rgnPtr->spans;

    spans[newIndex].left = left;
    spans[newIndex].right = right;
    spans[newIndex].next = spans[index].next;
    spans[index].next = newIndex;
}

/*
 *----------------------------------------------------------------------
 *
 * PrependSpan -- add a span to a region
 *
 *	Adds span `left',`right' to region `rgnPtr'
 *	before the span at `index'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes contents of `rgnPtr'.
 *	!!! May change the value of rgnPtr->spans !!!
 *
 *----------------------------------------------------------------------
 */

static void
PrependSpan(rgnPtr, index, left, right)
    CtkRegion * rgnPtr;
    int index;
    int left;
    int right;
{
    int newIndex = AllocSpan(rgnPtr);
    RegionSpan *spans = rgnPtr->spans;

    CopySpan(&spans[newIndex], &spans[index]);
    spans[index].left = left;
    spans[index].right = right;
    spans[index].next = newIndex;
    
}

/*
 *----------------------------------------------------------------------
 *
 * AllocSpan -- get a new span for a region
 *
 *	Allocates another span for region `rgnPtr'.
 *
 * Results:
 *	Returns index of new span.
 *
 * Side effects:
 *	!!! May change the value of rgnPtr->spans !!!
 *
 *----------------------------------------------------------------------
 */

static int
AllocSpan(rgnPtr)
    CtkRegion * rgnPtr;
{
    int i;
    int old_num;
    int new_num;

    if (rgnPtr->free == NO_SPAN) {
	/*
	 *  No spans in free list, allocate some more.
	 */
	old_num = rgnPtr->num_spans;
	new_num = old_num + 20;
	rgnPtr->spans = (RegionSpan *)
	    ckrealloc((char *) rgnPtr->spans, (new_num)*sizeof(RegionSpan));
	rgnPtr->num_spans = new_num;

	/*
	 *  Add the new spans (except one) to the regions free list.
	 */
	for (i=old_num+1; i < new_num; i++) {
	    rgnPtr->spans[i-1].next = i;
	}
	rgnPtr->spans[new_num-1].next = NO_SPAN;
	rgnPtr->free = old_num+1;

	/*
	 * Return the remaining new span.
	 */
	return (old_num);
    } else {
	/*
	 *  Spans in free list, return one.
	 */
	i = rgnPtr->free;
	rgnPtr->free = rgnPtr->spans[rgnPtr->free].next;
	return (i);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MergeSpan -- union a span into a region
 *
 *	Computes the union of region `rgnPtr' and span `left',`right'
 *	at line y, and stores it in `rgnPtr'.
 *	!!! `y' must be within rgnPtr->top and rgnPtr->bottom !!!
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	(Possibly) expands the region `rgnPtr'.
 *
 *----------------------------------------------------------------------
 */

static void
MergeSpan(rgnPtr, left, right, y)
    CtkRegion * rgnPtr;
    int left;
    int right;
    int y;
{
    RegionSpan *spans = rgnPtr->spans;
    int idx = y - rgnPtr->top;
    int lastIdx = NO_SPAN;
    int mergeIdx = NO_SPAN;

    if (y < rgnPtr->top || y > rgnPtr->bottom) {
    	panic("Merge span (y=%d) outside of regions vertical bounds (%d-%d)",
    		y, rgnPtr->top, rgnPtr->bottom);
    }

    if (spans[idx].left >= spans[idx].right) {
	/*
	 *  Empty scan line, replace it with the new span.
	 */
	spans[idx].left = left;
	spans[idx].right = right;
	return;
    }

    while (idx != NO_SPAN) {
	if (spans[idx].right >= left) {
	    /*
	     *	This spans is not left of the merge span.
	     */

	    if (spans[idx].left > right)  break; /* right of merge */

	    if (mergeIdx == NO_SPAN) {
		PseudoUnionSpans(
		    &spans[idx].left, &spans[idx].right, left, right);
		mergeIdx = idx;
	    } else {
		if (spans[mergeIdx].right < spans[idx].right) {
		    spans[mergeIdx].right = spans[idx].right;
		}
		idx = DeleteSpan(rgnPtr, idx, lastIdx);
		continue;
	    }
	}
    
	lastIdx = idx;
	idx = spans[idx].next;
    }

    if (mergeIdx == NO_SPAN) {
	/*
	 *  No merge performed, append merge span to scan line.
	 */
	if (lastIdx == NO_SPAN) {
	    PrependSpan(rgnPtr, idx, left, right);
	} else {
	    AppendSpan(rgnPtr, lastIdx, left, right);
	}
    }
}

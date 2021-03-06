/*
 * Internal definitions of the Cocoa rootless implementation
 */
/*
 * Copyright (c) 2003 Torrey T. Lyons. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#ifndef _CR_H
#define _CR_H

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#import "XView.h"
#else
typedef struct OpaqueNSWindow NSWindow;
typedef struct OpaqueXView XView;
#endif

#undef BOOL
#define BOOL xBOOL
#include "screenint.h"
#include "window.h"
#undef BOOL

// Predefined style for the window which is about to be framed
extern WindowPtr nextWindowToFrame;
extern unsigned int nextWindowStyle;

typedef struct {
    NSWindow *window;
    XView *view;
    GrafPtr port;
    CGContextRef context;
} CRWindowRec, *CRWindowPtr;

Bool CRInit(ScreenPtr pScreen);
void CRAppleWMInit(void);

#endif /* _CR_H */

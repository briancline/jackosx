/*	Copyright: 	© Copyright 2002 Apple Computer, Inc. All rights reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
			("Apple") in consideration of your agreement to the following terms, and your
			use, installation, modification or redistribution of this Apple software
			constitutes acceptance of these terms.  If you do not agree with these terms,
			please do not use, install, modify or redistribute this Apple software.

			In consideration of your agreement to abide by the following terms, and subject
			to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
			copyrights in this original Apple software (the "Apple Software"), to use,
			reproduce, modify and redistribute the Apple Software, with or without
			modifications, in source and/or binary forms; provided that if you redistribute
			the Apple Software in its entirety and without modifications, you must retain
			this notice and the following text and disclaimers in all such redistributions of
			the Apple Software.  Neither the name, trademarks, service marks or logos of
			Apple Computer, Inc. may be used to endorse or promote products derived from the
			Apple Software without specific prior written permission from Apple.  Except as
			expressly stated in this notice, no other rights or licenses, express or implied,
			are granted by Apple herein, including but not limited to any patent rights that
			may be infringed by your derivative works or by other works in which the Apple
			Software may be incorporated.

			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
			WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
			WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
			PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
			COMBINATION WITH YOUR PRODUCTS.

			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
			CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
			GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
			ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
			OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
			(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
			ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*=============================================================================
	CACFString.h

=============================================================================*/
#if !defined(__CACFString_h__)
#define __CACFString_h__

//=============================================================================
//	Includes
//=============================================================================

#include <CoreAudio/CoreAudioTypes.h>
#include <CoreFoundation/CFString.h>

//=============================================================================
//	CACFString
//=============================================================================

class	CACFString
{
//	Construction/Destruction
public:
				CACFString() : mCFString(NULL), mWillRelease(true) {}
				CACFString(CFStringRef inCFString, bool inWillRelease = true) : mCFString(inCFString), mWillRelease(inWillRelease) {}
				~CACFString() { Release(); }
				CACFString(const CACFString& inString) : mCFString(inString.mCFString), mWillRelease(inString.mWillRelease) { Retain(); }
	CACFString&	operator=(const CACFString& inString) { Release(); mCFString = inString.mCFString; mWillRelease = inString.mWillRelease; Retain(); return *this; }
	CACFString&	operator=(CFStringRef inCFString) { Release(); mCFString = inCFString; mWillRelease = true; return *this; }

private:
	void		Retain() { if(mWillRelease && (mCFString != NULL)) { CFRetain(mCFString); } }
	void		Release() { if(mWillRelease && (mCFString != NULL)) { CFRelease(mCFString); } }
	
	CFStringRef	mCFString;
	bool		mWillRelease;

//	Operations
public:
	void		AllowRelease() { mWillRelease = true; }
	void		DontAllowRelease() { mWillRelease = false; }
	bool		IsValid() { return mCFString != NULL; }

//	Value Access
public:
	CFStringRef	GetCFString() const { return mCFString; }
	CFStringRef	CopyCFString() const { if(mCFString != NULL) { CFRetain(mCFString); } return mCFString; }
	void		GetCString(char* outString, UInt32& ioStringSize) const;
	void		GetUnicodeString(UInt16* outString, UInt32& ioStringSize) const;

};

inline bool	operator<(const CACFString& x, const CACFString& y) { return CFStringCompare(x.GetCFString(), y.GetCFString(), 0) == kCFCompareLessThan; }
inline bool	operator==(const CACFString& x, const CACFString& y) { return CFStringCompare(x.GetCFString(), y.GetCFString(), 0) == kCFCompareEqualTo; }
inline bool	operator!=(const CACFString& x, const CACFString& y) { return !(x == y); }
inline bool	operator<=(const CACFString& x, const CACFString& y) { return (x < y) || (x == y); }
inline bool	operator>=(const CACFString& x, const CACFString& y) { return !(x < y); }
inline bool	operator>(const CACFString& x, const CACFString& y) { return !((x < y) || (x == y)); }

//=============================================================================
//	CACFMutableString
//=============================================================================

class	CACFMutableString
{
//	Construction/Destruction
public:
						CACFMutableString() : mCFMutableString(NULL), mWillRelease(true) {}
						CACFMutableString(CFMutableStringRef inCFMutableString, bool inWillRelease = true) : mCFMutableString(inCFMutableString), mWillRelease(inWillRelease) {}
						~CACFMutableString() { Release(); }
						CACFMutableString(const CACFMutableString& inString) : mCFMutableString(inString.mCFMutableString), mWillRelease(inString.mWillRelease) { Retain(); }
	CACFMutableString	operator=(const CACFMutableString& inString) { Release(); mCFMutableString = inString.mCFMutableString; mWillRelease = inString.mWillRelease; Retain(); return *this; }
	CACFMutableString	operator=(CFMutableStringRef inCFMutableString) { Release(); mCFMutableString = inCFMutableString; mWillRelease = true; return *this; }

private:
	void				Retain() { if(mWillRelease && (mCFMutableString != NULL)) { CFRetain(mCFMutableString); } }
	void				Release() { if(mWillRelease && (mCFMutableString != NULL)) { CFRelease(mCFMutableString); } }
	
	CFMutableStringRef	mCFMutableString;
	bool				mWillRelease;

//	Operations
public:
	void				AllowRelease() { mWillRelease = true; }
	void				DontAllowRelease() { mWillRelease = false; }
	bool				IsValid() { return mCFMutableString != NULL; }

//	Value Access
public:
	CFMutableStringRef	GetCFMutableString() const { return mCFMutableString; }
	CFMutableStringRef	CopyCFMutableString() const { if(mCFMutableString != NULL) { CFRetain(mCFMutableString); } return mCFMutableString; }
	void				GetCString(char* outString, UInt32& ioStringSize) const;
	void				GetUnicodeString(UInt16* outString, UInt32& ioStringSize) const;

};

#endif

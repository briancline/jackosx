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
	CAVolumeCurve.h

=============================================================================*/
#if !defined(__CAVolumeCurve_h__)
#define __CAVolumeCurve_h__

//=============================================================================
//	Includes
//=============================================================================

#include <CoreAudio/CoreAudioTypes.h>
#include <map>

//=============================================================================
//	Types
//=============================================================================

struct CAScalarPoint
{
	SInt32	mMinimum;
	SInt32	mMaximum;
	
	CAScalarPoint() : mMinimum(0), mMaximum(0) {}
	CAScalarPoint(const CAScalarPoint& inPoint) : mMinimum(inPoint.mMinimum), mMaximum(inPoint.mMaximum) {}
	CAScalarPoint(SInt32 inMinimum, SInt32 inMaximum) : mMinimum(inMinimum), mMaximum(inMaximum) {}
	CAScalarPoint&	operator=(const CAScalarPoint& inPoint) { mMinimum = inPoint.mMinimum; mMaximum = inPoint.mMaximum; return *this; }
	
	static bool	Overlap(const CAScalarPoint& x, const CAScalarPoint& y) { return (x.mMinimum < y.mMaximum) && (x.mMaximum > y.mMinimum); }
};

inline bool	operator<(const CAScalarPoint& x, const CAScalarPoint& y) { return x.mMinimum < y.mMinimum; }
inline bool	operator==(const CAScalarPoint& x, const CAScalarPoint& y) { return (x.mMinimum == y.mMinimum) && (x.mMaximum == y.mMaximum); }
inline bool	operator!=(const CAScalarPoint& x, const CAScalarPoint& y) { return !(x == y); }
inline bool	operator<=(const CAScalarPoint& x, const CAScalarPoint& y) { return (x < y) || (x == y); }
inline bool	operator>=(const CAScalarPoint& x, const CAScalarPoint& y) { return !(x < y); }
inline bool	operator>(const CAScalarPoint& x, const CAScalarPoint& y) { return !((x < y) || (x == y)); }

struct CADBPoint
{
	Float64	mMinimum;
	Float64	mMaximum;
	
	CADBPoint() : mMinimum(0), mMaximum(0) {}
	CADBPoint(const CADBPoint& inPoint) : mMinimum(inPoint.mMinimum), mMaximum(inPoint.mMaximum) {}
	CADBPoint(Float64 inMinimum, Float64 inMaximum) : mMinimum(inMinimum), mMaximum(inMaximum) {}
	CADBPoint&	operator=(const CADBPoint& inPoint) { mMinimum = inPoint.mMinimum; mMaximum = inPoint.mMaximum; return *this; }
	
	static bool	Overlap(const CADBPoint& x, const CADBPoint& y) { return (x.mMinimum < y.mMaximum) && (x.mMaximum >= y.mMinimum); }
};

inline bool	operator<(const CADBPoint& x, const CADBPoint& y) { return x.mMinimum < y.mMinimum; }
inline bool	operator==(const CADBPoint& x, const CADBPoint& y) { return (x.mMinimum == y.mMinimum) && (x.mMaximum == y.mMaximum); }
inline bool	operator!=(const CADBPoint& x, const CADBPoint& y) { return !(x == y); }
inline bool	operator<=(const CADBPoint& x, const CADBPoint& y) { return (x < y) || (x == y); }
inline bool	operator>=(const CADBPoint& x, const CADBPoint& y) { return !(x < y); }
inline bool	operator>(const CADBPoint& x, const CADBPoint& y) { return !((x < y) || (x == y)); }

//=============================================================================
//	CAVolumeCurve
//=============================================================================

class CAVolumeCurve
{

//	Construction/Destruction
public:
					CAVolumeCurve();
	virtual			~CAVolumeCurve();

//	Attributes
public:
	SInt32			GetMinimumScalar() const;
	SInt32			GetMaximumScalar() const;
	Float64			GetMinimumDB() const;
	Float64			GetMaximumDB() const;

//	Operations
public:
	void			AddRange(SInt32 mMinScalar, SInt32 mMaxScalar, Float64 inMinDB, Float64 inMaxDB);
	void			ResetRange();
	bool			CheckForContinuity() const;
	
	SInt32			ConvertDBToScalar(Float64 inDB) const;
	Float64			ConvertScalarToDB(SInt32 inScalar) const;
	Float64			ConvertScalarToPercent(SInt32 inScalar) const;
	Float64			ConvertDBToPercent(Float64 inDB) const;
	SInt32			ConvertPercentToScalar(Float64 inPercent) const;
	Float64			ConvertPercentToDB(Float64 inPercent) const;

//	Implementation
private:
	typedef	std::map<CAScalarPoint, CADBPoint>	CurveMap;
	
	CurveMap		mCurveMap;

};

#endif

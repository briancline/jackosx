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
	CAVolumeCurve.cpp

=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================

#include "CAVolumeCurve.h"
#include "CADebugMacros.h"
#include <math.h>

//=============================================================================
//	CAVolumeCurve
//=============================================================================

CAVolumeCurve::CAVolumeCurve()
:
	mCurveMap()
{
}

CAVolumeCurve::~CAVolumeCurve()
{
}

SInt32	CAVolumeCurve::GetMinimumScalar() const
{
	SInt32 theAnswer = 0;
	
	if(!mCurveMap.empty())
	{
		CurveMap::const_iterator theIterator = mCurveMap.begin();
		theAnswer = theIterator->first.mMinimum;
	}
	
	return theAnswer;
}

SInt32	CAVolumeCurve::GetMaximumScalar() const
{
	SInt32 theAnswer = 0;
	
	if(!mCurveMap.empty())
	{
		CurveMap::const_iterator theIterator = mCurveMap.begin();
		std::advance(theIterator, mCurveMap.size() - 1);
		theAnswer = theIterator->first.mMaximum;
	}
	
	return theAnswer;
}

Float64	CAVolumeCurve::GetMinimumDB() const
{
	Float64 theAnswer = 0;
	
	if(!mCurveMap.empty())
	{
		CurveMap::const_iterator theIterator = mCurveMap.begin();
		theAnswer = theIterator->second.mMinimum;
	}
	
	return theAnswer;
}

Float64	CAVolumeCurve::GetMaximumDB() const
{
	Float64 theAnswer = 0;
	
	if(!mCurveMap.empty())
	{
		CurveMap::const_iterator theIterator = mCurveMap.begin();
		std::advance(theIterator, mCurveMap.size() - 1);
		theAnswer = theIterator->second.mMaximum;
	}
	
	return theAnswer;
}

void	CAVolumeCurve::AddRange(SInt32 inMinScalar, SInt32 inMaxScalar, Float64 inMinDB, Float64 inMaxDB)
{
	CAScalarPoint theScalar(inMinScalar, inMaxScalar);
	CADBPoint theDB(inMinDB, inMaxDB);
	
	bool isOverlapped = false;
	bool isDone = false;
	CurveMap::iterator theIterator = mCurveMap.begin();
	while((theIterator != mCurveMap.end()) && !isOverlapped && !isDone)
	{
		isOverlapped = CAScalarPoint::Overlap(theScalar, theIterator->first);
		isDone = theScalar >= theIterator->first;
		
		if(!isOverlapped && !isDone)
		{
			std::advance(theIterator, 1);
		}
	}
	
	if(!isOverlapped)
	{
		mCurveMap.insert(CurveMap::value_type(theScalar, theDB));
	}
	else
	{
		DebugMessage("CAVolumeCurve::AddRange: new point overlaps");
	}
}

void	CAVolumeCurve::ResetRange()
{
	mCurveMap.clear();
}

bool	CAVolumeCurve::CheckForContinuity() const
{
	bool theAnswer = true;
	
	CurveMap::const_iterator theIterator = mCurveMap.begin();
	if(theIterator != mCurveMap.end())
	{
		SInt32 theScalar = theIterator->first.mMinimum;
		Float64 theDB = theIterator->second.mMinimum;
		do
		{
			SInt32 theScalarMin = theIterator->first.mMinimum;
			SInt32 theScalarMax = theIterator->first.mMaximum;
			SInt32 theScalarRange = theScalarMax - theScalarMin;
			
			Float64 theDBMin = theIterator->second.mMinimum;
			Float64 theDBMax = theIterator->second.mMaximum;
			Float64 theDBRange = theDBMax - theDBMin;

			theAnswer = theScalar == theScalarMin;
			theAnswer = theDB == theDBMin;
			
			theScalar += theScalarRange;
			theDB += theDBRange;
			
			std::advance(theIterator, 1);
		}
		while((theIterator != mCurveMap.end()) && theAnswer);
	}
	
	return theAnswer;
}

SInt32	CAVolumeCurve::ConvertDBToScalar(Float64 inDB) const
{
	SInt32 theAnswer = 0;
	
	Float64 theOverallDBMin = GetMinimumDB();
	Float64 theOverallDBMax = GetMaximumDB();
	
	if(inDB < theOverallDBMin) inDB = theOverallDBMin;
	if(inDB > theOverallDBMax) inDB = theOverallDBMax;

	CurveMap::const_iterator theIterator = mCurveMap.begin();
	while(theIterator != mCurveMap.end())
	{
		SInt32 theScalarMin = theIterator->first.mMinimum;
		SInt32 theScalarMax = theIterator->first.mMaximum;
		SInt32 theScalarRange = theScalarMax - theScalarMin;
		
		Float64 theDBMin = theIterator->second.mMinimum;
		Float64 theDBMax = theIterator->second.mMaximum;
		Float64 theDBRange = theDBMax - theDBMin;
		
		Float64 theScalarPerDB = static_cast<Float64>(theScalarRange) / theDBRange;

		if((inDB >= theDBMin) && (inDB <= theDBMax))
		{
			Float64 theDBOffset = inDB - theDBMin;
			theAnswer = theScalarMin + static_cast<SInt32>(theDBOffset * theScalarPerDB);
			break;
		}
		std::advance(theIterator, 1);
	}
	
	return theAnswer;
}

Float64	CAVolumeCurve::ConvertScalarToDB(SInt32 inScalar) const
{
	Float64 theAnswer = 0;
	
	SInt32 theOverallScalarMin = GetMinimumScalar();
	SInt32 theOverallScalarMax = GetMaximumScalar();
	
	if(inScalar < theOverallScalarMin) inScalar = theOverallScalarMin;
	if(inScalar > theOverallScalarMax) inScalar = theOverallScalarMax;

	CurveMap::const_iterator theIterator = mCurveMap.begin();
	while(theIterator != mCurveMap.end())
	{
		SInt32 theScalarMin = theIterator->first.mMinimum;
		SInt32 theScalarMax = theIterator->first.mMaximum;
		SInt32 theScalarRange = theScalarMax - theScalarMin;
		
		Float64 theDBMin = theIterator->second.mMinimum;
		Float64 theDBMax = theIterator->second.mMaximum;
		Float64 theDBRange = theDBMax - theDBMin;
		
		Float64 theDBPerScalar = theDBRange / static_cast<Float64>(theScalarRange);

		if((inScalar >= theScalarMin) && (inScalar <= theScalarMax))
		{
			Float64 theScalarOffset = inScalar - theScalarMin;
			theAnswer = theDBMin + (theScalarOffset * theDBPerScalar);
			break;
		}
		std::advance(theIterator, 1);
	}
	
	return theAnswer;
}

Float64	CAVolumeCurve::ConvertScalarToPercent(SInt32 inScalar) const
{
	Float64 theDB = ConvertScalarToDB(inScalar);
	return ConvertDBToPercent(theDB);
}

Float64	CAVolumeCurve::ConvertDBToPercent(Float64 inDB) const
{
	Float64 theDBMin = GetMinimumDB();
	Float64 theDBMax = GetMaximumDB();
	
	if(inDB < theDBMin) inDB = theDBMin;
	if(inDB > theDBMax) inDB = theDBMax;
	
	Float64 theAnswer = (inDB - theDBMin) / (theDBMax - theDBMin);
	if((theDBMax - theDBMin) > 30)
	{
		theAnswer = pow(theAnswer, 2.0);
	}

	return theAnswer;
}

SInt32	CAVolumeCurve::ConvertPercentToScalar(Float64 inPercent) const
{
	Float64 theDB = ConvertPercentToDB(inPercent);
	return ConvertDBToScalar(theDB);
}

Float64	CAVolumeCurve::ConvertPercentToDB(Float64 inPercent) const
{
	Float64 theDBMin = GetMinimumDB();
	Float64 theDBMax = GetMaximumDB();
	
	Float64 theDBValue = inPercent;
	if((theDBMax - theDBMin) > 30.0)
	{
		theDBValue = pow(theDBValue, 1.0 / 2.0);
	}
	theDBValue = theDBMin + (theDBValue * (theDBMax - theDBMin));
	
	return theDBValue;
}

/*
  Copyright ©  Johnny Petrantoni 2003
 
  This library is free software; you can redistribute it and modify it under
  the terms of the GNU Library General Public License as published by the
  Free Software Foundation version 2 of the License, or any later version.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License
  for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// JackVSTMain.cpp

#include "JackVST.hpp"

bool oome = false;


extern "C"  { AEffect *vst_main(audioMasterCallback audioMaster); } 

AEffect *vst_main (audioMasterCallback audioMaster)
{
	// Get VST Version
	if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0))
		return 0;  // old version

	// Create the AudioEffect
	JackVST* effect = new JackVST (audioMaster);
	if (!effect)
		return 0;

	// Check if no problem in constructor of JackVST
	if (oome)
	{
		delete effect;
		return 0;
	}
	return effect->getAeffect ();
}

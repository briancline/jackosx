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

// bequite.h

#include <CoreServices/CoreServices.h>

#ifdef __cplusplus
extern "C" {
#endif

int quantiProc(void) ;
char* ottieniNome(int n) ;
int ottieniPid(int n);
int ottieniFlag(int n);
char * ottieniNomeFromPid(int pid);

#ifdef __cplusplus
}
#endif
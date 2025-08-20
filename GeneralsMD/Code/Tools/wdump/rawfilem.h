/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                     $Archive:: /G/wdump/RAWFILEM.H                                         $*
 *                                                                                             *
 *                      $Author:: Eric_c                                                      $*
 *                                                                                             *
 *                     $Modtime:: 7/28/97 3:36p                                               $*
 *                                                                                             *
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   RawFileMClass::File_Name -- Returns with the filename associate with the file object.      *
 *   RawFileMClass::RawFileMClass -- Default constructor for a file object.                      *
 *   RawFileMClass::~RawFileMClass -- Default deconstructor for a file object.                   *
 *   RawFileMClass::Is_Open -- Checks to see if the file is open or not.                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef RAWFILEM_Hx
#define RAWFILEM_Hx

#include	<errno.h>
#include	<limits.h>
#include	<stddef.h>
#include	<stdlib.h>

#include	"win.h"

#define	NULL_HANDLE		INVALID_HANDLE_VALUE
#define	HANDLE_TYPE		HANDLE

#include	"WWFILE.H"

#ifdef NEVER
	/*
	**	This is a duplicate of the error numbers. The error handler for the RawFileMClass handles
	**	these errors. If the error routine is overridden and additional errors are defined, then
	**	use numbers starting with 100. Note that these errors here are listed in numerical order.
	**	These errors are defined in the standard header file "ERRNO.H".
	*/
	EZERO,				// Non-error.
	EINVFNC,				// Invalid function number.
	ENOFILE,				// File not found.
	ENOENT=ENOFILE,	// No such file or directory.
	ENOPATH,				// Path not found.
	EMFILE,				// Too many open files.
	EACCES,				// Permission denied.
	EBADF,				// Bad file number.
	ECONTR,				// Memory blocks destroyed.
	ENOMEM,				// Not enough core memory.
	EINVMEM,				// Invalid memory block address.
	EINVENV,				// Invalid environment.
	EINVFMT,				// Invalid format.
	EINVACC,				// Invalid access code.
	EINVDAT,				// Invalid data.
	EFAULT,				// Unknown error.
	EINVDRV,				// Invalid drive specified.
	ENODEV=EINVDRV,	// No such device.
	ECURDIR,				// Attempt to remove CurDir.
	ENOTSAM,				// Not same device.
	ENMFILE,				// No more files.
	EINVAL,				// Invalid argument.
	E2BIG,				// Argument list too long.
	ENOEXEC,				// exec format error.
	EXDEV,				// Cross-device link.
	ENFILE,				// Too many open files.
	ECHILD,				// No child process.
	ENOTTY,				// not used
	ETXTBSY,				// not used
	EFBIG,				// not used
	ENOSPC,				// No space left on device.
	ESPIPE,				// Illegal seek.
	EROFS,				// Read-only file system.
	EMLINK,				// not used
	EPIPE,				// Broken pipe.
	EDOM,					// Math argument.
	ERANGE,				// Result too large.
	EEXIST,				// File already exists.
	EDEADLOCK,			// Locking violation.
	EPERM,				// Operation not permitted.
	ESRCH,				// not used
	EINTR,				// Interrupted function call.
	EIO,					// Input/output error.
	ENXIO,				// No such device or address.
	EAGAIN,				// Resource temporarily unavailable.
	ENOTBLK,				// not used
	EBUSY,				// Resource busy.
	ENOTDIR,				// not used
	EISDIR,				// not used
	EUCLEAN,				// not used
#endif

#ifndef WWERROR
#define WWERROR	-1
#endif

/*
**	This is the definition of the raw file class. It is derived from the abstract base FileClass
**	and handles the interface to the low level DOS routines. This is the first class in the
**	chain of derived file classes that actually performs a useful function. With this class,
**	I/O is possible. More sophisticated features, such as packed files, CD-ROM support,
**	file caching, and XMS/EMS memory support, are handled by derived classes.
**
**	Of particular importance is the need to override the error routine if more sophisticated
**	error handling is required. This is more than likely if greater functionality is derived
**	from this base class.
*/
class RawFileMClass : public FileClass
{
		typedef FileClass BASECLASS;

	public:

		/*
		**	This is a record of the access rights used to open the file. These rights are
		**	used if the file object is duplicated.
		*/
		int Rights;
		int Error_Number; // added by ehc to allow multithread library usage

		RawFileMClass(char const *filename);
		RawFileMClass(void);
		RawFileMClass (RawFileMClass const & f);
		RawFileMClass & operator = (RawFileMClass const & f);
		virtual ~RawFileMClass(void);

		virtual char const * File_Name(void) const;
		virtual char const * Set_Name(char const *filename);
		virtual int Create(void);
		virtual int Delete(void);
		virtual bool Is_Available(int forced=false);
		virtual bool Is_Open(void) const;
		virtual int Open(char const *filename, int rights=READ);
		virtual int Open(int rights=READ);
		virtual int Read(void *buffer, int size);
		virtual int Seek(int pos, int dir=SEEK_CUR);
		virtual int Size(void);
		virtual int Write(void const *buffer, int size);
		virtual void Close(void);
		virtual unsigned long Get_Date_Time(void);
		virtual bool Set_Date_Time(unsigned long datetime);
		virtual void Error(int error, int canretry = false, char const * filename=NULL);

		void Bias(int start, int length=-1);

		HANDLE_TYPE Get_File_Handle(void) { return (Handle); };

		/*
		**	These bias values enable a sub-portion of a file to appear as if it
		**	were the whole file. This comes in very handy for multi-part files such as
		**	mixfiles.
		*/
		int BiasStart;
		int BiasLength;

	protected:

		/*
		**	This function returns the largest size a low level DOS read or write may
		**	perform. Larger file transfers are performed in chunks of this size or less.
		*/
		int Transfer_Block_Size(void) {return (int)((unsigned)UINT_MAX)-16L;};

		int Raw_Seek(int pos, int dir=SEEK_CUR);

	private:

		/*
		**	This is the low level DOS handle. A -1 indicates an empty condition.
		*/
		HANDLE_TYPE Handle;

		/*
		**	This points to the filename as a NULL terminated string. It may point to either a
		**	constant or an allocated string as indicated by the "Allocated" flag.
		*/
		char const * Filename;

		//
		// file date and time are in the following formats:
		//
		//      date   bits 0-4   day (0-31)
		//             bits 5-8   month (1-12)
		//             bits 9-15  year (0-119 representing 1980-2099)
		//
		//      time   bits 0-4   second/2 (0-29)
		//             bits 5-10  minutes (0-59)
		//             bits 11-15 hours (0-23)
		//
		unsigned short Date;
		unsigned short Time;

		/*
		**	Filenames that were assigned as part of the construction process
		**	are not allocated. It is assumed that the filename string is a
		**	constant in that case and thus making duplication unnecessary.
		**	This value will be non-zero if the filename has be allocated
		**	(using strdup()).
		*/
		bool Allocated;
};


/***********************************************************************************************
 * RawFileMClass::File_Name -- Returns with the filename associate with the file object.        *
 *                                                                                             *
 *    Use this routine to determine what filename is associated with this file object. If no   *
 *    filename has yet been assigned, then this routing will return NULL.                      *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the file name associated with this file object or NULL   *
 *          if one doesn't exist.                                                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
inline char const * RawFileMClass::File_Name(void) const
{
	return(Filename);
}


/***********************************************************************************************
 * RawFileMClass::RawFileMClass -- Default constructor for a file object.                        *
 *                                                                                             *
 *    This constructs a null file object. A null file object has no file handle or filename    *
 *    associated with it. In order to use a file object created in this fashion it must be     *
 *    assigned a name and then opened.                                                         *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
inline RawFileMClass::RawFileMClass(void) :
	Rights(READ),
	BiasStart(0),
	BiasLength(-1),
	Handle(INVALID_HANDLE_VALUE),
	Filename(0),
	Date(0),
	Time(0),
	Allocated(false)
{
}


/***********************************************************************************************
 * RawFileMClass::~RawFileMClass -- Default deconstructor for a file object.                     *
 *                                                                                             *
 *    This constructs a null file object. A null file object has no file handle or filename    *
 *    associated with it. In order to use a file object created in this fashion it must be     *
 *    assigned a name and then opened.                                                         *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
inline RawFileMClass::~RawFileMClass(void)
{
	Close();
	if (Allocated && Filename) {
		free((char *)Filename);
		Filename = NULL;
		Allocated = false;
	}
}


/***********************************************************************************************
 * RawFileMClass::Is_Open -- Checks to see if the file is open or not.                          *
 *                                                                                             *
 *    Use this routine to determine if the file is open. It returns true if it is.             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Is the file open?                                                            *
 *                                                                                             *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
inline bool RawFileMClass::Is_Open(void) const
{
	return(Handle != INVALID_HANDLE_VALUE);
}

#endif

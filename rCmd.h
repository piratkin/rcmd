///////////////////////////////////////////////////////////////////////////////////
//
// Author: Zoltan Csizmadia, 3/2001
//
// This code is freeware! If you change it, or want to use it, 
// please send an email!
//
// Email: zoltan_csizmadia@yahoo.com
//
// For companies(Austin,TX): If you would like to get my resume, send an email!
//
///////////////////////////////////////////////////////////////////////////////////
//
// xCmd.h
//
// History:
//
//    3/27/2001      Initial version
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef XCMD_H_INCLUDED
#define XCMD_H_INCLUDED

#define SERVICENAME        _T("xCmdSvc")
#define LONGSERVICENAME    _T("xCmd Service")

#define XCMDSVCEXE         _T("xCmdSvc.exe")

#define XCMDCOMM           _T("xCmd_communicaton")
#define XCMDSTDOUT         _T("xCmd_stdout")
#define XCMDSTDIN          _T("xCmd_stdin")
#define XCMDSTDERR         _T("xCmd_stderr")

#define StdOutput(x)       { _ftprintf( stdout, _T("%s"), x); fflush(stdout); }
#define StdError(x)        { _ftprintf( stderr, _T("%s"), x); fflush(stderr); }

class xCmdMessage
{
public:
   TCHAR szCommand[0x1000];
   TCHAR szWorkingDir[_MAX_PATH];
   DWORD dwPriority;
   DWORD dwProcessId;
   TCHAR szMachine[_MAX_PATH];
   BOOL  bNoWait;    
};

class xCmdResponse
{
public:
   DWORD dwErrorCode;
   DWORD dwReturnCode;
};

#endif
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
// xCmdSvc.cpp
//
// History:
//
//    3/27/2001      Initial version
//
///////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsvc.h>
#include <process.h>
#include "xCmdSvc.h"
#include "../xCmd.h"

void CommunicationPoolThread(PVOID);
void CommunicationPipeThreadProc(PVOID);
DWORD Execute(xCmdMessage*, DWORD*);

LONG  dwSvcPipeInstanceCount = 0;
TCHAR szStdOutPipe[_MAX_PATH] = _T("");
TCHAR szStdInPipe[_MAX_PATH] = _T("");
TCHAR szStdErrPipe[_MAX_PATH] = _T("");

// Service "main" function
void _ServiceMain( void* )
{
   // Start CommunicationPoolThread, which handles the incoming instances
   _beginthread( CommunicationPoolThread, 0, NULL );

   // Waiting for stop the service
   while( WaitForSingleObject( hStopServiceEvent, 10 ) != WAIT_OBJECT_0 )
   {
   }
   
   // Let's delete itself, after the service stopped
   DeleteSvc();

   CloseHandle( hStopServiceEvent );
}

// Communicaton Thread Pool, handles the incoming xCmd.exe requests
void CommunicationPoolThread(PVOID)
{
    HANDLE hPipe = NULL;
    
    for (;;)
    {
        SECURITY_ATTRIBUTES SecAttrib = {0};
        SECURITY_DESCRIPTOR SecDesc;
        InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&SecDesc, TRUE, NULL, TRUE);

        SecAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
        SecAttrib.lpSecurityDescriptor = &SecDesc;;
        SecAttrib.bInheritHandle = TRUE;

        // Create communication pipe
        hPipe = CreateNamedPipe(
            _T("\\\\.\\pipe\\")XCMDCOMM, 
            PIPE_ACCESS_DUPLEX, 
            PIPE_TYPE_MESSAGE | PIPE_WAIT, 
            PIPE_UNLIMITED_INSTANCES,
            0,
            0,
            (DWORD)-1,
            &SecAttrib);

        if ( hPipe != NULL )
        {
            // Waiting for client to connect to this pipe
            ConnectNamedPipe( hPipe, NULL );
            _beginthread( CommunicationPipeThreadProc, 0, (void*)hPipe);
        }
    }
}

// Handles a client
void CommunicationPipeThreadProc( void* pParam )
{
   HANDLE hPipe = (HANDLE)pParam;

   xCmdMessage msg;
   xCmdResponse response;

   DWORD dwWritten;
   DWORD dwRead;

   // Increment instance counter 
   InterlockedIncrement( &dwSvcPipeInstanceCount );

   ::ZeroMemory( &response, sizeof(response) );

   // Waiting for communication message from client
   if ( !ReadFile( hPipe, &msg, sizeof(msg), &dwRead, NULL ) || dwRead == 0 )
      goto cleanup;

   // Execute the requested command
   response.dwErrorCode  = Execute( &msg, &response.dwReturnCode );
   
   // Send back the response message (client is waiting for this response)
   if ( !WriteFile( hPipe, &response, sizeof(response), &dwWritten, NULL ) || dwWritten == 0 )
      goto cleanup;

cleanup:

   DisconnectNamedPipe( hPipe );
   CloseHandle( hPipe );

   // Decrement instance counter 
   InterlockedDecrement( &dwSvcPipeInstanceCount );

   // If this was the last client, let's stop ourself
   if ( dwSvcPipeInstanceCount == 0 )
      SetEvent( hStopServiceEvent );
     
}

// Creates named pipes for stdout, stderr, stdin
BOOL CreateStdPipes( xCmdMessage* pMsg, STARTUPINFO* psi )
{
   SECURITY_ATTRIBUTES SecAttrib = {0};
   SECURITY_DESCRIPTOR SecDesc;
   InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION);
   SetSecurityDescriptorDacl(&SecDesc, TRUE, NULL, FALSE);

   SecAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
   SecAttrib.lpSecurityDescriptor = &SecDesc;;
   SecAttrib.bInheritHandle = TRUE;

   psi->dwFlags |= STARTF_USESTDHANDLES;
   psi->hStdOutput = INVALID_HANDLE_VALUE;
   psi->hStdInput = INVALID_HANDLE_VALUE;
   psi->hStdError = INVALID_HANDLE_VALUE;

   // StdOut pipe name
   _stprintf( szStdOutPipe, _T("\\\\.\\pipe\\%s%s%d"), 
            XCMDSTDOUT, 
            pMsg->szMachine,
            pMsg->dwProcessId );

   // StdIn pipe name
   _stprintf( szStdInPipe, _T("\\\\.\\pipe\\%s%s%d"), 
            XCMDSTDIN, 
            pMsg->szMachine,
            pMsg->dwProcessId );

   // StdError pipe name
   _stprintf( szStdErrPipe, _T("\\\\.\\pipe\\%s%s%d"), 
            XCMDSTDERR, 
            pMsg->szMachine,
            pMsg->dwProcessId );

   // Create StdOut pipe
   psi->hStdOutput = CreateNamedPipe(
            szStdOutPipe, 
            PIPE_ACCESS_OUTBOUND, 
            PIPE_TYPE_MESSAGE | PIPE_WAIT, 
            PIPE_UNLIMITED_INSTANCES,
            0,
            0,
            (DWORD)-1,
            &SecAttrib);

   // Create StdError pipe
   psi->hStdError = CreateNamedPipe(
            szStdErrPipe, 
            PIPE_ACCESS_OUTBOUND, 
            PIPE_TYPE_MESSAGE | PIPE_WAIT, 
            PIPE_UNLIMITED_INSTANCES,
            0,
            0,
            (DWORD)-1,
            &SecAttrib);

   // Create StdIn pipe
   psi->hStdInput = CreateNamedPipe(
            szStdInPipe, 
            PIPE_ACCESS_INBOUND, 
            PIPE_TYPE_MESSAGE | PIPE_WAIT, 
            PIPE_UNLIMITED_INSTANCES,
            0,
            0,
            (DWORD)-1,
            &SecAttrib);

   if (  psi->hStdOutput == INVALID_HANDLE_VALUE ||
         psi->hStdError == INVALID_HANDLE_VALUE ||
         psi->hStdInput == INVALID_HANDLE_VALUE )
   {
      CloseHandle( psi->hStdOutput );
      CloseHandle( psi->hStdError );
      CloseHandle( psi->hStdInput );

      return FALSE;
   }

   // Waiting for client to connect to this pipe
   ConnectNamedPipe( psi->hStdOutput, NULL );
   ConnectNamedPipe( psi->hStdInput, NULL );
   ConnectNamedPipe( psi->hStdError, NULL );

   return TRUE;
}

// Execute the requested client command
DWORD Execute( xCmdMessage* pMsg, DWORD* pReturnCode )
{
   DWORD rc;
   TCHAR szCommand[_MAX_PATH];
   PROCESS_INFORMATION pi;
   STARTUPINFO si;

   ::ZeroMemory( &si, sizeof(si) );
   si.cb = sizeof(si);

   // Creates named pipes for stdout, stdin, stderr
   // Client will sit on these pipes
   if ( !CreateStdPipes( pMsg, &si ) )
      return 2;
   
   *pReturnCode = 0;
   rc = 0;

   // Initializes command
   // cmd.exe /c /q allows us to execute internal dos commands too.
   _stprintf( szCommand, _T("cmd.exe /q /c \"%s\""), pMsg->szCommand );
   
   // Start the requested process
   if ( CreateProcess( 
         NULL, 
         szCommand, 
         NULL,
         NULL, 
         TRUE,
         pMsg->dwPriority | CREATE_NO_WINDOW,
         NULL, 
         pMsg->szWorkingDir[0] != _T('\0') ? pMsg->szWorkingDir : NULL, 
         &si, 
         &pi ) )
   {
      HANDLE hProcess = pi.hProcess;

      *pReturnCode = 0;

      // Waiting for process to terminate
      if ( !pMsg->bNoWait )
      {
         WaitForSingleObject( hProcess, INFINITE );
         GetExitCodeProcess( hProcess, pReturnCode );
      }
   }
   else
      rc = 1;
   
   return rc;
}

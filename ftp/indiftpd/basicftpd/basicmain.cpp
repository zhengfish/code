// Copyright (C) 2003 by Oren Avissar
// (go to www.dftpd.com for more information)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "../core/Sock.h"
#include "../core/Thr.h"
#include "../core/Timer.h"
#include "../core/CmdLine.h"
#include "../core/StrUtils.h"
#include "../core/Ftps.h"       //this class is usually overrided
#include "../core/SiteInfo.h"   //this class is usually overrided

#define _MAINLOOPFREQ 2     //number of seconds the main loop runs at
#define _USERLOOPFREQ 1     //the freq of the loop in the user thread

#define _PROGNAME "BasicFTPD"
#define _PROGVERSION "1.0.0"

#define _LOGINSTR "Connected to BasicFTPD"

    //stores if the user quit (!0 = exit the server)
static int _flagquit = 0; 
    //stores the number of current client connections
static int _nconnections = 0;

    //Standard signal handler used to catch signals such as SIGINT (Ctrl-C)
static void _sighandler(int sig);
    //Used to check if the user entered a transfer command.
static int _iscommandxfer(char *command);

    //The main function that handles the client connections
#ifdef WIN32
  static void _connect(void *vpftps);   //WINDOWS must return "void" to run as a new thread
#else
  static void *_connect(void *vpftps);  //UNIX must return "void *" to run as a new thread
#endif


int main(int argc, char **argv)
{
    SOCKET sd, clientsd;
    CSock sock;
    CSiteInfo *psiteinfo;
    CThr thr;
    CTimer timer;
    CFtps *pftps;

        //Set the signal handlers
    signal(SIGINT,_sighandler);     //Ctrl-C
    signal(SIGTERM,_sighandler);
    signal(SIGABRT,_sighandler);
    #ifdef WIN32
      signal(SIGBREAK,_sighandler); //Ctrl-Break
    #else
      signal(SIGQUIT,_sighandler);
      signal(SIGKILL,_sighandler);
      signal(SIGSTOP,_sighandler);
      signal(SIGALRM,_sighandler);
      signal(SIGHUP,_sighandler);
      signal(SIGPIPE,_sighandler);
    #endif

    if (argc < 2) {
            //display the usage
        printf("%s usage: basicftpd <port>\n",_PROGNAME);
        return(0);
    }

        //initialize the network sockets
    sock.Initialize();

        //Create the class to store/access the site information
    if ((psiteinfo = new CSiteInfo(_PROGNAME,_PROGVERSION,FTPS_COREVERSION)) == NULL) {
        printf("ERROR: unable to allocate site information structure\n");
        return(1);
    }

        //Start the program log in display only logging mode.
    psiteinfo->SetProgLoggingLevel(SITEINFO_LOGLEVELDISPLAY);
    psiteinfo->SetAccessLoggingLevel(SITEINFO_LOGLEVELDISPLAY);

        //create the primary listening socket for the server
        //this is the socket that listens on the FTP control port
    psiteinfo->WriteToProgLog("MAIN","Exit using \"Ctrl-C\".");
    psiteinfo->WriteToProgLog("MAIN","Starting server on port %s...",argv[1]);
    sd = sock.OpenServer(argv[1]);
    if (sd == SOCK_INVALID) {
        psiteinfo->WriteToProgLog("MAIN","ERROR: Unable to start the server (sd = %d).",sd);
        delete psiteinfo;
        return(1);
    }

        //loop until the user quits
    while (_flagquit == 0) {
            //loop every _MAINLOOPFREQ sec
        if (sock.CheckStatus(sd,_MAINLOOPFREQ) > 0) {
            if ((clientsd = sock.Accept(sd)) != SOCK_INVALID) {
                    //Create the FTP server class to handle the client connection
                if ((pftps = new CFtps(clientsd,psiteinfo)) != NULL) {
                    psiteinfo->WriteToProgLog("MAIN","Connection established from %s:%s -> %s (%u).",
                                              pftps->GetClientIP(),pftps->GetClientPort(),pftps->GetClientName(),clientsd);
                    psiteinfo->WriteToAccessLog(pftps->GetServerIP(),pftps->GetServerPort(),pftps->GetClientIP(),
                                                pftps->GetClientPort(),pftps->GetLogin(),"CONNECT","","","");
                        //start a new thread to handle the client connection
                    if (thr.Create(_connect,(void *)pftps) == 0) {
                        psiteinfo->WriteToProgLog("MAIN","WARNING: unable to create a new client thread.");
                        delete pftps;
                    }
                }
            }
        }
    }

    psiteinfo->WriteToProgLog("MAIN","Server stopped on port %s",argv[1]);

        //close the listening socket
    sock.Close(sd);

        //wait for all the client connection threads to exit
    while (_nconnections > 0) timer.Sleep(100);
    psiteinfo->WriteToProgLog("MAIN","Server shutting down.");
    delete psiteinfo;

        //uninitialize the network sockets
    sock.Uninitialize();

    return(0);  //return success
}

    //standard signal handler used to catch signals such as SIGINT
static void _sighandler(int sig)
{

    #ifndef WIN32
      if (sig == SIGPIPE)   //ignore SIGPIPE
          return;
      if (sig == SIGHUP)    //ignore SIGHUP
          return;
    #endif

    _flagquit = 1;  //exit the program
}

    //Used to check if the user entered a transfer command.
    //Returns 0 if the command is not a transfer command
    //Returns 1 if download, and 2 if upload
static int _iscommandxfer(char *command)
{
    CStrUtils strutils;
    char commandbuffer[5];

        //Extract the FTP command
    strncpy(commandbuffer,command,sizeof(commandbuffer)-1);
    commandbuffer[sizeof(commandbuffer)-1] = '\0';

        //check for download commands
    if (strutils.CaseCmp(commandbuffer,"RETR") == 0)
        return(1);

        //check for upload commands
    if (strutils.CaseCmp(commandbuffer,"STOR") == 0 ||
        strutils.CaseCmp(commandbuffer,"STOU") == 0 ||
        strutils.CaseCmp(commandbuffer,"APPE") == 0)
        return(2);

    return(0);
}

    //This is the main function that handles each client.
    //This function is run in a new thread for each client connection.
#ifdef WIN32
  static void _connect(void *vpftps)    //WINDOWS must return "void" to run as a new thread
#else
  static void *_connect(void *vpftps)   //UNIX must return "void *" to run as a new thread
#endif
{
    CFtps *pftps = (CFtps *)vpftps; //FTP server class
    CTermsrv ftpscmd;       //Terminal command class (used to send and recv FTP commands)
    CSock sock;             //Sockets class (contains all the network func)
    CCmdLine cmdline;       //Stores the FTP received command line
    CStrUtils strutils;     //String utilities class
    CSiteInfo *psiteinfo;   //pointer to the site information class (used for logging)
    CTimer timer;           //used for Sleep() and other time related functions

    char **argv;        //array of pointers to the individual command line arguments
    int argc = 0;       //the number of command line arguments

    int sockstatus;     //the status of the socket (has data been received)

    int flaguser = 0;   //is the user allowed access (1 = user is allowed)
    int flagauth = 0;   //is the user authenticated  (1 = user if authenticated)

        //set the site information pointer
    psiteinfo = pftps->GetSiteInfoPtr();

    _nconnections++;    //increment the number of current client connections

        //Set the socket descriptor for the FTP command class
    ftpscmd.SetSocketDesc(pftps->GetSocketDesc());

        //Set the socket to keepalive to detect a dead socket
    if (sock.SetKeepAlive(pftps->GetSocketDesc()) == 0)
        psiteinfo->WriteToProgLog("MAIN","Failed to set socket to KeepAlive.");

        //Set the socket to receive OOB data (for ABOR)
    if (sock.SetRecvOOB(pftps->GetSocketDesc()) == 0)
        psiteinfo->WriteToProgLog("MAIN","Failed to set socket to receive OOB data.");

        //Send the Login string to the client
    pftps->EventHandler(0,NULL,_LOGINSTR,"220",1,1,0);


    while (pftps->GetFlagQuit() == 0 && _flagquit == 0) {
        if ((sockstatus = sock.CheckStatus(pftps->GetSocketDesc(),_USERLOOPFREQ)) > 0) {
                //receive the FTP command and check if the connection was broken
            if (ftpscmd.CommandRecv(cmdline.m_cmdline,cmdline.GetCmdLineSize()) == 0) {
                psiteinfo->WriteToProgLog("MAIN","Error connecting to client (%s disconnected).",pftps->GetLogin());
                pftps->DoQUIT(0,NULL);
                break;
            }
                //update the user's current information
            pftps->SetLastActive();             //update the last active time
        } else {
            strcpy(cmdline.m_cmdline,"~");      //used to indicate no activity
        }

            //parse the input from the user
        argv = cmdline.ParseCmdLine(&argc);

            //Make sure the user is logged in
            //(BasicFTPD allows any login and password)
            //If the user is not logged in only allow the commands
            //USER, PASS, or QUIT
        if (argc > 0 && flagauth == 0) {        //user is not authenticated

            switch (*(argv[0])) {

                case '~': {
                    if (sockstatus > 0) //if a command was received
                        pftps->EventHandler(argc,argv,"Please login with USER and PASS.","530",1,1,0);
                    //else Do Nothing (idle indicator)
                } break;

                case 'U': case 'u': {
                    if (strutils.CaseCmp(argv[0],"USER") == 0) {
                        flaguser = pftps->DoUSER(argc,argv);
                    } else {
                        pftps->EventHandler(argc,argv,"Please login with USER and PASS.","530",1,1,0);
                    }
                } break;

                case 'P': case 'p': {
                    if (flaguser != 0 && strutils.CaseCmp(argv[0],"PASS") == 0) {
                        if (pftps->DoPASS(argc,argv) == 0) {
                            flaguser = 0;
                            flagauth = 0;  //the user was not authenticated
                        } else {
                            flagauth = 1;  //the user was authenticated
                        }
                    } else {
                        pftps->EventHandler(argc,argv,"Please login with USER and PASS.","530",1,1,0);
                    }
                } break;

                case 'Q': case 'q': {
                    if (strutils.CaseCmp(argv[0],"QUIT") == 0) {
                        pftps->DoQUIT(argc,argv);
                    } else {
                        pftps->EventHandler(argc,argv,"Please login with USER and PASS.","530",1,1,0);
                    }
                } break;

                default: {
                    pftps->EventHandler(argc,argv,"Please login with USER and PASS.","530",1,1,0);
                } break;

            } // end switch

        } else if (argc > 0 && flagauth != 0) { //user is authenticated

            switch (*(argv[0])) {

                case '~': {
                    if (sockstatus > 0)     //if a command was received
                        pftps->ExecFTP(argc,argv);
                    //else Do Nothing (idle indicator)
                } break;

                case 'P': case 'p': {
                    if (strutils.CaseCmp(argv[0],"PASS") == 0) {
                        if (pftps->DoPASS(argc,argv) == 0) {
                            flaguser = 0;
                            flagauth = 0;  //the user was not authenticated
                        }
                    } else {
                        pftps->ExecFTP(argc,argv);
                    }
                } break;

                case 'U': case 'u': {
                    if (strutils.CaseCmp(argv[0],"USER") == 0) {
                        flagauth = 0;  //reset the authorization
                        flaguser = pftps->DoUSER(argc,argv);
                    } else {
                        pftps->ExecFTP(argc,argv);
                    }
                } break;

                default: {
                        //check if the user entered a data transfer command (Ex. RETR, STOR)
                    if (_iscommandxfer(cmdline.m_cmdline) != 0) {
                            //only allow 1 data transfer at a time
                        if (pftps->GetNumDataThreads() > 0) {
                            pftps->EventHandler(argc,argv,"Only 1 transfer per control connection is allowed.","550",1,1,0);
                            break;
                        }
                    }
                        //execute the FTP command
                    pftps->ExecFTP(argc,argv);
                } break;

            } // end switch

        } else {    //nothing was entered
            pftps->EventHandler(argc,argv,"command not understood","500",1,1,0);
        }
    }

    sock.Close(pftps->GetSocketDesc()); //close the control connection
    psiteinfo->WriteToProgLog("MAIN","Connection closed (%u), %s disconnected.",pftps->GetSocketDesc(),pftps->GetLogin());
    psiteinfo->WriteToAccessLog(pftps->GetServerIP(),pftps->GetServerPort(),pftps->GetClientIP(),pftps->GetClientPort(),pftps->GetLogin(),"DISCONNECT","","","");

        //DON'T delete the CFtps class until ALL connection threads have exited
    pftps->SetFlagAbor(1); pftps->SetFlagQuit(1); //stop all threads
    while ((pftps->GetNumListThreads()+pftps->GetNumDataThreads()) > 0) timer.Sleep(100); //wait until all threads have stopped
    delete pftps;  //delete the FTP server class

    _nconnections--;    //decrement the number of current client connections

    #ifndef WIN32
      return(NULL);   //UNIX must return a value
    #endif
}

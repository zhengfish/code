------------------------------------------------------------------------------
 IndiFTPD
------------------------------------------------------------------------------

[Overview]

IndiFTPD is a standalone (single executable) FTP server designed for 
situations when a user wants to quickly start up a server without going 
through an installation. It contains the essential FTP features as well as 
many advanced features and configuration options.  IndiFTPD is completely
self contained  and does not require any configuration files or supporting
binaries (on Windows the C runtime dll, for example msvcrt40.dll, might be
needed).  It is currently designed to support the Windows, Linux, Solaris,
and FreeBSD operating systems.  For a more complete description of IndiFTPD
visit http://www.dftpd.com/products.php.

Please report any bugs or send questions/suggestions at 
http://www.dftpd.com/feedback.php


[Building IndiFTPD]

- Windows -
IndiFTPD can be built for Windows using either "Microsoft Visual Studio .NET
2003" or "Microsoft Visual C++ 6.0".  The project files for both Microsoft
compilers are located in the indiftpd sub-directory.  When using VS .NET
indiftpd.sln should be used to open the project.  For VC++ 6.0 indiftpd.dsw
should be used.

To use SSL with IndiFTPD first download the latest OpenSSL source code for
version 0.9.6 (available at www.openssl.org).  Then build it as described in
the OpenSSL documentation.  If you already have a build of the libeay32.lib
and ssleay32.lib libraries, along with the associated header files, you can
skip building OpenSSL.  The directions below assume you are building OpenSSL
version 0.9.6l with the root directory at the same level as the "ftps" 
directory.  If the libraries and include files are located elsewhere you will
need to set the paths accordingly.

To enable SSL using "Microsoft Visual Studio .NET 2003" set the following
properties for the indiftpd project:
> "C/C++ -> General -> Additional Include Directories" add
..\..\openssl-0.9.6l\include
> "C/C++ -> Preprocessor -> Preprocessor Definitions" add
ENABLE_SSL
> "Linker -> General -> Additional Library Directories" add
..\..\openssl-0.9.6l\out32
> "Linker -> Input -> Additional Dependencies" add
Gdi32.lib libeay32.lib ssleay32.lib

To enable SSL using "Microsoft Visual C++ 6.0" set the following
settings for the indiftpd project:
> "C/C++ (tab) -> Preprocessor (Category:) -> Additional include directories:" add
..\..\openssl-0.9.6l\include
> "C/C++ (tab) -> Preprocessor (Category:) -> Preprocessor definitions:" add
ENABLE_SSL
> "Link (tab) -> Input (Category:) -> Additional library path:" add
..\..\openssl-0.9.6l\out32
> "Link (tab) -> Input (Category:) -> Object/library modules:" add
Gdi32.lib libeay32.lib ssleay32.lib


- UNIX -
For the UNIX systems (Linux, Solaris, and FreeBSD) the Makefile, also located
in the indiftpd sub-directory, should be used.  The makefile has one required
parameter and one optional parameter.  The parameters are passed to the
Makefile via environment variables.  The required variable, called "platform",
must be set to either linux, solaris, or freebsd.  The optional parameter,
called "opensslroot", must contain the path to the root of the OpenSSL
directory tree.  Below are two examples of how to use the make command:

Example1: make platform=linux
Example2: make platform=solaris opensslroot=../../openssl-0.9.6l

The library options used in the Makefile may create dependencies on certain
shared objects (Ex. libstdc++.so.5).  To avoid these dependencies the static
libraries should be included instead.  For example, the option "-lstdc++"
may be replaced with "/usr/lib/gcc-lib/i386-redhat-linux/3.2.2/libstdc++.a"
to remove the dependency on the libstdc++.so.5 shared object (library).  The
path to the libstdc++.a library will vary on different versions of UNIX.  
Also, using the statically linked libraries will increase the size of the 
executable.

To use SSL download the latest OpenSSL source code for version 0.9.6 
(available at www.openssl.org).  Then build the code as described in the
OpenSSL documentation.  Next set the "opensslroot" parameter for the make
command to point to the root of the OpenSSL directory tree.  If the
"opensslroot" parameter is omitted IndiFTPD will be built without SSL support.


[About the code]

The IndiFTPD code is partitioned into two primary pieces: the server core, and
the specific server implementation.  The core contains all the code for a 
basic FTP server.  The only file that must be added to make the core a 
functional server is an implementation of the main() function.  For an example
of a minimal implementation of the server core see basicmain.cpp in the 
"basicftpd" subdirectory.  To build BasicFTPD use one of the project files
(in Windows) or the Makefile (in UNIX) -- the build process for BasicFTPD is
similar to IndiFTPD.  IndiFTPD is also designed to be easily extended by
simply adding new methods to override the default FTP command implementations.
To add new functionality you can simply add methods to the CIndiFtps class.

The 2 classes in the core that will most often be overridden are CFtps
(in Ftps.cpp/Ftps.h) and CSiteInfo (in SiteInfo.cpp/SiteInfo.h).  The CFtps
class contains the handlers for all the FTP commands (Ex. LIST, PASV,
CWD, etc).  It also stores all the state for a client connection.  An
instance of CFtps is created for each client connection.  For an example of
how to implement new FTP commands see the CInfiFtps class (in IndiFtps.cpp/
IndiFtps.h).  The CSiteInfo class stores all the information for the FTP
server.  Only 1 instance of the CSiteInfo class should be created in the 
main() function and passed in to all threads.  As a general convention all
thread synchronization is contained in the CSiteInfo class.  When shared
information is needed by a thread, a new copy of the information is returned
and never a pointer to the actual shared information.  For an example of
overriding the CSiteInfo class see the CIndiSiteInfo class (in 
IndiSiteInfo.cpp/IndiSiteInfo.h).

Below is a short description of each core class:

CBlowfishCrypt [BlowfishCrypt.cpp/BlowfishCrypt.h]:
Implementation of Bruce Schneier's BLOWFISH algorithm from "Applied 
Cryptography", Second Edition.

CCmdLine [CmdLine.cpp/CmdLine.h]:
Used to store and manipulate a command line input.  This class is used to
parse a command line into its individual arguments or recombine an array
of arguments into a single string.  It can also be used to parse lines in a
configuration file.  The parse method in CCmdLine can parse a string based 
on any delimiting character.

CCrypto [Crypto.cpp/Crypto.h]:
Provides encryption and decryption using the Blowfish algorithm.  This class
contains methods to encrypt a buffer into a HEX string or decrypt an encrypted
HEX string -- used for passwords.

CDll [Dll.cpp/Dll.h]:
Implements a doubly-linked list.  This is used to store information such as
the list of users on the site.

CFSUtils [FSUtils.cpp/FSUtils.h]:
Provides utility functions used for file system access.  This class contains
methods for directory listing, building and validating paths, getting 
information about files or directories, deleting and renaming files, 
getting/setting the current working directory, etc.

CFtps [Ftps.cpp/Ftps.h]:
Primary class for the FTP server.  This class contains all the functions used
to handle the FTP commands (Ex. LIST, CWD) as well as storing the current
state for each client connection (logged in users).

CFtpsXfer [FtpsXfer.cpp/FtpsXfer.h]:
This class contains the methods for transferring data.  This class is called
from the global function ftpsXferThread() that is used to start new threads
for FTP data connections.  This class is used for data transfer commands
such as LIST, RETR, and STOR.

CLog [Log.cpp/Log.h]:
Basic class for writing to a log file.

CService [Service.cpp/Service.h]:
This class contains methods to setup an application to run as a service
-- not used in IndiFTPD.  Currently this is mostly useful for Windows.

CSiteInfo [SiteInfo.cpp/SiteInfo.h]:
This class is used to store and access information about the server such as
configuration information or getting a list of users for the site.  Only
1 instance of this class should be created and passed to all the client
connection threads.  Any information that is shared among the client threads
should be accessed through this class.

CSock [Sock.cpp/Sock.h]:
This class is used to abstract TCP/IP sockets.  It contains methods for 
opening client and server connections, getting the IP address and port of
client or server connections, sending and receiving data over the network,
setting the options for the connection, etc.

CSSLSock [SSLSock.cpp/SSLSock.h]:
Implements SSL functionality using the OpenSSL libraries.  This class contains
methods for generating X509 certificates, public and private keys, negotiating
SSL client and server connections, sending and receiving data over SSL, etc.

CStrUtils [StrUtils.cpp/StrUtils.h]:
Contains string handling methods such as string comparisons, changing the case
of a string, matching wildcards, reading a string from the terminal without
displaying the typed input, building strings using format options, etc.

CTermcli [Termcli.cpp/Termcli.h]:
Used to send and receive terminal commands as a client.  For example, when
sending and receiving commands on the FTP control connection as a client.

CTermsrv [Termsrv.cpp/Termsrv.h]:
Used to send and receive terminal commands as a server.  For example, when
sending and receiving commands on the FTP control connection as a server.

CThr [Thr.cpp/Thr.h]:
Provides threading support.  This class contains methods to create and destroy
threads along with synchronization functions.

CTimer [Timer.cpp/Timer.h]:
Provides millisecond accurate timing functions.  This class is used to time 
the duration of events such as data transfers as well as regulate transfer
rates.  It also contains the sleep method used to block a thread for a 
specified amount of time.

/**
 * @file mongooserver.c
 * @author closear@gmail.com
 * @date 2016-12-13 15:15:06 Tue/50
 *
 * @brief
 *  a simple web server based mongoose-6.6 for static file sharing or serving on Windows and Linux
 *
 * @see https://github.com/yorkie/serve
 * @see https://github.com/cesanta/mongoose
 *
 * @note
 *  Build on Windows with Visual Studio
 *  cl -MT mongooserver.c mongoose.c
 */

#include "mongoose.h"

static const char *s_http_port = "8888";
static int s_sig_num = 0;
static struct mg_serve_http_opts s_http_server_opts;

static void signal_handler ( int sig_num )
{
    signal ( sig_num, signal_handler );
    s_sig_num = sig_num;
}

static void ev_handler ( struct mg_connection *nc, int ev, void *ev_data )
{
    struct http_message *hm = ( struct http_message * ) ev_data;
    switch ( ev ) {
    case MG_EV_HTTP_REQUEST:
        mg_serve_http ( nc, hm, s_http_server_opts );
        break;
    default:
        break;
    }
}

int main ( int argc, char const *argv[] )
{
    struct mg_mgr mgr;
    struct mg_connection *nc = 0;
    int i = 0;

    /* set the default documentation root */
    s_http_server_opts.document_root = "./public";

    /* Parse command line arguments */
    for ( i = 1; i < argc; i++ ) {
        if ( strcmp ( argv[i], "-r" ) == 0 ) {
            s_http_server_opts.document_root = argv[++i];
        } else if ( strcmp ( argv[i], "-p" ) == 0 || strcmp ( argv[i], "--port" ) == 0 ) {
            s_http_port = argv[++i];
        } else if ( strcmp ( argv[i], "-h" ) == 0 || strcmp ( argv[i], "--help" ) == 0 ) {
            printf ( "\n  %s -r [dir] -p [port]\n\n", argv[0] );
            return 0;
        }
    }


    /* detect the document root directory */
    const char* folderr = s_http_server_opts.document_root;
    struct stat sb = { 0 };

    if ( ! ( stat ( folderr, &sb ) == 0 && S_ISDIR ( sb.st_mode ) ) ) {
        fprintf ( stderr, "'%s' is not existed, bye.\n", folderr );
        return EXIT_FAILURE;
    }


    /* Start listening */
    mg_mgr_init ( &mgr, NULL );
    nc = mg_bind ( &mgr, s_http_port, ev_handler );
    if ( nc == NULL ) {
        fprintf ( stderr, "mg_bind(%s) failed\n", s_http_port );
        mg_mgr_free ( &mgr );
        return EXIT_FAILURE;
    }
    mg_set_protocol_http_websocket ( nc );

    /* Handle with signals */
    signal ( SIGINT, signal_handler );
    signal ( SIGTERM, signal_handler );

    /* Run event loop until signal is received */
    printf ( "Starting file server on port %s at directory '%s'\n",
             s_http_port,
             s_http_server_opts.document_root );
    while ( s_sig_num == 0 ) {
        mg_mgr_poll ( &mgr, 1000 );
    }

    /* Cleanup */
    mg_mgr_free ( &mgr );
    printf ( "Exiting on signal %d\n", s_sig_num );

    return 0;
}


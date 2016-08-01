#include "condor_common.h"
#include "condor_debug.h"
#include "condor_config.h"
#include "condor_daemon_core.h"
#include "subsystem_info.h"

#include "gahp-client.h"

// Required by GahpServer::Startup().
char * GridmanagerScratchDir = NULL;

void
doPolling() {
	dprintf( D_ALWAYS, "doPolling()\n" );

	// Create an EC2 GAHP. [FIXME: in some other function, since we only need 1 for now)
	std::string gahpName;
	formatstr( gahpName, "annex-%s@%s", "publicKeyFile.c_str()", "m_serviceURL.c_str()" );

	ArgList args;

	// FIXME: I think the EC2 GAHP ignores the -f flag, and it certainly
	// won't otherwise grab the rigth set of parameters.  Maybe call it
	// with the -localname flag for now?
	char * gahp_log = param( "ANNEX_GAHP_LOG" );
	if( gahp_log == NULL ) {
		dprintf( D_ALWAYS, "Warning: ANNEX_GAHP_LOG not defined.\n" );
	} else {
		args.AppendArg( "-f" );
		args.AppendArg( gahp_log );
		free( gahp_log );
	}

	args.AppendArg( "-w" );
	int minWorkerCount = param_integer( "ANNEX_GAHP_WORKER_MIN_NUM", 1 );
	args.AppendArg( minWorkerCount );

	args.AppendArg( "-m" );
	int maxWorkerCount = param_integer( "ANNEX_GAHP_WORKER_MAX_NUM", 1 );
	args.AppendArg( maxWorkerCount );

	args.AppendArg( "-d" );
	char * gahp_debug = param( "ANNEX_GAHP_DEBUG" );
	if( gahp_debug == NULL ) {
		args.AppendArg( "D_ALWAYS" );
	} else {
		args.AppendArg( gahp_debug );
		free( gahp_debug );
	}

	char * gahp_path = param( "ANNEX_GAHP" );
	if( gahp_path == NULL ) {
		EXCEPT( "ANNEX_GAHP must be defined." );
	}

	GahpClient * gahp = new GahpClient( gahpName.c_str(), gahp_path, & args );
	free( gahp_path );

	// gahp->setNotificationTimerId( );
	gahp->setMode( GahpClient::normal );

	int gct = param_integer( "ANNEX_GAHP_CALL_TIMEOUT", 10 * 60 );
	gahp->setTimeout( gct );

	if( gahp->Startup() == false ) {
		EXCEPT( "Failed to start GAHP." );
	}

	// HAX!
	std::string m_serviceURL( "http://ec2.amazonaws.com" );
	std::string m_public_key_file( "/home/tlmiller/condor/test/ec2/accessKeyFile" );
	std::string m_private_key_file( "/home/tlmiller/condor/test/ec2/secretKeyFile" );
	std::string m_remoteJobID( "i-12345678" );
	std::string gahpErrorCode;
	gahp->ec2_vm_stop(	m_serviceURL, m_public_key_file, m_private_key_file,
						m_remoteJobID, gahpErrorCode );
}

void
main_init( int /* argc */, char ** /* argv */ ) {
	dprintf( D_ALWAYS, "main_init()\n" );

	// For use by the command-line tool.
	// daemonCore->RegisterCommand... ( ... )

	// Load our hard state and register timer(s) as appropriate.
	// ...

	// For now, just poll AWS regularly.
	// Units appear are seconds.
	unsigned delay = 0;
	unsigned period = param_integer( "ANNEX_POLL_INTERVAL", 300 );
	daemonCore->Register_Timer( delay, period, & doPolling, "poll the cloud" );
}

void
main_config() {
	dprintf( D_ALWAYS, "main_config()\n" );
}

void
main_shutdown_fast() {
	dprintf( D_ALWAYS, "main_shutdown_fast()\n" );
	DC_Exit( 0 );
}

void
main_shutdown_graceful() {
	dprintf( D_ALWAYS, "main_shutdown_graceful()\n" );
	DC_Exit( 0 );
}

void
main_pre_dc_init( int /* argc */, char ** /* argv */ ) {
	dprintf( D_ALWAYS, "main_pre_dc_init()\n" );
}

void
main_pre_command_sock_init() {
	dprintf( D_ALWAYS, "main_pre_command_sock_init()\n" );
}

int
main( int argc, char ** argv ) {
	set_mySubSystem( "ANNEXD", SUBSYSTEM_TYPE_DAEMON );

	dc_main_init = & main_init;
	dc_main_config = & main_config;
	dc_main_shutdown_fast = & main_shutdown_fast;
	dc_main_shutdown_graceful = & main_shutdown_graceful;
	dc_main_pre_dc_init = & main_pre_dc_init;
	dc_main_pre_command_sock_init = & main_pre_command_sock_init;

	return dc_main( argc, argv );
}
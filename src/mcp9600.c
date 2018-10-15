// Get temperature(s) from MCP9600 device(s)
//


#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <argp.h>

#define mcp9600_VERSION_MAJOR 0
#define mcp9600_VERSION_MINOR 0

// CLI Arguments
// argp
const char *argp_program_version = "fade9685";
const char *argp_program_bug_address = "pjvint@gmail.com";

static char doc[] = "fade9685 - simple CLI application to control PWM with a PCA9685 device on I2C";
/* A description of the arguments we accept. */
static char args_doc[] = "ARG1 [STRING...]";


static struct argp_option options[] = {
	{ "reset", 'R', 0, 0, "Reset PCA9685" },
	{ "bus", 'b', "BUS", 0, "Bus number" },
	{ "address", 'a', "ADDRESS", 0, "Address (ie 0x40)" },
	{ "verbose", 'v', "VERBOSITY", 0, "Verbose output" },
	{ "help", 'h', 0, 0, "Show help" },
	{ 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
	char *args[2];                /* arg1 & arg2 */
	unsigned int frequency, channels, bus, address, verbose, reset, rec709;
	int step;
	float dutycycle, luminosity;
};

/* Parse a single option. */
static error_t parse_opt ( int key, char *arg, struct argp_state *state )
{
	/* Get the input argument from argp_parse, which we
	know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;

	switch ( key )
	{
		case 'b':
			arguments->bus = atoi( arg );
			break;
		case 'a':
			arguments->address = strtoul( arg, NULL, 16 );
			break;
		case 'v':
			arguments->verbose = strtoul( arg, NULL, 10 );
			break;
		case 'R':
			arguments->reset = 1;
			break;
		case 'h':
			//print_usage( "mcp9600" );
			exit( 0 );
			break;
		case ARGP_KEY_ARG:
			if ( state->arg_num >= 2 )
			{
				argp_usage( state );
			}
			arguments->args[ state->arg_num ] = arg;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

// TODO: Add syslog ability
void printLog( char *msg, unsigned int verbose, unsigned int level )
{
	if ( level > verbose )
		return;

	fprintf( stderr, "%d: %s\n", level, msg );
	return;
}

float readTemp( int file, unsigned int address )
{
	ioctl( file, I2C_SLAVE, address );

	char cfg[2];

	cfg[0] = 0x05;
	cfg[1] = 0x00;

	write( file, cfg, 2 );

	cfg[0] = 0x06;
	cfg[1] = 0x00;

	// check status flag
	while( 1 )
	{
		char reg1[1] = {0x04};
		write(file, reg1, 1);

		char data1[1];
		if(read(file, data1, 1) != 1)
		{
			return 0;
		}	
		else
		{
			if ( data1[0] & 0x40 )
			{
				// clear status flag
				cfg[0] = 0x04;
				data1[0] &= ~(1<<6);
				cfg[1] = data1[0];
				write( file, cfg, 2 );
				break;
			}
		}

		char reg[1] = {0x00};
		write( file, reg, 1 );
		char data[2] = {0};

		if ( read( file, data, 2 ) != 2 )
		{
			printLog( "Error!", 1, 1 );
		}
		else
		{
			int lowTemp = data[0] & 0x80;

			float ret;

			if ( lowTemp )
			{
				ret = data[0] * 16 + data[1] / 16 - 4096;
				return ret;
			}
			else
			{
				ret = data[0] * 16 + data[1] * 0.0625;
				return ret;
			}
		}	
	}
}

int main( int argc, char **argv )
{
	struct arguments arguments;

	/* Default values. */
	arguments.bus = 1;
	arguments.address = 0x40;
	arguments.verbose = 0;
	arguments.reset = 0;

	/* Parse our arguments; every option seen by parse_opt will
	be reflected in arguments. */
	argp_parse ( &argp, argc, argv, 0, 0, &arguments );

}

int initHardware( unsigned int adpt, unsigned int addr )
{
	int file;
	char bus[11];

	sprintf( bus, "/dev/i2c-%01d", adpt );

	if ( ( file = open( bus, O_RDWR ) ) < 0 )
	{
		char msg[256];
		snprintf( msg, 256, "Error opening %s\n", bus );
		printLog( msg, 1, 1 );
		exit( 1 );
	}
}

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
const char *argp_program_version = "mcp9600_pi";
const char *argp_program_bug_address = "pjvint@gmail.com";

static char doc[] = "mcp9600 - simple CLI application to read thermocouple temperature with an MCP9600 device";
static char args_doc[] = "ARG1 [STRING...]";


static struct argp_option options[] = {
	{ "bus", 'b', "BUS", 0, "Bus number" },
	{ "address", 'a', "ADDRESS", 0, "Address (ie 0x40)" },
	{ "ambient", 'A', 0, 0, "Read cold junction temperature" },
	{ "Resolution", 'r', "RESOLUTION", 0, "ADC Resolution. 0-3, 0=Max (18bit) 3 = min (12 bit)" },
	{ "thermocouple", 't', "THERMOCOUPLE", 0, "Thermocouple type" },
	{ "filter", 'f', "FILTER", 0, "Filter coeffocient" },
	{ "delay", 'd', "DELAY", 0, "Loop delay (ms) (if not set display once and exit)" },
	{ "quiet", 'q', 0, 0, "Suppress normal output, return temperature" },
	{ "verbose", 'v', "VERBOSITY", 0, "Verbose output" },
	{ "help", 'h', 0, 0, "Show help" },
	{ 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
	char *args[2];                /* arg1 & arg2 */
	unsigned int bus, address, verbose, resolution, filter, ambient, delay, quiet;
	char *thermocouple;
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
		case 'r':
			arguments->resolution = atoi( arg );
			break;
		case 't':
			arguments->thermocouple = arg;
			break;
		case 'd':
			arguments->delay = atoi( arg ) * 1000;
			break;
		case 'f':
			arguments->filter = atoi( arg );
			break;
		case 'A':
			arguments->ambient = 1;
			break;
		case 'q':
			arguments->quiet = 1;
			break;
		case 'h':
			//print_usage( "mcp9600" );
			printf("Try --usage\n");
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

int sensorConfig(  unsigned int bus, unsigned int address, unsigned char thermocoupleType, unsigned char filterCoefficient, unsigned char config )
{
	int file;
	
	// Set the type: K, J, T, N, S, E, B, R 
	// Set the filter coefficient - 0 (off) to 7 (max)
	unsigned char type;
	switch( thermocoupleType )
	{
		case 'K':
			type = 0x00;
			break;

		case 'J':
			type = 0x01;
			break;

		case 'T':
			type = 0x02;
			break;

		case 'N':
			type = 0x03;
			break;

		case 'S':
			type = 0x04;
			break;

		case 'E':
			type = 0x05;
			break;

		case 'B':
			type = 0x06;
			break;

		case 'R':
			type = 0x07;
			break;

		default:
			printLog( "Bad thermocouple type.", 1, 1 );
			exit( 1 );
	}

	// Shift type left 4 bits
	type = type << 4;
	// OR in the filter
	type = type | filterCoefficient;

	file = initHardware ( bus, address );

	ioctl( file, I2C_SLAVE, address );

	// Test getting device ID
	//char r[1] = {0x20};
	//write( file, r, 1 );

	char cfg[2];
	//read( file, cfg, 2 );
	//printf("\nDeviceID: %02x %02x\n", cfg[0], cfg[1] );

	
	
	cfg[0] = 0x05;	// sensor config register
	cfg[1] = type;

	write( file, cfg, 2 );

	// set the device config (ADC resolution etc)
	cfg[0] = 0x06;
	cfg[1] = config;

	write( file, cfg, 2 );

	return file;	

}

float readTemp( int file, unsigned int address )
{
	ioctl( file, I2C_SLAVE, address );

	char cfg[2];
	char reg1[1];

	// Test getting ID
	//char r[1] = {0x20};
	//write( file, r, 1 );
	//read( file, cfg, 2 );
	//printf("\nDeviceID: %02x %02x\n", cfg[0], cfg[1] );

	//cfg[0] = 0x05;
	//cfg[1] = 0x00;

	//write( file, cfg, 2 );

	//cfg[0] = 0x06;
	//cfg[1] = 0x00;

	//char reg1[1] = {0x04};
	//write(file, reg1, 1);

	// Wait for status bit to be high
	reg1[0] = 0x04;
	write( file, reg1, 1 );

	while ( (read( file, reg1, 1 ) && ( 1 << 6 )) == 0 )
	{
		usleep( 100000 );	// wait 100 ms
	}

	// clear the status bit (also sets "burst complete" bit to zero, but not important)
	write( file, reg1, 0 );

	reg1[0] = 0;
	write(file, reg1, 1);
	char data[2] = {0};
	read( file, data, 2 );

	int lowTemp = data[0] & 0x80;
	float ret;
	if ( lowTemp )
	{
		ret = data[0] * 16 + data[1] / 16 - 4096;
	}
	else
	{
		ret = data[0] * 16 + data[1] * 0.0625;
	}
	//printf("%f\n", ret);
	return ret;
}

float readAmbientTemp( int file, unsigned int address )
{
	ioctl( file, I2C_SLAVE, address );

	char cfg[2];

	char reg1[1] = {0x02};
	write(file, reg1, 1);
	char data[2] = {0};
	read( file, data, 2 );

	int lowTemp = data[0] & 0x80;
	float ret;
	if ( lowTemp )
	{
		ret = data[0] * 16 + data[1] / 16 - 4096;
	}
	else
	{
		ret = data[0] * 16 + data[1] * 0.0625;
	}
	//printf("%f\n", ret);
	return ret;
}

int main( int argc, char **argv )
{
	struct arguments arguments;

	/* Default values. */
	arguments.bus = 1;
	arguments.address = 0x60;
	arguments.verbose = 0;
	arguments.resolution = 0;
	arguments.thermocouple = 'K';
	arguments.delay = 0;
	arguments.filter = 0;
	arguments.ambient = 0;
	arguments.quiet = 0;

	/* Parse our arguments; every option seen by parse_opt will
	be reflected in arguments. */
	argp_parse ( &argp, argc, argv, 0, 0, &arguments );

	unsigned char config = arguments.resolution;

	int file = sensorConfig( arguments.bus, arguments.address, arguments.thermocouple, arguments.filter, config );

	while ( 1 )
	{
		if ( arguments.ambient == 1 )
		{
			float temp = readAmbientTemp( file, arguments.address );
			printf( "%.2f ", temp );
		}

		float temp = readTemp( file, arguments.address );

		if ( arguments.quiet != 0 )
		{
			return (int) temp;
		}

		printf("%.2f\n", temp);

		if ( arguments.delay == 0 )
		{
			break;
		}

		usleep ( arguments.delay );
	}

	return 0;
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

	//ioctl( file, I2C_SLAVE, addr );

	return file;
}

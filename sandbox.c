#include "argparser.h"

int main( int argc, char** argv ) {
    argparser_t parser = argparser_init( argv[0], "-c <count>" );

    argparser_add( &parser,
        "count",
        "Number of items to process",
        true,         // required
        1,            // one argument expected
        ARG_TYPE_U64,
        "--count",
        "-c"          // alias
    );

    argparser_add( &parser,
        "name",
        "Name of the user",
        false,        // optional
        1,            // one argument expected
        ARG_TYPE_STRING,
        "--name",
        "-n"          // alias
    );

    argparser_add( &parser,
        "verbose",
        "Enable verbose output",
        false,        // optional
        0,            // no argument (flag)
        ARG_TYPE_NONE,
        "--verbose",
        "-v"          // alias
    );

    if ( argparser_parse( &parser, argc, argv ) != 0 ) {
        argparser_deinit( &parser );
        return EXIT_FAILURE;
    }

    printf( "Count: %llu\n", parser.args[1].values[0].u64 );

    if ( parser.args[1].found ) {
        printf( "Name: %s\n", parser.args[2].values[0].str );
    } else {
        printf( "Name: (not provided)\n" );
    }

    if ( parser.args[3].found ) {
        printf( "Verbose mode is ON\n" );
    } else {
        printf( "Verbose mode is OFF\n" );
    }

    argparser_deinit( &parser );
    return 0;
}

#include "argparser.h"
int main( int argc, char** argv ) {

    // initialise your argument parser object. argv[0] is just your program name, but you can actually pass in whatever you want here.
    argparser_t parser = argparser_init( argv[0], "your sample usage string", "another sample usage if necessary" );

    // call this function as many times as you need to
    argparser_add( &parser,
        "identifier",       // you'll use this field to access variables tied to this argument later,
        // but not to actually use the flag when you're running your program

        "description",      // a brief description of this variable - what it does

        true,               // whether the argument is required or not
        // if it's required and isn't found, the program will gracefully exit

        4,                  // how many consecutive arguments this flag consumes. helpful for lists

        ARG_TYPE_STRING,    // the argument type. refer to the header file for more

        "--alias1",         // here you must define at least one way to actually use your defined flag
        "--alias2",         // it's a variadic macro so you can realistically do as many as you want
        "-a"
    );

    argparser_add( &parser,
        "count",
        "Number of items to process",
        false,
        1,
        ARG_TYPE_U64,
        "--count",
        "-c"
    );

    argparser_add( &parser,
        "verbose",
        "Enable verbose output",
        false,
        0,
        ARG_TYPE_NONE,      // this is the only type that CAN and MUST have 0 as the argument count
        "--verbose",
        "-v"
    );

    // parse the arguments
    if ( argparser_parse( &parser, argc, argv ) != 0 ) {
        argparser_deinit( &parser );
        return EXIT_FAILURE;
    }

    // to see if a flag was found call this awesome helper function
    if ( argparser_found( &parser, "count" ) ) {
        // to get the values of a given argument, use the handy getter functions
        // if your flag only has 1 as it's arg_count, you'll only be using 0,
        // but for ones with multiple you can use the respective indexes you're after
        // giving an invalid index will gracefully exit your program
        // but keep in mind using the wrong type will only give a warning, idk there might be some intention behind it
        printf( "Count: %llu\n", argparser_get_u64( &parser, "count", 0 ) );
    }

    // using the rgparser_get_none function will just return whether it was found or not
    // it's there for things like the above "verbose" flag, where you don't actually need to pass a value,
    // but you'll still want to know if it's there or not
    printf( "%s\n", argparser_get_none( &parser, "verbose" ) == argparser_found( &parser, "verbose" ) ? "true" : "false" ); // this will return true

    // for the rest of your (positional) arguments you can just use the new argc and argv within the argparser_t struct
    for ( int i = 0; i < parser.argc; i++ ) {
        printf( "%s, ", parser.argv[i] );
    }

    // cleanup
    argparser_deinit( &parser );

    // the rest of your code
    return 0;
}

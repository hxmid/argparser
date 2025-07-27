#pragma once

#ifndef ARGPARSER_H
#define ARGPARSER_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define ARG_TYPE_LIST \
    X(NONE,     "none") \
    X(U64,      "uint64_t") \
    X(I64,      "int64_t") \
    X(F64,      "double") \
    X(U32,      "uint32_t") \
    X(I32,      "int32_t") \
    X(F32,      "float") \
    X(U16,      "uint16_t") \
    X(I16,      "int16_t") \
    X(U8,       "uint8_t") \
    X(I8,       "int8_t") \
    X(BOOL,     "bool") \
    X(STRING,   "const char *")

    typedef enum {
    #define X(name, str) ARG_TYPE_##name,
        ARG_TYPE_LIST
    #undef X
        ARG_TYPE_COUNT
    } arg_type;

    static const char* arg_type_str[] = {
    #define X(name, str) str,
        ARG_TYPE_LIST
    #undef X
    };

    static_assert(
        sizeof( arg_type_str ) / sizeof( arg_type_str[0] ) == ARG_TYPE_COUNT,
        "arg_type_str array length mismatch"
        );

    typedef union {
        uint64_t    u64;
        int64_t     i64;
        double      f64;
        uint32_t    u32;
        int32_t     i32;
        float       f32;
        uint16_t    u16;
        int16_t     i16;
        uint8_t     u8;
        int8_t      i8;
        bool        b;
        char* str;
    } arg_value;

    typedef struct {
        struct {
            char* identifier;
            char* description;
            char** aliases;
            size_t aliases_len;
            bool required;
            arg_type type;
        } meta;

        bool found;
        arg_value* values;
        size_t values_len;
    } arg_t;


    void arg_deinit( arg_t* arg ) {
        if ( arg->meta.identifier ) {
            free( arg->meta.identifier );
            arg->meta.identifier = NULL;
        }

        if ( arg->meta.aliases ) {
            for ( size_t i = 0; i < arg->meta.aliases_len; i++ ) {
                if ( arg->meta.aliases[i] ) {
                    free( arg->meta.aliases[i] );
                    arg->meta.aliases[i] = NULL;
                }
            }
            arg->meta.aliases_len = 0;
            free( arg->meta.aliases );
            arg->meta.aliases = NULL;
        }

        arg->meta.required = false;
        arg->found = false;

        if ( arg->meta.type == ARG_TYPE_STRING ) {
            for ( size_t i = 0; i < arg->values_len; i++ ) {
                free( arg->values[i].str );
            }
        }

        if ( arg->values ) {
            arg->values_len = 0;
            free( arg->values );
            arg->values = NULL;
        }
        arg->meta.type = ARG_TYPE_NONE;

        if ( arg->meta.description ) {
            free( arg->meta.description );
            arg->meta.description = NULL;
        }
    }

    typedef struct usage_linked_list_t {
        struct usage_linked_list_t* next;
        char* usage;
    } *usage_node_t;

    usage_node_t usage_node_create( char* usage ) {
        usage_node_t node = (usage_node_t)calloc( 1, sizeof( struct usage_linked_list_t ) );
        if ( !node ) {
            fprintf( stderr, "[FATAL]: could not allocate memory for usage_linked_list_t\n" );
            exit( EXIT_FAILURE );
        }

        node->usage = strdup( usage );
        if ( !node->usage ) {
            fprintf( stderr, "[FATAL]: could not allocate memory for usage_linked_list_t.usage\n" );
            exit( EXIT_FAILURE );
        }

        node->next = NULL;
        return node;
    }

    void usage_node_free( usage_node_t node ) {
        if ( node->usage ) {
            free( node->usage );
            node->usage = NULL;
        }

        node->next = NULL;

        free( node );
    }

    typedef struct {
        char* program_name;

        arg_t* args;
        size_t args_capacity;
        size_t args_length;

        usage_node_t usage;

        int argc;

        char** argv;
        size_t argv_length;
        size_t argv_capacity;
    } argparser_t;

    void argparser_deinit( argparser_t* argparser ) {
        if ( argparser->args ) {
            for ( size_t i = 0; i < argparser->args_length; i++ ) {
                arg_deinit( &argparser->args[i] );
            }

            free( argparser->args );
            argparser->args = NULL;
        }

        argparser->args_capacity = 0;
        argparser->args_length = 0;

        if ( argparser->program_name ) {
            free( argparser->program_name );
            argparser->program_name = NULL;
        }

        while ( argparser->usage ) {
            usage_node_t temp = argparser->usage;
            argparser->usage = argparser->usage->next;
            usage_node_free( temp );
        }

        if ( argparser->argv ) {
            for ( size_t i = 0; i < argparser->argv_length; i++ ) {
                free( argparser->argv[i] );
            }
            free( argparser->argv );
            argparser->argv = NULL;
        }

        argparser->argv_length = 0;
        argparser->argv_capacity = 0;

        argparser->argc = 0;
    }

    void argparser_add_inner(
        argparser_t* argparser,
        const char* identifier,
        const char* description,
        bool required,
        size_t arg_count,
        arg_type type,
        const char* aliases,
        ...
    ) {

        if ( arg_count == 0 && type != ARG_TYPE_NONE ) {
            fprintf( stderr, "[FATAL]: `arg_count` must be >= 1 when `type != ARG_TYPE_NONE`\n" );
            exit( EXIT_FAILURE );
        } else if ( arg_count != 0 && type == ARG_TYPE_NONE ) {
            fprintf( stderr, "[FATAL]: `arg_count` must be == 0 when `type == ARG_TYPE_NONE`\n" );
            exit( EXIT_FAILURE );
        }

        if ( !argparser->args_capacity ) {
            argparser->args_capacity = 1;
            argparser->args = (arg_t*)calloc( 1, sizeof( arg_t ) );
            if ( !argparser->args ) {
                fprintf( stderr, "[FATAL]: could not allocate memory for first arg_t in `argparser_t.args`\n" );
                exit( EXIT_FAILURE );
            }
        } else if ( argparser->args_length == argparser->args_capacity ) {
            argparser->args_capacity <<= 1;
            arg_t* reallocation = (arg_t*)realloc( argparser->args, argparser->args_capacity * sizeof( arg_t ) );
            if ( !reallocation ) {
                fprintf( stderr, "[FATAL]: could not reallocate memory for `argparser_t.args`\n" );
                exit( EXIT_FAILURE );
            }
            argparser->args = reallocation;
        }

        arg_t arg;

        arg.meta.identifier = strdup( identifier );
        if ( !arg.meta.identifier ) {
            fprintf( stderr, "[FATAL]: could not allocate memory for `arg_t.meta.identifier`\n" );
            exit( EXIT_FAILURE );
        }

        arg.meta.aliases = NULL;
        arg.meta.aliases_len = 0;
        arg.meta.required = required;
        arg.meta.type = type;
        arg.values_len = arg_count;
        arg.found = false;

        if ( arg.values_len > 0 ) {
            arg.values = (arg_value*)calloc( arg.values_len, sizeof( arg_value ) );
            if ( !arg.values ) {
                fprintf( stderr, "[FATAL]: could not allocate memory for `arg.values`\n" );
                exit( EXIT_FAILURE );
            }
        } else {
            arg.values = NULL;
        }

        arg.meta.description = strdup( description );
        if ( !arg.meta.description ) {
            fprintf( stderr, "[FATAL]: could not allocate memory for `arg_t.meta.description`\n" );
            exit( EXIT_FAILURE );
        }

        va_list parameters;
        va_start( parameters, aliases );
        while ( aliases != NULL ) {
            char** allocation = NULL;
            if ( arg.meta.aliases == NULL ) {
                allocation = (char**)calloc( arg.meta.aliases_len + 1, sizeof( char* ) );
            } else {
                allocation = (char**)realloc( arg.meta.aliases, ( arg.meta.aliases_len + 1 ) * sizeof( char* ) );
            }

            if ( !allocation ) {
                fprintf( stderr, "[FATAL]: could not allocate memory for `arg_t.meta.aliases`\n" );
                exit( EXIT_FAILURE );
            }

            arg.meta.aliases = allocation;
            arg.meta.aliases[arg.meta.aliases_len] = strdup( aliases );
            if ( !arg.meta.aliases[arg.meta.aliases_len] ) {
                fprintf( stderr, "[FATAL]: could not allocate memory for `arg_t.meta.aliases[%u]`\n", arg.meta.aliases_len );
                exit( EXIT_FAILURE );
            }
            arg.meta.aliases_len += 1;
            aliases = va_arg( parameters, const char* );
        }
        va_end( parameters );
        if ( !arg.meta.aliases_len ) {
            fprintf( stderr, "[FATAL]: need at least one alias for command line argument\n" );
            exit( EXIT_FAILURE );
        }

        argparser->args[argparser->args_length] = arg;
        argparser->args_length += 1;
    }

#define argparser_add(argparser, identifier, description, required, arg_count, type, ...) \
        argparser_add_inner(argparser, identifier, description, required, arg_count, type, __VA_ARGS__, NULL)

    argparser_t argparser_init_inner( char* program_name, char* usages, ... ) {
        argparser_t argparser;
        argparser.program_name = strdup( program_name );
        if ( !argparser.program_name ) {
            fprintf( stderr, "[FATAL]: could not allocate memory for `argparser_t.program_name`\n" );
            exit( EXIT_FAILURE );
        }

        argparser.args = NULL;
        argparser.args_capacity = 0;
        argparser.args_length = 0;
        argparser.usage = NULL;

        va_list parameters;
        va_start( parameters, usages );
        while ( usages ) {
            usage_node_t node = usage_node_create( usages );
            node->next = argparser.usage;
            argparser.usage = node;
            usages = va_arg( parameters, char* );
        }

        argparser.argc = 0;

        argparser.argv = NULL;
        argparser.argv_length = 0;
        argparser.argv_capacity = 0;

        argparser_add_inner( &argparser, "help", "prints the usage for the program", false, 0, ARG_TYPE_NONE, "--help", "-h", NULL );
        return argparser;
    }

#define argparser_init(program_name, ...) \
    argparser_init_inner(program_name, __VA_ARGS__, NULL)

    void argparser_print_usage( argparser_t* argparser ) {
        printf( "usage: " );
        usage_node_t usage = argparser->usage;

        while ( usage ) {
            printf( "%s %s\n", argparser->program_name, usage->usage );
            if ( usage->next ) {
                printf( "   or: " );
            }
            usage = usage->next;
        }

        for ( size_t i = 0; i < argparser->args_length; i++ ) {
            printf( "\t%s:\t%s", argparser->args[i].meta.identifier, argparser->args[i].meta.aliases[0] );

            for ( size_t j = 1; j < argparser->args[i].meta.aliases_len; j++ ) {
                printf( ", %s", argparser->args[i].meta.aliases[j] );
            }

            printf( "\n\t\t%s\n", argparser->args[i].meta.description );
            printf( "\t\t\trequired: %s\n", argparser->args[i].meta.required ? "true" : "false" );
            printf( "\t\t\ttype:     %s\n", arg_type_str[argparser->args[i].meta.type] );
            printf( "\t\t\tcount:    %zu\n", argparser->args[i].values_len );
        }
    }

    int argparser_parse( argparser_t* argparser, int argc, char** argv ) {

        for ( size_t i = 1; i < argc; i++ ) {
            bool found = false;
            size_t index = (size_t)-1;
            for ( size_t j = 0; j < argparser->args_length; j++ ) {
                for ( size_t k = 0; k < argparser->args[j].meta.aliases_len; k++ ) {
                    if ( !strcmp( argparser->args[j].meta.aliases[k], argv[i] ) ) {
                        found = true;
                        index = j;
                        break;
                    }
                }

                if ( found == true ) {
                    if ( !strcmp( argparser->args[index].meta.identifier, "--help" ) || !strcmp( argparser->args[index].meta.identifier, "-h" ) ) {
                        argparser_print_usage( argparser );
                        exit( EXIT_SUCCESS );
                    }
                    break;
                }
            }
            if ( !found ) {
                if ( argv[i][0] == '-' ) {
                    fprintf( stderr, "[ERROR]: unknown argument `%s` at position `%zu`\n", argv[i], i );
                    argparser_deinit( argparser );
                    return 1;
                } else if ( argparser->argv_capacity == 0 ) {
                    argparser->argv_capacity = 1;
                    argparser->argv = (char**)calloc( 1, sizeof( char* ) );
                    if ( argparser->argv == NULL ) {
                        fprintf( stderr, "[FATAL]: could not allocate memory for argparser_t.argv\n" );
                        exit( EXIT_FAILURE );
                    }
                } else if ( argparser->argc == argparser->argv_capacity ) {
                    argparser->argv_capacity <<= 1;
                    char** reallocation = (char**)realloc( argparser->argv, argparser->argv_capacity * sizeof( char* ) );
                    if ( !reallocation ) {
                        fprintf( stderr, "[FATAL]: could not reallocate memory for argparser_t.argv\n" );
                        exit( EXIT_FAILURE );
                    }

                    argparser->argv = reallocation;
                }
                argparser->argv[argparser->argc] = strdup( argv[i] );
                if ( !argparser->argv[argparser->argc] ) {
                    fprintf( stderr, "[FATAL]: could not duplicate string for argparser_t.argv\n" );
                }
                argparser->argc += 1;

            } else if ( argparser->args[index].found ) {
                fprintf( stderr, "[ERROR]: redefinition of argument `%s` at position `%zu`\n", argparser->args[index].meta.identifier, i );
                argparser_deinit( argparser );
                return 1;

            } else if ( argparser->args[index].meta.type == ARG_TYPE_NONE ) {
                argparser->args[index].found = true;

            } else {
                for ( size_t j = 0; j < argparser->args[index].values_len; j++ ) {
                    char* end = NULL;

                    switch ( argparser->args[index].meta.type ) {
                    case ARG_TYPE_NONE: {
                            assert( 0 && "unreachable" );
                            break;
                        }
                    case ARG_TYPE_U64: {
                            argparser->args[index].values[j].u64 = strtoull( argv[i + j + 1], &end, 10 );
                            break;
                        }
                    case ARG_TYPE_I64: {
                            argparser->args[index].values[j].i64 = strtoll( argv[i + j + 1], &end, 10 );
                            break;
                        }
                    case ARG_TYPE_F64: {
                            argparser->args[index].values[j].f64 = strtod( argv[i + j + 1], &end );
                            break;
                        }
                    case ARG_TYPE_U32: {
                            argparser->args[index].values[j].u32 = (uint32_t)strtoull( argv[i + j + 1], &end, 10 );
                            break;
                        }
                    case ARG_TYPE_I32: {
                            argparser->args[index].values[j].i32 = (int32_t)strtoll( argv[i + j + 1], &end, 10 );
                            break;
                        }
                    case ARG_TYPE_F32: {
                            argparser->args[index].values[j].f32 = strtof( argv[i + j + 1], &end );
                            break;
                        }
                    case ARG_TYPE_U16: {
                            argparser->args[index].values[j].u16 = (uint16_t)strtoull( argv[i + j + 1], &end, 10 );
                            break;
                        }
                    case ARG_TYPE_I16: {
                            argparser->args[index].values[j].i16 = (int16_t)strtoll( argv[i + j + 1], &end, 10 );
                            break;
                        }
                    case ARG_TYPE_U8: {
                            argparser->args[index].values[j].u8 = (uint8_t)argv[i + j + 1][0];
                            break;
                        }
                    case ARG_TYPE_I8: {
                            argparser->args[index].values[j].i8 = (int8_t)argv[i + j + 1][0];
                            break;
                        }
                    case ARG_TYPE_BOOL: {
                            if ( !strcmp( argv[i + j + 1], "true" ) ) {
                                argparser->args[index].values[j].b = true;
                            } else if ( !strcmp( argv[i + j + 1], "false" ) ) {
                                argparser->args[index].values[j].b = false;
                            } else {
                                fprintf( stderr, "[FATAL]: invalid boolean value `%s` at position %zu\n", argv[i + j + 1], i + j + 1 );
                                argparser_deinit( argparser );
                                return 1;
                            }
                            break;
                        }
                    case ARG_TYPE_STRING: {
                            argparser->args[index].values[j].str = strdup( argv[i + j + 1] );
                            if ( !argparser->args[index].values[j].str ) {
                                fprintf( stderr, "[FATAL]: could not allocate memory for argparser_t.args[%zu].values[%zu].str\n", index, j );
                                exit( EXIT_FAILURE );
                            }
                            break;
                        }

                    default: {
                            fprintf( stderr, "[FATAL]: unhandled argument of type %d\n", argparser->args[index].meta.type );
                            exit( EXIT_FAILURE );
                        }
                    }

                    // NOTE(h): these types don't use char* end, so we can just skip over this check for them
                    if ( argparser->args[index].meta.type != ARG_TYPE_STRING &&
                        argparser->args[index].meta.type != ARG_TYPE_BOOL &&
                        argparser->args[index].meta.type != ARG_TYPE_U8 &&
                        argparser->args[index].meta.type != ARG_TYPE_I8 ) {
                        if ( end == NULL || *end != '\0' ) {
                            fprintf( stderr, "[FATAL]: invalid argument at position %zu\n", i + j + 1 );
                            argparser_print_usage( argparser );
                            argparser_deinit( argparser );
                            return 1;
                        }
                    }
                }

                i += argparser->args[index].values_len;
                argparser->args[index].found = true;
            }
        }

        for ( size_t i = 0; i < argparser->args_length; i++ ) {
            if ( !argparser->args[i].found && argparser->args[i].meta.required ) {
                fprintf( stderr, "[FATAL]: missing required argument `%s`\n", argparser->args[i].meta.identifier );
                argparser_print_usage( argparser );
                argparser_deinit( argparser );
                return 1;
            }
        }
        return 0;
    }


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ARGPARSER_H

#include "trace.h"

#include <stdio.h>
#include <stdarg.h>

#include "tau/si.h"


#ifdef _DEBUG
static unsigned long TRACE_LEVEL = TRACE_LEVEL_INFO;
#else
static unsigned long TRACE_LEVEL = TRACE_LEVEL_ERROR;
#endif

namespace debug
{
    __thread void* instance = NULL;
    
    void trace( const char* scope, unsigned long level, const char *format, ... )
    {

        if ( level <= TRACE_LEVEL )
        {
            char temp[ 0x10000 ];
            unsigned int bytes = 0;

            va_list next;
            va_start( next, format );

            memset( &temp, 0, sizeof( temp ) );
            auto file = stdout;
        
            if ( level == TRACE_LEVEL_ERROR )
            {
                file = stderr;
                strcat( temp, "" );
            }
            else if ( level > TRACE_LEVEL_ERROR )
            {
                
                sprintf( temp + strlen( temp ), "[%ld] ", tau::si::Thread::threadId() );
            
                
                if ( scope )
                {
                    sprintf( temp + strlen( temp ), "%s: ", pretty( scope ).c_str() );
                }
                if ( instance )
                {
                    sprintf( temp + strlen( temp ), "0x%x ", ( unsigned int )  ( uintptr_t ) instance );
                }
            }
        
            vsnprintf( temp + strlen( temp ), sizeof( temp ) - strlen( temp ) - 1, format, next );
            strcat( temp, "\n" );
            fprintf( file, "%s", temp );
            va_end( next );
        }
    }   
}

#ifndef VEGA_TRACE_H_
#define VEGA_TRACE_H_

#include <string.h>
#include <string>
#include <typeinfo>


#define	TRACE_LEVEL_INFO			3
#define TRACE_LEVEL_WARNING			2	
#define TRACE_LEVEL_ERROR			1	
#define TRACE_LEVEL_VERBOSE			0	


namespace debug
{
    void trace ( const char* scope, unsigned long level, const char *format, ... );
    
    inline std::string pretty( const std::string& name )
    {
        std::string pretty = name;
        size_t brace = pretty.find( '(' );

        if ( brace != std::string::npos )
        {
            pretty[brace] = 0;
            pretty.append( "()" );
        }

        return pretty;
    }
    
    class Enter
    {
    public:
        Enter ( const char* function )
        : m_function( pretty( function ) )
        {
            debug::trace( NULL, TRACE_LEVEL_INFO, "Entering %s", m_function.c_str ( ) );
        }

        ~Enter ( )
        {
            debug::trace( NULL, TRACE_LEVEL_INFO, "Leaving %s", m_function.c_str ( ) );
        }

    private:
        std::string m_function;
    };
    
     extern  __thread void* instance;
}




#define TRACE_VERBOSE( format, ... ) debug::trace( __FUNCTION__, TRACE_LEVEL_VERBOSE, format, __VA_ARGS__ )
#define ERROR( format, ... ) debug::trace( __FUNCTION__, TRACE_LEVEL_ERROR, format, __VA_ARGS__ )
#ifdef _DEBUG
    #define ENTER()  debug::Enter enter( __PRETTY_FUNCTION__ )
   
    #define TRACE( format, ... ) \
        debug::instance = ( void* ) this; \
        debug::trace( __PRETTY_FUNCTION__, TRACE_LEVEL_INFO, format, __VA_ARGS__ ); \
        debug::instance = NULL;
        

    #define STRACE( format, ... ) debug::trace( __PRETTY_FUNCTION__, TRACE_LEVEL_INFO, format, __VA_ARGS__ )

#else
    #define ENTER() 
    #define TRACE( format, ... )
#endif

#endif 

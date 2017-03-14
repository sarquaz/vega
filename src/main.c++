
#include "Vega.h"


#include <stdio.h>
#include <signal.h>
#include <execinfo.h>



class Terminate : public tau::si::Thread
{

private:

    virtual void routine( )
    {
        ENTER( );

        Vega::instance( )->onTerminate( );
        ::exit( 1 );
    }
};

Terminate* s_terminate = NULL;

class Signals
{
public:
    static void init()
    {
        ::signal( SIGSEGV, handler );
        ::signal( SIGTERM, handler );
        ::signal( SIGINT, handler );
        ::signal( SIGABRT, handler );
        
        ::signal( SIGPIPE, SIG_IGN );
    }
    
private:
    
    static void handler( int signal )
    {
        ENTER();
        
        switch ( signal )
        {
            case SIGSEGV:
            case SIGABRT:
            {
                if ( signal == SIGABRT )
                {
                    Vega::get().cleanup();
                }
                unsigned int length = 15;
                void *array[ length ];
                ::backtrace_symbols_fd( array, ::backtrace( array, length ), STDERR_FILENO );
                ::exit( 1 );
            }
                break;

            case SIGTERM:
            case SIGINT:
            {
                if ( tau::si::Thread::threadId( ) == Vega::instance( )->mainThreadId( ) && !s_terminate )
                {
                    s_terminate = new Terminate( );
                    s_terminate->start();
                    s_terminate->join();
                    ::exit( 1 );
                }
            }
                break;
        }

        
    }
    
    

    
};

class Option
{
public:
    Option( const std::string& optShort, const std::string& optLong, const std::string& description, std::string name = "", bool needValue = false )
    : m_long( optLong ), m_short( optShort ), m_description( description ), m_needValue( needValue ), m_name( name )
    {
        m_value[0] = 0;
    }

    Option( const Option& option )
    {
        m_value[0] = 0;
        m_long = option.m_long;
        m_short = option.m_short;
        m_needValue = option.m_needValue;
        m_description = option.m_description;
        m_name = option.m_name;
    }

    std::string usage()
    {
        std::string usage;

        if ( m_short.size() > 0)
        {
            usage +=  m_short + ", ";
        }

        if ( m_long.size() > 0 )
        {
            usage += m_long;
            if ( m_needValue )
            {
                usage += "=value";
            }
        }
        usage +=  m_description;
        return usage;
    }

    const char* value() const
    {
        return m_value;
    }

    bool parse( const std::string& arg ) const
    {
        if ( arg == m_short ||  arg == m_long )
        {
            if ( m_needValue )
            {
                std::string format = m_long;
                
                if ( arg.find( '\'') != std::string::npos )
                {
                    format += "='%s'";
                }
                else
                {
                    format += "=%s";
                }
                
                if ( sscanf( arg.c_str(), format.c_str(), m_value ) != 1 )
                {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    static bool isOption( const std::string& arg )
    {
        return arg[0] == '-';
    }

    const std::string& name() const
    {
        return m_name;
    }

private:
    std::string m_long;
    std::string m_short;
    bool m_needValue;

    std::string m_description;
    char m_value[128];
    std::string m_name;
};

std::vector< Option > options;

void usage( bool help = false )
{
    printf( "Usage: %s [options] [filename] [filename arguments]\n\n", VEGA_NAME );
    printf("Options: \n");

    for ( int  i = 0; i < options.size(); i++ )
    {
        printf( "%s\n", options[i].usage().c_str() );
    }

    exit( 1 );
}

const Option* findOption( const std::string& arg )
{
    for ( unsigned int i = 0; i < options.size( ); i++ )
    {
        const Option* option = &options[i];
        
        if ( option->parse( arg ) )
        {
            return option;
        }
    }
    
    return NULL;
}

int main(int argc, char* argv[])
{
    Signals::init();
    
    options.push_back( Option( "-h", "--help", "\t\tprints help", "help" ) );
    options.push_back( Option( "", "--version", "\t\tprints version", "version" ) );

    std::string filename;
    lua::Script::Arguments arguments;

    Vega* vega = Vega::instance();
    
    try
    {
        for ( int i = 0; i < argc; i++ )
        {
            std::string string = argv[i];
            arguments.push_back( string );
            
            if ( !i )
            {
                continue;
            }
                    
            if ( Option::isOption( argv[i] ) && filename.empty() )
            {

                const Option* option = findOption( string );
                if ( !option )
                {
                    printf( "Unrecognized option %s \n", string.c_str() );
                    throw argv[i];
                }
                if ( option->name( ) == "help" )
                {
                    usage( true );
                }
                if ( option->name( ) == "version" )
                {
                    printf( "%s version %s\n",  VEGA_NAME, vega->version().c_str() );
                    exit(1);
                }
            }
            else
            {
                if ( filename.empty() )
                {
                    filename = string;
                }
            }
        }
    }
    catch ( ... )
    {
        usage();
    }
    
    if ( filename.empty() )
    {
        usage( true );
    }
    
    
    std::string path;
    char separator = '/';
    size_t lastSlash = filename.rfind( separator );
    
    if ( lastSlash != std::string::npos )
    {
        path = filename.substr( 0, lastSlash );
        filename = filename.substr( lastSlash + 1 );
    }
    
    vega->setScript( lua::Script( filename, arguments ) );
        
    //
    //  get filename path
    //
    if ( path[0] != '/' )
    {
        //
        //  form the absolute path in case of relative filename path
        //  
        std::string current = tau::si::Directory::current( );
   
        if ( path != "." )
        {
            std::string temp = path;
            path = current;
            if ( temp.size() )
            {
                path = path + "/" + temp;
            }
        }
    }
    
    tau::si::Directory::set( path );
    
    int code = 0;
    try
    {
        code = vega->run();
    }
    catch ( const std::runtime_error& e )
    {
        if ( strlen( e.what() ) )
        {
            ERROR( "%s", e.what() );
        }
        
        code = 1;
    }

//	catch( ... )
//    {          
//       // 
//       //   crash
//       // 
//       raise( SIGSEGV ); 
//       code = 1;
//    }
    
    vega->onTerminate();
    return code;
}

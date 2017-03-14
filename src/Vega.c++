#include "Vega.h"
#include "trace.h"

#include <stdexcept>

 #include "api.h"


Vega* Vega::s_instance = NULL;
using namespace tau;

Vega::Vega( )
:  m_liner( NULL ), m_script( NULL ), m_status( 0 ), m_threads( 1 )
{
    ENTER();
    
    char* threads = getenv( "THREADS" );
    if ( threads )
    {
        m_threads = atoi( threads );
    }
    
    handler( tau::Line::Started, ( Vega::Handler ) &Vega::lineEvent );
    m_mainThreadId = tau::si::Thread::threadId();
}


Vega::~Vega( )
{
    ENTER();
    
    s_instance = NULL;  

    if ( m_liner )
    {
        m_liner->stop();
        delete m_liner;
    }
    
    delete m_script;
}

void Vega::lineEvent( tau::Grain& grain )
{
    ENTER();
    new Mill( dynamic_cast< tau::Line& >( grain ) );
}

Vega* Vega::instance()
{
    if ( !s_instance )
    {
        s_instance = new Vega( );
    }

    return s_instance;
}

const Vega& Vega::get()
{
    return *instance();
}

int Vega::run( )
{
    ENTER( );
    
    m_liner = tau::Liner::instance( *this, m_threads );
    m_liner->start( );
    
    return m_status;
 }

 void Vega::onTerminate( )
 {
     tau::si::Gate gate( m_lock );
     if ( !s_instance )
     {
         return;
     }
     
     ENTER();
     
     delete s_instance;
     s_instance = NULL;
}

void Vega::error( const lua::Exception& e )
{
    ERROR( "%s", e.message.c_str() );
   s_instance->setStatus( 1 );
}

void Vega::cleanup() const
{
    
}
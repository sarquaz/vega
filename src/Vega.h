#ifndef _VEGA_H
#define	_VEGA_H

#include "common.h"
#include "lua/lua.h"
#include <tau/liner.h>

class Mill;

class Vega : public tau::in::Female
{
    friend class Mill;
    
public:
    virtual ~Vega();
    static Vega* instance();
    static const Vega& get();
    
    void setScript( const lua::Script& script )
    {
        m_script = new lua::Script( script );
    }
    
    void onTerminate( );
    int run( );
    
    std::string version() const
    {
        return tau::u::fprint( "%d.%d", VEGA_VERSION_MAJOR,  VEGA_VERSION_MINOR );
    }
    
    intptr_t mainThreadId() const
    {
        return m_mainThreadId;
    }
    
    const lua::Script& script() const
    {
        return *m_script;
    }
    
    void setStatus( int code )
    {
        m_status = code;
    }
    
    static void error( const lua::Exception& e );
    void cleanup() const;
    
private:
    Vega();
    void lineEvent( tau::Grain& grain ); 
    
private:
    lua::Script* m_script;
    static Vega* s_instance;
    intptr_t m_mainThreadId;
    tau::Liner* m_liner;
    static unsigned int s_threads;
    static tau::si::Semaphore s_stop;
    tau::si::Lock m_lock;
    int m_status;
    unsigned int m_threads;
};

#endif	/* _VEGA_H */


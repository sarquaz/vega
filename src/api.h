#ifndef API_H
#define	API_H

#include "Vega.h"

using namespace tau;

class Top : public lua::Object
{
public:
    Top()
    {
        setGlobal( true );
    }
    
    typedef std::map< const Top*, Top* > Map;
    
protected:
    virtual void gc( )
    {
         
    }
};

class Api : public lua::Object
{
public:
    static Api* get( const std::string& name )
    {
        return dynamic_cast< Api* >( tau::line().generate( type(), name ) );
    }
    
    static const Grain::Generators& populate();
    static unsigned int type()
    {
        return typeid( Api ).hash_code();
    }
    
protected:
    virtual void gc( )
    {
         delete this;
    }
};

class Mill: public Top
{
public:
    Mill( tau::Line& line );
    virtual ~Mill();
    
private:
    void load();   

    void start( lua::h::Stack& stack );    
    void reload( lua::h::Stack& stack );    
    
    void timerEvent( tau::Grain& grain );
    void threadStopEvent( tau::Grain& grain );
    virtual bool handle( unsigned int type, tau::Grain& grains );
    
    typedef std::list< Api* > ApiList;
    
    void api( Top* api )
    {
        m_api[ api ] = api;
        m_lua.set( *api );
    }
    
    base::Timer* timer()
    {
        return base::Timer::get( this );
    }
    
    
private:
    struct Start
    {
        unsigned int function;
        lua::Runner& runner;
        Mill& main;
        
        Start( Mill& _main, lua::Runner& _runner, unsigned int _function )
        : runner( _runner ), function( _function ), main( _main )
        {
            runner.ref();
        }
        
        ~Start()
        {
            main.lua().unref( function );
        }
        
        void operator()();
    };
    
    virtual unsigned int index( ) const
    {
        return typeid ( this ).hash_code( );
    }
    
    void run();
    
    
    
private:
    lua::Main m_lua;
    Top::Map m_api;
};

class Mall: public Top
{
public:
    Mall( );
    virtual ~Mall()
    {
        ENTER();
    }
    
private:
    virtual bool onProcess( const std::string& name, lua::h::Stack& stack );
    
    virtual unsigned int index( ) const
    {
        return typeid ( this ).hash_code( );
    }
    
    void add( Grain::Populate generator );
    base::Set::Options options( lua::h::Stack& );
        
};

class Set: public Top
{
public:
    Set( );
    virtual ~Set();
    
private:
    void require( lua::h::Stack& stack );
    void info( lua::h::Stack& stack );
    void random( lua::h::Stack& stack );
    void dump( lua::h::Stack& stack );
    void load( lua::h::Stack& stack );
    void compare( lua::h::Stack& stack );
    
    virtual unsigned int index( ) const
    {
        return typeid ( this ).hash_code( );
    }
    
    typedef std::list< tau::si::Module* > ModuleList;
    
private:
    std::default_random_engine m_random;
    ModuleList m_modules;
};

class Pile: public Rock, public Api
{
public:
    static Pile* get( Pill* pill = NULL );
    static Rock* create( );
    
    virtual ~Pile()
    {
    }
    
    virtual void cleanup();
    
    const Pill& pill() const
    {
        return used();
    }
    
private:
    Pile();
    virtual void gc()
    {
        ENTER();
        tau::reuse( *this );
    }
    
    void setUsed( Pill* pill )
    {
        m_used = pill ? pill : &m_pill;
    }
    
    void find( lua::h::Stack& );
    void read( lua::h::Stack& );
    void write( lua::h::Stack& );
    void length( lua::h::Stack& );
    
    Pill& used() const
    {
        assert( m_used );
        return *m_used;
    }
    
    virtual unsigned int index() const
    {
        return typeid( *this ).hash_code();
    }
    
private:
    Pill m_pill;
    Pill* m_used;   
};




#endif	

    
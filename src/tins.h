#ifndef STATES_H
#define	STATES_H

#include "common.h"
#include "api.h"

using namespace tau;
using namespace lua;

class Link: public Api
{
protected:
    Link()
    {
    }
    
    virtual void onIndex( const Main::Router& router, h::Table& table );
    virtual bool handle( unsigned int type, Grain& grain );
    virtual void cleanup();
    
    tau::Pair get();
    
private:
    struct State
    {
        unsigned int type;
        Handler handler;
        tau::List queue;
        
        State( unsigned int _type, Handler _handler )
        : type( _type ), handler( _handler )
        {
            
        }
        
        State( const State& state )
        : type( state.type ), handler( state.handler ), queue( state.queue )
        {
            
        }
    };
    
    void queue( const tau::Pair& pair );
    typedef std::map< const Api::Call*, State > Calls;
    
    void dispatch( Api::Call& call, Grain& grain );
    
private:
    Calls m_calls;
};

class Wait: public Link
{
public:
    virtual ~Wait()
    {
    }
    
    virtual void onAssign( Runner& runner );
   
    
protected:
    Wait();
    unsigned int release( unsigned int count = 0 );
    
    virtual void cleanup();
    Interval parse( h::Stack& ) const;
    virtual void onIndex( const Main::Router& router, h::Table& table ); 
    
private:
    
    virtual unsigned int waitEvent()
    {
        return 0;
    }
    
    void wait( h::Stack& );
    virtual void onTimer( base::Timer& timer )
    {
        
    }

    virtual void onRunnerStop( Runner& );
    void timer( Grain& grain );
    enum Type
    {
        Timeout = 1,
        Infinite,
        Resume
    };
    
    virtual bool onTimeout( Runner& )
    {
        return false;
    }
    
    virtual void onWait()
    {
    }
    
    void add( Runner& runner )
    {
        runner.females().add( *this );
    }
    
    void remove( Runner& runner )
    {
        runner.females().remove( *this );
    }
    
    struct Joined
    {
        std::map< Runner*, base::Timer* > map;
        Wait& wait;
        
        Joined( Wait& _wait )
        : wait( _wait )
        {
        }
        
        void clear();
        
        Runner* remove( Runner* runner = NULL );
        void add( base::Timer& timer, Runner& runner );
        unsigned int size() const
        {
            return map.size();
        }
        
    };
    
private:
    Joined m_joined;
    Interval m_interval;
};

class Tin: public Wait, public Rock
{
public:
    virtual ~Tin();
        
    static Tin* get( const std::string& name )
    {
        return dynamic_cast< Tin* >( tau::line().generate( type(), name ) );
    }
    
    static const Grain::Generators& populate( );
    virtual void setBase( base::Base* base );
    static unsigned int type() 
    {
        return typeid( Tin ).hash_code();
    }
    
protected:    
    Tin( base::Base* base = NULL );
    base::Base& base()
    {
        assert( m_base );
        return *m_base;
    }
    
    const base::Base& base() const 
    {
        assert( m_base );
        return *m_base;
    }
    
    virtual void cleanup();
    virtual void suspend( );
    
    template< class Allocate > static Grain* create( const std::type_info& type, Allocate allocate )
    {
        return dynamic_cast < Grain* > ( tau::line().tok( type, [ & ]( ) { return allocate( ); } ) );
    }
    
    virtual void start( base::Base& base );
    virtual void onAssignedStop()
    {
        cleanup();
    }
    
private:    
    virtual bool handle( unsigned int type, Grain& grain );
    virtual void onTimer( base::Timer& );
    virtual void gc()
    {
        Rock::deref();
    }
    
    virtual unsigned int index() const
    {
        return hash();
    }
    
    void schedule( const tau::Pair& pair );

private:
    tau::base::Base* m_base;    
};

class Event : public Tin
{
public:
    Event( );
    virtual ~Event()
    {
        ENTER();
    }
    
    static Grain* create( )
    {
        return Tin::create( typeid( Event ), [](){ return new Event(); } );
    }

private:
    virtual unsigned int hash( ) const
    {
        return typeid ( *this ).hash_code( );
    }

    void set( h::Stack& );
    
    virtual void cleanup();
    
    virtual void onWait();
    void release();
    
    virtual void onRunnerStopped( Runner& )
    {
    }
    
private:
    unsigned int m_count;
};

class Net: public Tin
{
public:
    Net( );
    virtual ~Net()
    {
    }
    
    void close();
    static Grain* create()
    {
        return Tin::create( typeid( Net ), [](){ return new Net(); } );
    }
    
private:
    virtual unsigned int hash() const
    {
        return typeid( *this ).hash_code();
    }
    
    
    tau::base::Net& net()
    {
        return dynamic_cast< tau::base::Net& >( Tin::base() );
    }
    
    const tau::base::Net& net() const
    {
        return dynamic_cast< const tau::base::Net& >( Tin::base() );
    }
    
    unsigned int length() const
    {
        return net().in().length();
    }
    
    void id( h::Stack& stack );
    void send( h::Stack& stack );
    void receive( h::Stack& stack );

    void read( h::Stack& stack );
    void close( h::Stack& stack )
    {
        close();
    }
    
    void find( h::Stack& stack );
    void length( h::Stack& stack );
    void peer( h::Stack& stack );
    
    
    void readEvent( tau::Grain& grain );
    void closeEvent( tau::Grain& grain );
    
    void resume( );
    
private:
    unsigned int m_read;
};


class Flow: public Tin
{
public:
    Flow( );
    static Grain* create()
    {
        return Tin::create( typeid( Flow ), [](){ return new Flow(); } );
    }
    
    static Flow& get( Runner& );
    
    virtual ~Flow()
    {
        ENTER();
    }
    
private:
    virtual unsigned int hash( ) const
    {
        return typeid ( *this ).hash_code( );
    }
    
    void id( h::Stack& );
    void end( h::Stack& );  

    virtual bool onTimeout( Runner& );
    virtual void onIndex( const Main::Router& router, h::Table& table );
    
    virtual void onRunnerStop( Runner& );
    
    void use( Runner& runner );
    virtual void cleanup();
    
    void unref();
        
    Runner& runner();
        
private:
    unsigned int m_reference;
    Runner* m_used;
};

class Jet: public Tin
{
public:
    virtual ~Jet()
    {
    }
    
protected:
    Jet(  );
    void process( Pill& pill );
    
    enum Type
    { 
        Read,
        Data
    };
    
    virtual void cleanup();
    
private:
    virtual void onData( Grain& grain )
    {
        
    }
    
    virtual void doRead( h::Stack& stack )
    {
        
    }
    
    void dataEvent( Grain& grain );
    void exitEvent( Grain& grain );
    void writeEvent( Grain& grain );
    
    virtual bool onProcess( const std::string& name, h::Stack& stack );
    
    typedef std::map< std::string, Tin::Call* > Calls;
    
    void write( h::Stack& );
    void join( h::Stack& );
    
    virtual base::Set& jet() = 0;
    
    virtual unsigned int waitEvent()
    {
        return base::Set::Close;
    }
    
private:
    Type m_type;
    Calls m_calls;
    bool m_read;
};

class Process: public Jet
{
public:
    Process( );
    virtual ~Process()
    {
        ENTER();
    }
    
    static Grain* create()
    {
        return Tin::create( typeid( Process ), [](){ return new Process(); } );
    }
    
    void start( h::Stack& stack );
    
    
private:
    base::Process& process()
    {
        return dynamic_cast< base::Process& >( Tin::base() );
    }
    
    virtual base::Set& jet()
    {
        return dynamic_cast< base::Set& >( Tin::base() );
    }
    
    virtual void setBase( base::Base* base );
    virtual void cleanup();
    
    virtual void onData( Grain& grain );
    virtual void doRead( h::Stack& stack );
    
    
private:

    
};

#endif	

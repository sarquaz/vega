#include "tins.h"
#include "lua/types.h"



void Link::onIndex( const Main::Router& router, h::Table& table )
{
    ENTER();
    
    std::for_each( router.calls().begin(), router.calls().end(), [ & ] ( const Main::Router::Call::Map::value_type& value ) 
    { 
        auto& call = value.second;
        if ( call.id )
        {
            try
            {
                auto type = call.id;
                TRACE( "call 0x%x(%s) mapped to type %d", &call, call.name.c_str(), type );
                auto& handlers = in::Female::handlers();
                
                m_calls.insert( Calls::value_type( &call, State( type, handlers.at( type ) ) ) );
                handlers.erase( type );
            }
            catch ( const std::out_of_range& )
            {
                assert( false );
            }
        }
    } );
}

void Link::queue( const tau::Pair& pair )
{
    ENTER();
    State* state = NULL;
    auto found = m_calls.find( m_call );

    if ( found != m_calls.end() )
    {
        state = &found->second;
    }
    else if ( !m_call )
    {
        std::for_each( m_calls.begin(), m_calls.end(), [ & ] ( Calls::value_type& value ) 
        {
            if ( value.second.type == pair.first )
            {
                state = &value.second;
            }
        } );
    }
    
    if ( state )
    {
        TRACE( "queuing event %d", pair.first );
        state->queue.push_back( pair );
    }
}

void Link::dispatch( Api::Call& call, Grain& grain )
{
    ENTER();
    try
    {
        auto& state = m_calls.at( &call );
        TRACE( "found handler for call %s type %d", call.name.c_str(), state.type );
        ( this->*state.handler )( grain );
    }
    catch ( const std::out_of_range& e )
    {
        TRACE( "handler for call %s not found", call.name.c_str() );
    }
}

bool Link::handle( unsigned int type, Grain& grain )
{
    TRACE( "handling type %d", type );
    
    if ( !in::Female::handle( type, grain ) )
    {
        auto& way = Api::runner().way();
        TRACE( "call 0x%x: runner way: object 0x%x call 0x%x", m_call, way.object, way.call );
        if ( way.object == this && way.call == m_call )
        {
            dispatch( Api::call(), grain );
        }
        else
        {
            queue( tau::Pair( type, &grain ) );
        }
    }
    
    return true;
}

void Link::cleanup()
{
    ENTER();
    
    std::for_each( m_calls.begin(), m_calls.end(), [ & ]( Calls::value_type& value ) { value.second.queue.clear(); } );
}

tau::Pair Link::get()
{
    auto& state  = m_calls.at( m_call );
    auto pair = state.queue.back();
    state.queue.pop_back();
    return pair;
}

void Wait::onIndex( const Main::Router& router, h::Table& table )
{
    Link::onIndex( router, table );
    table.set( "errors", 1 );
}

void Wait::onAssign( Runner& runner )
{
    runner.females( ).add( *this );
}

Wait::Wait( )
: m_joined( *this )
{
    Api::method( "wait", ( Api::Method ) & Wait::wait, waitEvent() );
    in::Female::handler( base::Timer::Timeout, ( Wait::Handler ) &Wait::timer );
}

void Wait::Joined::clear()
{
    while( map.size() )
    {
        remove();
    }
}
          
Runner* Wait::Joined::remove( Runner* runner )
{
    if ( map.empty() )
    {
        return NULL;
    }
    
    auto i = map.begin();
    if ( runner )
    {
        i = map.find( runner );
    }
    
    if ( i != map.end() )
    {
        runner = i->first;
        i->second->deref();
        map.erase( i );
    }
    
    return runner;
}

void Wait::Joined::add( base::Timer& timer, Runner& runner )
{
    wait.male( runner );
    timer.setGrain( &runner );
    map[ &runner ] = &timer;
}

void Wait::wait( h::Stack& stack )
{
    ENTER();
    
    unsigned int type = Timeout;
    if ( stack.top() )
    {
        m_interval = parse( stack );
    }
    else
    {
        type = Infinite ;
        m_interval = Interval( Interval::Infinite );
    }
    
    auto timer = base::event( this, type )( &m_interval );
    
    m_joined.add( *timer, Api::runner() );
    Api::suspend();
    
    onWait();
}

Interval Wait::parse( h::Stack& stack ) const
{
    long seconds = 0;
    long micros = 0;
    long millis = 0;
    if ( stack.type() == Table ) 
    {
        h::Table table = stack.table( );
        auto values = table.values();
        for ( auto i = values.begin( ); i != values.end( ); i++ )
        {
            auto& key = i->first;
            auto number = atoi( i->second.c_str( ) );
            
            if ( key == "sec" )
            {
                seconds = number;
            }
            if ( key == "msec" )
            {
                millis = number ;
            }
            if ( key == "usec" )
            {
                micros = number;
            }
        }
    }
    else 
    {
        if ( stack.type() != Number  )
        {
            throw lua::Exception( "expecting passed number" );
        }
        seconds = stack.number();
    }
    
    Interval interval( seconds, millis, micros );
    return interval;
}

void Wait::onRunnerStop( Runner& runner )
{
    ENTER();
    m_joined.remove( &runner );
}

void Wait::timer( Grain& grain )
{
    auto& timer = dynamic_cast< base::Timer& >( grain );
        
    switch ( timer.type() )
    {
        case Infinite:
            return;
            
        case Timeout:
        {
            auto runner = dynamic_cast< Runner* >( timer.grain() );
            if ( !onTimeout( *m_joined.remove( runner ) ) )
            {
                Api::error( lua::Exception( "timeout" ) );
            }        
        }
        break;
        
        default:
            onTimer( timer );
    }
            
}

unsigned int Wait::release( unsigned int count )
{
    if ( !count )
    {
        count = m_joined.size();
    }
    
    Runner::List list;
    for ( auto i = 0; i < count; i++ )
    {
        auto runner = m_joined.remove();
        if ( runner )
        {
            list.push_back( runner );
        }
    }
    
    std::for_each( list.begin(), list.end(), [ ]( Runner* runner ) { runner->next(); } );
    return list.size();
}

void Wait::cleanup()
{
    ENTER();
    
    m_joined.clear();
    Api::clear();
    Link::cleanup();
}

Tin::Tin( base::Base* base )
: m_base( base )
{
    setBase( base );
}

Tin::~Tin()
{
    if ( m_base )
    {
        base().deref();
    }
}

void Tin::setBase( base::Base* base )
{
    if ( base )
    {
        base->females( ).add( *this );
    }
    
    m_base = base;
}

void Tin::start( base::Base& base )
{
    m_base = &base;
}

void Tin::cleanup()
{   
    if ( m_base )
    {
        base().females().remove( *this );
        base().deref();
        m_base = NULL;
    }
    
    Wait::cleanup();
}

bool Tin::handle( unsigned int type, Grain& grain )
{
    try
    {
        Wait::handle( type, grain );
    }
    catch( lua::Exception& e )
    {
        Api::error( e );
    }
    
    return true;
}

void Tin::schedule( const tau::Pair& pair )
{
    ENTER();
    base::event( this, pair.first )( NULL, pair.second );
}
void Tin::suspend( )
{
    ENTER();
    try
    {
        schedule( Link::get() );
    }
    catch( ... )
    {
        
    }
    
    Api::suspend();    
}

void Tin::onTimer( base::Timer& timer )
{
    if ( timer.grain() )
    {
        handle( timer.type(), *timer.grain() );
        timer.deref();
    }
}

const Grain::Generators& Tin::populate( )
{
    ENTER();
    
    tau::add( type(), ( Grain::Generator ) &Flow::create, "runner" );
    tau::add( type(), ( Grain::Generator ) &Net::create, "net" );
    tau::add( type(), ( Grain::Generator ) &Process::create, "process" );
    tau::add( type(), ( Grain::Generator ) &Event::create, "event" );
    
    return *tau::generators( type() );
}

Event::Event(  )
: m_count( 0 )
{
    Api::method( "set", ( Tin::Method ) & Event::set );
    Api::setName( "event" );
}

void Event::set( h::Stack& stack )
{
    ENTER();
    unsigned int count = stack.number();
    m_count += count ? count : 1;
    
    release();
}

void Event::release()
{
    if ( m_count ) 
    {
        m_count -= Wait::release( m_count );
    }
}

void Event::onWait()
{
    release();
}

void Event::cleanup()
{
    ENTER();
    m_count = 0;
    Wait::cleanup();
}

Net::Net(  )
: m_read( 0 )
{
    ENTER();
    
    Api::method( "id", ( Tin::Method ) &Net::id );
    Api::method( "send", ( Tin::Method ) &Net::send );
    Api::method( "length", ( Tin::Method ) &Net::length );
    Api::method( "find", ( Tin::Method ) &Net::find );
    Api::method( "read", ( Tin::Method ) &Net::read );
    Api::method( "close", ( Tin::Method ) &Net::close );
    Api::method( "receive", ( Tin::Method ) &Net::receive );
    Api::method( "peer", ( Tin::Method ) &Net::peer );    
}

void Net::id( h::Stack& stack )
{
    ENTER();
    stack.push( net().fd() );
}

void Net::peer( h::Stack& stack )
{
    ENTER();
    
    h::Table table( stack.lua() ); 
    table.set( "host", net().host() );
    table.set( "port", net().port() );
    
    stack.push( table );
}
void Net::receive( h::Stack& stack )
{
    ENTER();
    
    if ( !length() )
    {
        Tin::suspend();
    }
}

void Net::send( h::Stack& stack )
{
    ENTER();
//    net().awrite( stack.data() );
}

void Net::close( )
{
    ENTER();
    net().aclose();
}

void Net::readEvent( Grain& ) 
{
    ENTER();
    
    resume();
}

void Net::find( h::Stack& stack )
{
    ENTER();
//    stack.push( net().input().find( stack.data() ) );
}

void Net::resume( )
{
    ENTER();
    h::Arguments arguments;
    
    if ( m_read )
    {
        //arguments.addBuffer( data( m_read ) );
        m_read = 0;
    }
    
    Api::resume( &arguments );
}


void Net::read( h::Stack& stack )
{
    ENTER();
    
    unsigned int length = 0;
    if ( stack.type() == Number )
    {
        length = stack.integer();
    }
    
    if ( !this->length() )
    {
        m_read = length;
        Tin::suspend();
        return;
    }
    
    if ( !length )
    {
        length = this->length();
    }
    
    //stack.pushBuffer( data( length ) );
}

void Net::length( h::Stack& stack )
{
    ENTER();

    stack.push( ( int ) length() );
}

void Net::closeEvent( Grain& grain ) 
{
    ENTER();
    
    throw lua::Exception( "error connecting to %s:%d", net().host().c_str(), net().port() );
}

Flow::Flow( )
: m_reference( 0 ), m_used( NULL )
{
    Api::method( "id", ( Api::Method ) & Flow::id );
    Api::method( "terminate", ( Api::Method ) & Flow::end );
    
    Api::setName( "jet" );
}

Flow& Flow::get( Runner& _runner )
{
    auto& runner = *( dynamic_cast< Flow* >( create() ) );
    runner.use( _runner );
    return runner;
}

void Flow::use( Runner& runner )
{
    m_used = &runner;
    this->male( runner );
}

void Flow::cleanup()
{
    ENTER();
    m_used = NULL;
    Tin::cleanup();
}

void Flow::unref()
{
    if ( m_reference )
    {
        Api::lua().unref( m_reference );
        m_reference = 0;
    }
}

bool Flow::onTimeout( Runner& runner )    
{
    ENTER();
    unref();

    if ( m_used )
    {
        m_used->stop();
        return false;
    }    
    
    Api::resume();
    return true;
}

void Flow::onRunnerStop( Runner& runner )
{
    ENTER();
    
    if ( &runner == m_used )
    {
        Wait::release();
    }
}

void Flow::id( h::Stack& stack )
{
    ENTER();
    
    unref();
    
    h::Table table( stack.lua() ); 
    table.set( "id", runner().id() );
    table.set( "status", runner().running() ? "running" : "stopped" );
    
    stack.push( table );
}

Runner& Flow::runner()
{
    if ( m_used )
    {
        return *m_used;
    }
    
    return *Api::assigned();
}

void Flow::end( h::Stack& stack )
{
    ENTER();
    unref();
    
    if ( m_used )
    {
        m_used->stop();
    }
    else
    {
        Api::suspend();
        Api::runner().stop();
    }
}

void Flow::onIndex( const Main::Router& router, h::Table& table )
{
    m_reference = table.reference();
    Wait::onIndex( router, table );
}

Jet::Jet()
{
    std::list< std::string > calls = {"wait", "read"};
    
//    for ( auto i = calls.begin(); i != calls.end(); i ++ )
//    {
//          auto call = Tin::method( *i, base::Jet::Data );
//          m_calls[ *i ] = call;
//    }
    
    
    Api::method( "write", ( Tin::Method ) &Jet::write );
    
    
    in::Female::handler( base::Set::Write, ( Tin::Handler ) & Jet::writeEvent );
    in::Female::handler( base::Set::Data, ( Tin::Handler ) & Jet::dataEvent );
    in::Female::handler( base::Set::Close, ( Tin::Handler ) & Jet::exitEvent );
}


void Jet::writeEvent( Grain& grain )
{
    ENTER();
}

void Jet::join( h::Stack& stack )
{
    ENTER();
    Tin::suspend();
}

void Jet::write( h::Stack& stack )
{
    ENTER();
    
    if ( stack.type() == Nil )
    {
        throw lua::Exception( "expecting passed value" );
    }

    if ( stack.type() == Table )
    {
        h::Table table = stack.table( );
        void* data = table.data( "__instance" );

        if ( data )
        {
            auto pile = dynamic_cast< Pile* >( Object::get( data ) );
            //jet().push( pile->pill() );
            return;
        }
        else
        { 
            throw lua::Exception( "expecting passed pile instance" );
        }
    }
    
    //jet().push( stack.data() );
}

bool Jet::onProcess( const std::string& name, h::Stack& stack )
{
    ENTER();
    TRACE( "processing %s", name.c_str() );
    auto found = m_calls.find( name );
    
    if ( found != m_calls.end() )
    {
        Api::setCall( found->second );
        m_read = name == "read";
        
        doRead( stack );
        Tin::suspend();
        return true;
    }
    
    return false;
}

void Jet::dataEvent( Grain& grain )
{
    ENTER();
    
    onData( grain );
}

void Jet::exitEvent( Grain& grain )
{
    ENTER();
    Wait::resume();
}


void Jet::process( Pill& pill )
{
    ENTER();
    h::Arguments arguments;
    
    TRACE( "last call read: %d", m_read );
    
    if ( m_read )
    {
        TRACE( "read", "" );
        arguments.add( pill );
       
    }
    else 
    {
        TRACE( "data", "" );
        
        auto pile = Pile::get( &pill );
        arguments.add( *pile );
    }
    
    Tin::resume( &arguments );
    
    if ( m_read )
    {
        pill.clear();
    }
}

void Jet::cleanup()
{
    ENTER();
    m_read = false;
    Tin::cleanup();
}


Process::Process(  ) 
{
    ENTER();
    
    Api::method( "start", ( Tin::Method ) &Process::start );
    Api::setName( "process" );
}

void Process::start( h::Stack& stack )
{
    Api::setCall( NULL );
    process().read();
}

void Process::doRead( h::Stack& stack )
{
    ENTER();
    
    unsigned int id = 0; 
    if ( stack.type() == Number )
    {
        id = stack.integer();
    }
//    process().read( id );    
}

void Process::onData( Grain& grain )
{
    ENTER();

    auto& stream = dynamic_cast< base::Process::Stream& >( grain );
    
    Jet::process( stream.in() );
}

void Process::setBase( base::Base* base )
{
    Tin::setBase( base );
    //process().read( base::Process::Stream::Err );
}

void Process::cleanup()
{
    ENTER();
    Jet::cleanup();
}


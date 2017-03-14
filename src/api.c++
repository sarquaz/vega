#include "Vega.h"
#include "tins.h"

const Grain::Generators& Api::populate()
{    
    tau::add( type(), ( Grain::Generator ) &Pile::create, "pile" );
    
    return *tau::generators( type() );
}

Mill::Mill( Line& line ) 
: m_lua( Vega::get().script() )
{
    ENTER();
    
    Top::setName( "main" );

    Top::method( "start", ( Api::Method ) &Mill::start );
    Top::method( "reload", ( Api::Method ) &Mill::reload );
    
    in::Female::handler( base::Timer::Timeout, ( Mill::Handler ) &Mill::timerEvent );
    in::Female::handler( Line::Stopped, ( Mill::Handler ) &Mill::threadStopEvent );
    
    line.females().add( *this );
    
#ifdef _DEBUG
    line.setMax( 0 );
#endif
    
    load();
}

Mill::~Mill()
{
    ENTER();
    
    for( auto i = m_api.begin(); i != m_api.end(); i++ )
    {
        delete i->second;
    }
}

bool Mill::handle( unsigned int type, tau::Grain& grain )
{
    tau::in::Female::handle( type, grain );
    
    if ( type == tau::Line::Stopped )
    {
        delete this;
    }
    
    return true;
}

void Mill::run()
{
    ENTER();
    
    Top::clear();
    m_lua.stop();
    
    auto& started = lua::Main::get().runner();
    started.setStart( *lua::types::Function::get( Vega::get().script() ) );
    
    try
    {
        started.run();
    }
    catch ( const lua::Exception& e )
    {
        Vega::error( e );
    }
}
void Mill::load()
{
    ENTER();
        
    m_lua.open( );
    m_lua.set( *this );
    api( new Mall( ) );
    api( new Set( ) );
    
    run( );
}

void Mill::Start::operator ()()
{
    ENTER();
    
    auto& started = lua::Main::get().runner();
    
    started.setStart( *lua::types::Function::get( function ) );
    auto& runner = Flow::get( started );
    started.run();
    
    lua::h::Arguments arguments;
    arguments.add( runner );
    
    this->runner.run( &main, &arguments );
    this->runner.deref();
}

void Mill::timerEvent( tau::Grain& grain )    
{
    ENTER();
    
    auto& base = dynamic_cast < base::Base& > ( grain );
    
    if ( base.data() )
    {
        Start& start = *static_cast < Start* > ( base.data( ) );
        start( );
        delete &start;
    }
    else
    {
        run();
    }
    
    base.deref( );
}

void Mill::threadStopEvent( tau::Grain& grain )
{
    ENTER();
    
    Vega::instance()->setStatus( m_lua.success() ? 0 : 1 );
    m_lua.close();
}

void Mill::start( lua::h::Stack& stack )
{
    ENTER();
    
    auto timer = base::event( this )();
    timer->setData( new Start( *this, Top::runner(), stack.reference() ) );
    
    Top::suspend();
}

void Mill::reload( lua::h::Stack& stack )
{
    ENTER();
    stack.runner().suspend( this );
    base::event( this )();
}

Mall::Mall( )
{
    setName( "mall" );
    
    add( Tin::populate );
    add( Api::populate );
    
    base::Set::populate();
}

void Mall::add( Grain::Populate generator )
{
    auto& generators = generator();
    
    for ( auto i = generators.begin(); i != generators.end(); i++ )
    {
        Top::method( i->first );
    }
}

bool Mall::onProcess( const std::string& name, lua::h::Stack& stack )
{
    ENTER();

    TRACE( "%s", name.c_str() );
    
    auto api = Api::get( name );
    if ( !api )
    {
        api = Tin::get( name );
    }
    
    if ( !api )
    {
        return false;
    }
    
    TRACE("found tin 0x%x for name %s", api, name.c_str() );
    
    auto tin = dynamic_cast< Tin* > ( api );
    auto startable = base::Set::get( name );
    
    if ( startable )
    {
        base::Set::Options options = this->options( stack );
        startable->start( options );
        tin->setBase( startable );
    }
    
    
    stack.push( *api );
    return true;
}

base::Set::Options Mall::options( lua::h::Stack& stack )
{
    base::Set::Options options;
    
    if ( stack.type( ) != lua::Table )
    {
        throw lua::Exception( "expecting table" );
    }

    auto values = stack.table( ).values( );
    for ( auto i = values.begin( ); i != values.end( ); i++ )
    {
        auto key = i->first;
        auto value = i->second;

        TRACE( "option %s: %s", key.c_str( ), value.c_str( ) );
        options[ key ] = value;
    }
    
    return options;
}

void Set::require( lua::h::Stack& stack )
{
    auto name = u::fprint( "%s.vega.so", stack.string().c_str() );
    si::Module* module = NULL;

    try
    {
        module = new si::Module( name );

        void ( *open )( void* );
        *( void ** ) ( &open ) = module->symbol( "vega_open" );

        open( &stack );
        m_modules.push_back( module );
    }
    catch( tau::Error& e )
    {
    }
}

Set::Set( )
{
    setName( "set" );
    
    Top::method( "require", ( Api::Method ) &Set::require );
    Top::method( "info", ( Api::Method ) &Set::info );
    Top::method( "random", ( Api::Method ) &Set::random );
    
    Top::method( "dump", ( Api::Method ) &Set::dump );
    Top::method( "load", ( Api::Method ) &Set::load );
    Top::method( "compare", ( Api::Method ) &Set::compare );
    
    m_random.seed( tau::si::millis() + tau::line().id() );
}

Set::~Set()
{
    ENTER();
    for( auto i = m_modules.begin(); i != m_modules.end(); i++ )
    {
        delete *i;
    }
}

void Set::dump( lua::h::Stack& stack )
{
    ENTER();
    
    auto value = stack.value();
    
    if ( !value ) 
   {
        throw lua::Exception( "error dumping object" );
    }
    
    
    Pill pill;
    
    lua::types::Value::dump( pill, *value );
    value->destroy();
    
    TRACE( "pill size %d offset %d", pill.size(), pill.offset() );

    stack.push( pill );
}

void Set::load( lua::h::Stack& stack )
{
    ENTER();

    Pill data = stack.data();    
    auto value = lua::types::Value::load( data );
    if ( value )
    {
        stack.push( *value );
        value->destroy();
    }
    else
    {
        throw lua::Exception( "error loading object" );
    }
}

void Set::compare( lua::h::Stack& stack )
{
    ENTER();
    
    std::list< lua::types::Value* > values;
    while ( values.size() < 2 )
    {
        values.push_back( stack.value() );
    }
    
    stack.push( *values.front() == *values.back() );
    for ( auto i = values.begin(); i != values.end(); i++ )
    {
        ( *i )->destroy();
    }
}


 void Set::info( lua::h::Stack& stack )
{
    lua::h::Table table( stack.lua() );
    
    table.set( "line", tau::line().id() );
    table.set( "pid", si::Process::id() );
    table.set( "version", Vega::get().version() );
    
    stack.push( table );
}
 
void Set::random( lua::h::Stack& stack )
{
    unsigned int max = stack.number();
    int value = max ? m_random() % max : m_random();
    stack.push( value ); 
}


Pile* Pile::get( Pill* pill )
{
    auto pile = dynamic_cast < Pile* > ( tau::get( typeid( Pile ), []( ) {
        return new Pile( ); } ) );
       
    pile->setUsed( pill );
    return pile;
}

Rock* Pile::create( )
{
    return dynamic_cast < Rock* > ( get() );
       
}

Pile::Pile()
: m_used( NULL )
{
    ENTER();
    Api::method( "read", ( Api::Method ) &Pile::read );
    Api::method( "write", ( Api::Method ) &Pile::write );
    Api::method( "find", ( Api::Method ) &Pile::find );
    Api::method( "length", ( Api::Method ) &Pile::length );
    
}

void Pile::cleanup()
{
    ENTER();
    m_used = NULL;
    m_pill.clear();
}

void Pile::read( lua::h::Stack& stack )
{
    ENTER();
    
    unsigned int length = 0;
    
    if ( stack.top() )
    {
        if ( stack.type() == lua::Number )
        {
            length = stack.integer();
        }
        else
        {
            throw lua::Exception( "expecting number" );
        }
    }
    
    if ( !length )
    {
        length = used().length();
    }
    
    const char* data = used().read( length );
    stack.push( data, length );
}

void Pile::write( lua::h::Stack& stack )
{
    ENTER();
    used().add( stack.data() );
}

void Pile::length( lua::h::Stack& stack )
{
    ENTER();
    stack.push( ( int ) used().length() );
}

void Pile::find( lua::h::Stack& stack )
{
    ENTER();
    stack.push( used().find( stack.data() ) );
}




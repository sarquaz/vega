#include "main.h"

namespace lua
{
    std::string Main::s_global = "__vega";
        
    __thread Main* t_main = NULL;
        
    void Main::open(  )
    {
        ENTER( );
        
        t_main = this;
        m_lua = State::create();
        types::Value::populate();
    }
    
    Main& Main::get()
    {
        assert( t_main );
        return *t_main;
    }
    
    void Main::gc()
    {
        ENTER();
        tau::base::event( this )();
    }

    void Main::close( )
    {
        ENTER();
        if ( !m_lua )
        {
            return;
        }
        
        stop();

        for ( auto i = m_routers.begin( ); i != m_routers.end( ); i++ )
        {
            delete i->second;
        }
        
        m_routers.clear( );
        
        lua_close( m_lua );
        m_lua = NULL;
    }
    

    void Main::set( Object& object )
    {
        ENTER();
        
        h::Table table( *this );
        Index index( &object );
        index( table );
        
        h::Table global( *this, name() );
        global.set( object.name(), table );
    }
    
    Exception::~Exception( )
    {
        if ( runner )
        {
            runner->stop( );
        }
    }
    
    Exception::Exception( const char* format, ... )
    : runner( NULL )
    {
        va_list next;
        va_start( next, format );
        message = types::String::format( format, next );
        va_end( next );
    }
    Main::Main( const Script& script )
    : m_script( script ), m_success( true ), m_init( false )
    {
        tau::in::Female::handler( Runner::Stop, ( Main::Handler ) & Main::runnerStopped );
        tau::in::Female::handler( tau::base::Timer::Timeout, ( Main::Handler ) & Main::timer );
    }
    
    void Main::Index::operator ()( h::Table& table )
    {
        ENTER();
        
        auto hash = object.index();
        
        Router* router = NULL;
        
        auto found = routers.find( hash );
        if ( found == routers.end( ) )
        {
            router = new Router( object );
            routers[ hash ] = router;
        }
        else
        {
            router = found->second;
        }
        
        object.onIndex( *router, table );
        
        h::Table index( table.lua() );
        index.setReference( "__index", router->reference() );
        
        table.setmetatable( index );
    }
    
    void* Main::Router::Dispatch::instance( h::Stack& stack )
    {
        void* data = NULL;
        h::Table table = stack.table( );

        if ( stack.top( ) )
        {
            table.setIndex( -stack.top( ) );
            data = table.data( "__instance" );

            if ( data )
            {
                table.setPop( false );
                runner().remove( table.index( ) );
                stack.setTop( stack.top( ) - 1 );
            }
        }
        
        return data;
    }
    
    void Main::Router::Dispatch::operator()( h::Stack& stack )
    {
        _runner = &stack.runner();
        
        void* data = NULL;
        if ( stack.type() == Table )
        {
            data = instance( stack );
        }
        
        if ( !data )
        {
            data = runner().touserdata( lua_upvalueindex( 2 ) );
        }
        
        auto call = ( Call* ) runner().touserdata( lua_upvalueindex( 1 ) );
        assert( call );

        object = Object::get( data );            
        if ( object )
        {
            ( *object )( *call, stack ); 
        }
        else
        {
            throw Exception( "no instance found" );
        }
    }
    
    int Main::Router::dispatch( lua_State* lua )
    {        
        auto& runner = Runner::get( lua_getdata( lua ) );
        unsigned int count = 0;
        
        try
        {
            h::Stack stack( runner );

            Dispatch()( stack );
            count = stack.count();
            
            if ( stack.top( ) )
            {
                assert( lua_gettop( lua ) == stack.top( ) );
            }
            else
            {
                assert( lua_gettop( lua ) == stack.count( ) );
            }
        }

        catch( const Exception& e )
        {
            return runner.error( e.message );
        }
                
        if ( runner.suspended() )
        {
            return runner.yield();
        }

        return count;
    }

    void Main::Router::add( const Object& object )
    {
        if ( m_reference )
        {
            return;
        }

        auto& main = Main::get();
        h::Table table( main );
        
        auto& calls = object.calls();
        
        std::for_each( calls.begin(), calls.end(), [ & ]( const Object::Call::Map::value_type& value ) 
        {
            auto inserted = m_calls.insert( value );
            auto& call = ( inserted.first )->second;
            
            h::Arguments upvalues;
            upvalues.add( &call );

            if ( object.global() )
            {
                upvalues.add( ( void* ) object.data() );
            }
            
            table.set( call.name, dispatch, upvalues );
        } );
        
        {
            h::Stack stack( main, 1 );
            m_reference = stack.reference();
        }
    }
    
    void Main::init() 
    {
        if ( m_init )
        {
            return;
        }
        
        ENTER();
        
        State::execute( tau::u::fprint( "require '%s'", VEGA_NAME ) );
        
        h::Table table( *this );
        table.setCounter( -1 );
        for ( auto i = m_script.args.begin( ); i != m_script.args.end( ); i++ )
        {
            table.insert( *i );
        }
        
        setglobal( "arg" );
        m_init = true;
    }

    Object::Object( )
    : m_call( NULL ), m_data( this ), m_global( false ), m_runners( NULL, NULL ) 
    {
        tau::in::Female::handler( Runner::Stop, ( Object::Handler ) &Object::runnerStopped );
    }
    
    void Object::runnerStopped( tau::Grain& grain )
    {
        ENTER();
        auto& runner = dynamic_cast< Runner& >( grain );
        
        if ( &runner == assigned() )
        {
            onAssignedStop();
        }
        else
        {
            this->unmale( runner );
            onRunnerStop( runner );
        }
    }
    
    void Object::operator()( Call& call, h::Stack& stack )
    {
        m_call = &call;
        stack.runner().way()( this, &call );
        
        if ( !onProcess( m_call->name, stack ) )
        {
            assert( call.method );
            ( this->*call.method )( stack );
        }
    }
    
    void Object::push( Runner& runner ) 
    {
        h::Table table( runner );
        Main::Index( this )( table );

        auto data = ( void* ) &m_data;
        
        h::Data udata( runner );
        udata( gcStatic, data );
        
        table.setIndex( -2 );
        table.setfield( "__u" );

        table.set( "__instance", data );
        assign( runner );
    }
    
    void Object::suspend( )
    {
        runner().suspend( this );
    }
    
    void Object::resume( h::Arguments* arguments )
    {
        ENTER();
        
        runner().run( this, arguments );
    }
    
    Runner& Object::runner()
    {
        assert( current() );
        return *current();
    }
    
    int Object::gcStatic( lua_State* lua )
    {
        auto instance = Data::get( lua_touserdata( lua, lua_upvalueindex( 1 ) ) );
        instance->gc( );
        return 0;
    }

    void Object::error( const lua::Exception& e )
    {
        ENTER( );
        
        Exception exception( e );
        std::string name = this->name( );

        if ( m_call )
        {
            name = name + "." + m_call->name;
        }

        exception.message = tau::u::fprint( "%s: %s error", runner( ).id( ).c_str( ), name.c_str( ) );
        if ( e.message.length( ) )
        {
            exception.message = exception.message + ": " + e.message;
        }

        runner( ).exception( exception );
        runner( ).run( );
    }
    
    void Object::assign( Runner& runner )
    {
        m_runners.first = &runner;
        this->male( runner );
    }
    
    void Object::clear( )
    {
        tau::in::Female::clear();
        m_runners = Runners( NULL, NULL );
    }

    Runner& Main::runner( )
    {
        auto runner = Runner::create( );
        runner->females().add( *this );
        
        return *runner;
    }
    
    void Main::stop()
    {
        ENTER();
        dispatch( Close, *this );
    }
    
    void Main::runnerStopped( tau::Grain& grain )
    {
        ENTER();
        auto& runner = dynamic_cast< Runner& >( grain );
        m_success = m_success & runner.success();
    }
    
    void Main::timer( tau::Grain& grain )
    {
        State::gc();
        ( dynamic_cast< tau::base::Timer& >( grain ) ).deref();
    }
    
    void Runner::init( )
    {        
        if ( m_start )
        {
            main().init( );

            m_start->push( *this );
            m_start->destroy( );
            m_start = NULL;
        }
    }
    Runner* Runner::create( )
    {
        auto runner = dynamic_cast < Runner* > ( tau::get( typeid( Runner ), [  ] ( ) { return new Runner(  ); } ) );
        runner->start( );
        
        return runner;
    }

    void Runner::run( Object* object, h::Arguments* arguments )
    {
        ENTER( );
        
        if ( skip( object ) )
        {
            TRACE( "skipping thread resume, was suspended by object 0x%x, resume called by object 0x%x", m_way.object, object );
            return;
        }
        
        TRACE("resumed by object 0x%x, state: %d, start: 0x%x, running %d", object, m_status, m_start, running() );
        
        if ( errors() || stopped() )
        {
            return; 
        }
        
        init();
        setStatus( Running );
        
        int result = 0;
        try
        {
            result = resume( h::Arguments::push( *this, arguments ) );
        }
        catch ( const Exception& e )
        {
            setStatus( Error );
            ERROR(  "%s", e.message.c_str() );
        }
        
        if ( result == LUA_YIELD )
        {
            TRACE( "runner 0x%x yielded", this );
        }
        else
        {
            stop();
        }
    }
    
    void Runner::destroy( )
    {
        ENTER( );
        tau::ie::Tok::destroy( );
        Main::get().gc();
    }
    
    void Runner::Way::operator()( Object* _object, Object::Call* _call )
    {
        object = _object;
        call = _call;
        object->setRunner( runner );
    }
    
    Runner::Runner( )
    : m_start( NULL ), m_status( Stopped ), m_exception( NULL ), m_data( this ), m_way( *this )
    {
        ENTER();
        
        tau::in::Female::handler( Main::Close, ( Runner::Handler ) &Runner::mainStopped );
        tau::in::Female::handler( tau::base::Timer::Timeout, ( Runner::Handler ) &Runner::timer );
    }
    
    void Runner::next()
    {
        assert( suspended() );
        tau::base::event( this )( );
    }
    
    bool Runner::skip( Object* object ) const
    {
        if ( object && way().object )
        {
            return object != way().object;
        }
        
        return false;
    }
   
    void Runner::start( )
    {
        ENTER( );
        
        assert( !started() );
        
        if ( !m_lua )
        {
            m_lua = main().thread();
        }

        State::setdata( &m_data );
        
        females().add( main() );
        main().females().add( *this );
        
        m_reference = main().reference( );
        main().pop( 1 );
        
        setStatus( Started );
    } 
    
    void Runner::stop(  )
    {
        ENTER();
        
        if ( status() != Error )
        {
            setStatus( Stopped );
        }

        
        dispatch( Stop, *this );
        deref();
    }
    
    void Runner::timer( tau::Grain& grain )
    {
        run();
        ( dynamic_cast< tau::Rock& >( grain ) ).deref();
    }
    
    void Runner::cleanup(  )
    {
        ENTER();
        
        if ( m_lua )
        {
            m_lua = NULL;
            main().unref( m_reference );
            m_reference = 0;
            m_way.clear();
            m_status = Stopped;
            
            
            clear();
        }
    }
    
    bool Runner::errors( )
    {
        if ( m_exception )
        {
            auto& error = *types::Table::get();

            Exception exception( *m_exception );
            
            delete m_exception;
            m_exception = NULL;
            
            error.set( "error", types::String::get( exception.message ) );
            h::Arguments arguments;
            arguments.add( error );

            run( NULL, &arguments );
            error.destroy();
            
            return true;
        }

        return false;
    }
    
    void Runner::clear( )
    {
        ENTER();
        
        main().females().remove( *this );
        females().clear();
        
        if ( m_exception )
        {
            delete m_exception;
            m_exception = NULL;
        }
        
        if ( m_start )
        {
            m_start->destroy( );
            m_start = NULL;
        }
    }
}
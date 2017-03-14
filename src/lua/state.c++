#include "state.h"
#include "helpers.h"

namespace lua
{
    std::string State::traceback( ) const
    {
        h::Table table( *this, "debug" );
        table.field( "traceback" );
        table.setPop( false );
        State::call( 0, 1 );

        h::Stack stack( *this );
        stack.setIndex( -1 );
        return stack.string( );
    }
    
    void State::execute( const std::string& script ) const
    {
        load( script );
        resume();
    }
    
    void State::load( const Script& script ) const
    {
        check( [ & ] () { return luaL_loadfile( m_lua, script.name.c_str() ); } );
    }
    
    void State::load( const std::string& script ) const
    {
        check( [ & ] () { return luaL_loadstring( m_lua, script.c_str() ); } );
    }
    
    template< class Check > void State::check( Check check ) const
    {
        int result = check( );
        if ( result )
        {
            h::Stack stack( *this );
            throw Exception( stack.string( ).c_str( ) );
        }
    }
    
    lua_State* State::create()
    {
        auto lua = luaL_newstate();
        luaL_openlibs( lua );
        return lua;
    }
    
    std::string State::objectid( int index ) const
    {
        global( "_tostring" );
        pushvalue( index - 1 );
        call( 1, 1 );

        std::string id;
        {
            h::Stack stack( *this, 1 );
            id = stack.string();
        }
        
        std::string::size_type space = id.find( " " );
        if ( space != std::string::npos )
        {
            id = id.substr( space + 1 );
        }
        
        return id;
    }
    
    void State::pushReference( unsigned int reference ) const
    {
        grow( [ & ]( ){ lua_rawgeti( m_lua, LUA_REGISTRYINDEX, reference ); } );
    }
    
    int State::error( const std::string& message ) const
    {
        grow( [ & ]( ){ lua_pushstring( m_lua, message.c_str( ) ); } );
        return lua_error( m_lua );
    }

    void State::table( ) const
    {
        grow( [ & ]( ){ lua_newtable( m_lua ); } );
    }

    void State::pushvalue( int index ) const
    {
        grow( [ & ]( ) { lua_pushvalue( m_lua, index ); } );
    }
    void State::global( const std::string& name ) const
    {
        grow( [ & ]( ) { lua_getglobal( m_lua, name.c_str( ) ); } );
    }
    
    void State::userdata( ) const
    {
        grow( [ & ]( ){ lua_newuserdata( m_lua, 1 ); } );
    }
    void State::getfield( int index, const std::string& name ) const
    {
        grow( [ & ]( ){ lua_getfield( m_lua, index, name.c_str( ) ); } );
    }
    
    bool State::toboolean( int index ) const
    {
        bool result = false;
        if ( lua_isboolean( m_lua, index ) )
        {
            result = lua_toboolean( m_lua, index ); 
        }
        
        return result;
    }
    
    std::string State::tostring( int index, bool force ) const
    {
        if ( force )
        {
            if ( !lua_isstring( m_lua, index ) )
            {
                return std::string( );
            }
        }

        return lua_tostring( m_lua, index );
    }
    
    tau::Pill State::topill( int index ) const
    {
        tau::Pill pill;
        if ( type( index ) == String  )
        {
            size_t length;
            const char* data = lua_tolstring( m_lua, index, &length );
            pill.set( data, length );
        }
        
        return pill;
    }
    
    void* State::touserdata( int index ) const
    {
        void* data = NULL;
        if ( lua_islightuserdata( m_lua, index ) )
        {
            data = lua_touserdata( m_lua, index );
        }
        
        return data;
    }
    
    long State::tonumber( int index ) const
    {
        long number = 0;
        if ( lua_isnumber( m_lua, index ) )
        {
            number = lua_tonumber( m_lua, index );
        }
        
        return number;
    }
    
    int State::tointeger( int index ) const
    {
        int integer = 0;
        if ( lua_isnumber( m_lua, index ) )
        {
            integer = lua_tointeger( m_lua, index );
        }
        
        return integer;
    }
    
    void State::push( void* data ) const
    {
        grow( [ & ] ( ) {
            if ( data )
            {
                lua_pushlightuserdata( m_lua, data );
            }
            else
            {
                lua_pushnil( m_lua );
            }
        } );
    }
    
    void State::push( lua_CFunction value, unsigned int upvalues ) const
    {
        grow( [ & ] ( ) {
            if ( upvalues )
            {
                lua_pushcclosure( m_lua, value, upvalues );
            }
            else
            {
                lua_pushcfunction( m_lua, value );
            }
        } );
    }
    
    void State::push( const char* data, unsigned int length ) const
    {
        grow( [ & ] ( ) {
            if ( data )
            {
                lua_pushlstring( m_lua, data, length );
            }
            else
            {
                lua_pushnil( m_lua );
            }
        } );
    }
    
    void State::push( long number ) const
    {
        grow( [ & ] ( ) { lua_pushnumber( m_lua, number ); } );
    }
    
    void State::push( int number ) const
    {
        grow( [ & ] ( ) { lua_pushinteger( m_lua, number ); } );
    }
    void State::push( bool value ) const
    {
        grow( [ & ] ( ) { lua_pushboolean( m_lua, value ); } );
    }
    
    int State::resume( unsigned int count ) const
    {
        int result = lua_resume( m_lua, count );
        
        if ( result > 1 )
        {
            h::Stack stack( *this );
            stack.setIndex( -1 );

            throw Exception( stack.string() + "\n" + traceback() );
        } 
        
        return result;
    }
    
}

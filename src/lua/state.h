#ifndef VEGA_STATE_H
#define	VEGA_STATE_H

#include <lua.hpp>

#include "../common.h"
#include "tau/liner.h"

namespace lua
{
    enum Type
    {
        Table = LUA_TTABLE, 
        String = LUA_TSTRING,
        Number = LUA_TNUMBER,
        Boolean = LUA_TBOOLEAN,
        Function = LUA_TFUNCTION,
        Nil = LUA_TNIL
    };
    
    struct Script
    {
        typedef std::list< std::string > Arguments;
        
        std::string name;
        Arguments args;

        Script( const std::string _name, const Arguments _args )
        : name( _name ), args( _args )
        {
        }

        Script( const Script& script )
        {
            name = script.name;
            args = script.args;
        }
    };
    
    class Runner;
    class Main;
    class Object;

    struct Exception
    {
        std::string message;
        Runner* runner;
        
        Exception( const char *format, ... );
        Exception( const std::string& error )
        : runner( NULL ), message( error )
        {
            
        }
        Exception( Runner* runner = NULL )
        : runner( runner )
        {
        }
        
        Exception( const Exception& e )
        : message( e.message ), runner( e.runner )
        {
            
        }
        
        ~Exception();
    };
    
    class State
    {
        friend class Main;
        
    public:
        operator lua_State*( ) const 
        {
            assert( m_lua );
            return m_lua;
        }
        
        virtual ~State()
        {
        }
        
        template< class Push > void grow( Push push ) const
        {
            lua_checkstack( m_lua, 1 );
            push();
        }
        
        void pushReference( unsigned int reference ) const;
        void push( lua_CFunction function, unsigned int upvalues = 0 ) const;
        void push( void* data ) const;        
        void push( const tau::Pill& pill ) const
        {
            push( pill.data(), pill.length() );
        }
        void push( const char* data, unsigned int length ) const;
        void push( const std::string& value ) const
        {
            push( value.c_str(), value.size() );
        }
        void push( long number ) const;
        void push( int number ) const;
        void push( bool value ) const;
        
        unsigned int reference( int index = -1 ) const
        {
            pushvalue( index );
            return luaL_ref( m_lua, LUA_REGISTRYINDEX );
        }
        
        void pop( unsigned int count ) const
        {
            lua_pop( m_lua, count );
        }
        
        void unref( unsigned int reference ) const
        {
            luaL_unref( m_lua, LUA_REGISTRYINDEX, reference );
        }
                
        int error( const std::string& message ) const;
        void table() const;
        void userdata() const;
        void pushvalue( int index ) const;
        
        void load( const Script& script ) const;
        void load( const std::string& string ) const;
        
        std::string objectid( int index ) const;
        
        void global( const std::string& name ) const;
        void setglobal( const std::string& name ) const
        {
            lua_setglobal( m_lua, name.c_str() );
        }
        
        void remove( int index ) const
        {
            lua_remove( m_lua, index );
        }
                
        void gc() const
        {
            ENTER( );
            lua_gc( m_lua, LUA_GCCOLLECT, 0 );
        }
        
        std::string traceback() const;

        int top() const
        {
            return lua_gettop( m_lua );
        }
        
        void* touserdata( int index ) const;
        std::string tostring( int index, bool force = false ) const;
        tau::Pill topill( int index ) const;
        bool toboolean( int index ) const;
        long tonumber( int index ) const;
        int tointeger( int index ) const;
        
        Type type( int index ) const
        {
            return ( Type ) lua_type( m_lua, index );
        }
        
        
        int resume( unsigned int count = 0 ) const;

        void setdata( void* data ) const
        {
            lua_setdata( m_lua, data );
        }
        void* getdata() const
        {
            return lua_getdata( m_lua );
        }
        
        void getfield( int index, const std::string& name ) const;
        void setmetatable( int index ) const
        {
            lua_setmetatable( m_lua, index );
        }
        void setfield( const std::string& name, int index ) const
        {
            lua_setfield( m_lua, index, name.c_str() );
        }
        void settable( int index ) const
        {
            lua_settable( m_lua, index );
        }
        
        void insert( int index ) const
        {
            lua_insert( m_lua, index );
        }
        
    protected:
        State( lua_State* lua = NULL )
        : m_lua( lua )
        {
        }
        
        void call( unsigned int results = 0, unsigned int args = 0) const
        {
            lua_call( m_lua, results, args );
        }
        
        void execute( const std::string& script ) const; 
        
    private:
        static lua_State* create();
        template< class Check > void check( Check check ) const;
        
    protected:
        lua_State* m_lua;
        
    private:    
        std::string m_script;
    };
    

}

#endif	

    
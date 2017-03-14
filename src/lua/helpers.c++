#include "helpers.h"    
#include "types.h"
#include "main.h"

namespace lua
{
    namespace h   
    {
        void Data::operator()( lua_CFunction gc, void* data )
        {
            Lua::dec();
            Table table( m_lua );
            Arguments upvalues;
            upvalues.add( data );
            table.set( "__gc", gc, upvalues );
            
            m_lua.setmetatable( Lua::index() );
        }
        
        Table::Table( Stack& stack, int index )
        : Lua( stack.lua( ) )
        {
            init( false, 0, index, &stack );
        }
            
        bool Table::is( ) const
        {
            return lua_istable( m_lua, m_index );
        }

        Table::Strings Table::values( )
        {
            Strings result;
            if ( !is( ) )
            {
                return result;
            }

            get( );

            lua_pushnil( m_lua );

            unsigned index = Lua::index();
            index--;

            while ( lua_next( m_lua, index ) != 0 )
            {
                std::string key = m_lua.tostring( -2 );
                std::string  value = m_lua.tostring( -1 );
                if ( value.length() )
                {
                    result[ key ] = value;
                }
                
                m_lua.pop( 1 );
            }

            return result;
        }

        void Table::init( bool pop, unsigned int ref, int index, Stack* stack )
        {
            m_stack = stack;
            Lua::setPop( pop );
            m_create = !Lua::getPop();
            Lua::setIndex( index );
            m_counter = 1;
            m_ref = ref;
            m_loaded = !global( );
        }
        
        void Table::create( )
        {
            if ( global( ) )
            {
                get( );
                return;
            }

            if ( m_create )
            {
                m_lua.table();
                m_create = false;
            }
        }

        void Table::get( )
        {
            if ( m_stack || m_loaded )
            {
                return;
            }

            if ( !global( ) )
            {
                return;
            }

            m_loaded = true;

            if ( m_ref )
            {
                m_lua.pushReference( m_ref );
            }
            else
            {
                m_lua.global( m_name );

                if ( m_lua.type( -1 ) == Nil )
                {
                    m_lua.pop( 1 );
                    m_lua.table();
                    m_lua.setglobal( m_name );
                    m_lua.global( m_name );
                }
            }
        }
        
        void Table::field( const std::string& name )
        {
            field( name, [](){}, false );
        }
        
        template< class Field > void Table::field( const std::string& name, Field field, bool pop )
        {
            get(); 
            assert( is() );
            if ( !is() )
            {
                return;
            }

            m_lua.getfield( Lua::index(), name );
            field();
            
            if ( pop )
            {
                m_lua.pop( 1 );
            }
        }
        
        template < class Set > void Table::setfield( const std::string name, Set set )
        {
            create( );
            set( );
            m_lua.setfield( name, -2 );
        }

        bool Table::boolean( const std::string& name )
        {
            bool result = false;
            field( name, [ & ](){ result = m_lua.toboolean( -1 ); } );
            return result;
        }

        std::string Table::string( const std::string& name )
        {
            std::string result;
            field( name, [ & ](){ result = m_lua.tostring( -1, true ); } );
            return result;
        }

        void* Table::data( const std::string& name )
        {
            void* result = NULL;
            field( name, [ & ](){ result = m_lua.touserdata( -1 ); } );
            return result;
        }

        void Table::insert( const std::string& value, int index )
        {
            create( );

            bool inc = false;
            if ( !index )
            {
                index = m_counter;
                inc = true;
            }
            m_lua.push( ( int ) index );
            m_lua.push( value );
            
            m_lua.settable( -3 );

            if ( inc )
            {
                m_counter++;
            }
        }

        void Table::setReference( const std::string& name, unsigned int ref )
        {
            setfield( name, [ & ]( ) {
                m_lua.pushReference( ref );
            } );
        }

        void Table::set( const std::string& name, Table& table )
        {
            table.get( );
            
            setfield( name, [ & ]( ) {
                m_lua.insert( -2 );
            } );
            
            table.setPop( false );
        }

        void Table::set( const std::string& name, lua_CFunction value, const Arguments& upvalues )
        {   
            setfield( name, [ & ]( ) {
                m_lua.push( value, upvalues.push( m_lua ) );
            } );
        }

        void Table::set( const std::string& name, void* value )
        {
            setfield( name, [ & ]( ) {
                m_lua.push( value );
            } );
        }

        void Table::set( const std::string& name, const std::string& value )
        {
            setfield( name, [ & ]( ) {
                m_lua.push( value.c_str(), value.length() );
            } );
        }

        void Table::set( const std::string& name, unsigned int value )
        {
            setfield( name, [ & ]( ) {
                m_lua.push( ( int ) value );
            } );            
        }

        void Table::setmetatable( Table& table )
        {
            table.get();
            
            if ( m_create )
            {
                create( );
                m_lua.insert( -2 );
            }
            
            m_lua.setmetatable( -- m_index );
        }
        
        Stack::Stack( Runner& runner )
        : Lua( runner ),  m_runner( &runner ), m_count( 0 )
        {
            init( 0 );
        }
        
        void Stack::init( unsigned int top )
        {
            Lua::setCount( top ? top : m_lua.top( ) );
            Lua::setIndex( -Lua::count() );
            Lua::setPop( count() > 0 );
        }

        long Stack::number( )
        {
            unsigned int result = 0;
            get( [ & ]( ){ result = m_lua.tonumber( index() ); } );
            return result;
        }

        int Stack::integer( )
        {
            int result = 0;
            get( [ & ]( ){ result = m_lua.tointeger( index() ); } );
            return result;
        }

        void* Stack::pointer( )
        {
            void* result = NULL;
            get( [ & ]( ){ result = m_lua.touserdata( index() ); } );
            return result;
        }
        
        template < class Get > void Stack::get( Get get )
        {
            if ( !index() )
            {
                return;
            }
            
            get();
            inc();
        }

        Table Stack::table( )
        {
            Table table( *this, m_index );
            inc( );
            return table;
        }

        tau::Pill Stack::data(  )
        {
            tau::Pill result;
            get( [ & ]( ){ result = m_lua.topill( index() ); } );
            return result;
        }

        std::string Stack::string( )
        {
            std::string result;
            get( [ & ]( ){ result = m_lua.tostring( index(), true ); } );
            return result;
        }
        types::Value* Stack::value( )
        {
            types::Value* value = types::Value::load( m_lua, index( ), false );
            inc( );
            return value;
        }
        unsigned int Stack::reference( )
        {
            unsigned int result = m_lua.reference( index() );
            inc();
            return result;
        }
        bool Stack::boolean( )
        {
            bool result = false;
            get( [ & ]( ){ result = m_lua.toboolean( index() ); } );
            return result;
        }
        
        void Stack::push( Table& table )
        {
           table.get();
           pushvalue( [](){} );
        }

        void Stack::push( const char* data, unsigned int length )
        {
            pushvalue( [ & ]( ) 
            {
                m_lua.push( data, length );
            } );
        }

        void Stack::push( void* data )
        {
            pushvalue( [ & ]( ) 
            {
                m_lua.push( data );
            } );
        }

        void Stack::push( long number )
        {
            pushvalue( [ & ]( ) 
            {
                m_lua.push( number );
            } );
        }

        void Stack::push( int number )
        {
            pushvalue( [ & ]( ) 
            {
                m_lua.push( number );
            } );
        }

        void Stack::push( bool value )
        {
            pushvalue( [ & ]( ) 
            {
                m_lua.push( value );
            } );
        }

        void Stack::push( Object& object )
        {
            pushvalue( [ & ]( ) 
            {
                object.push( runner() );
            } );
        }

        void Stack::pushReference( unsigned int reference )
        {
            pushvalue( [ & ]( ) 
            {
                m_lua.pushReference( reference );
            } );
        }
        
        template < class Push > void Stack::pushvalue( Push push )
        {
            pop( );
            push( );
            m_count++;
        }

        void Stack::push( const types::Value& value )
        {
            pushvalue( [ & ]( ) {
                value.push( m_lua );
            } );
        }
        
        
        Type Stack::type( ) const
        {
            unsigned int type = 0;
            if ( index() )
            {
                type = m_lua.type( index() );
            }
            
            return ( Type) type;
        }
    
        void Arguments::add( const tau::Pill& pill )
        {
            Argument arg( Pill );
            arg.pill = &pill;
            m_list.emplace_back( arg );
        }

        void Arguments::add( unsigned int number )
        {
            Argument arg( Number );
            arg.number = number;
            m_list.emplace_back( arg );
        }

        void Arguments::addReference( unsigned int ref )
        {
            Argument arg( Reference );
            arg.number = ref;
            m_list.emplace_back( arg );
        }
        
        void Arguments::add( void* pointer )
        {
            Argument arg( Pointer );
            arg.pointer = pointer;
            m_list.emplace_back( arg );
        }

        void Arguments::add( Object& object )
        {
            Argument arg( Obj );
            arg.object = &object;
            m_list.emplace_back( arg );
        }
        
        void Arguments::add( const types::Value& value )
        {
            Argument arg( Value );
            arg.value = &value;
            m_list.emplace_back( arg );
        }
        
        unsigned int Arguments::push( const State& lua ) const
        {
            for ( auto i = m_list.begin( ); i != m_list.end( ); i++ )
            {
                auto argument = *i;
                switch ( argument.type )
                {
                    case Pointer:
                        lua.push( argument.pointer );
                        break;
                        
                    case Pill:
                        lua.push( argument.pill->data( ), argument.pill->length( ) );
                        break;

                    case Number:
                        lua.push( argument.number );
                        break;

                    case Nil:
                        lua.push( NULL );
                        break;

                    case Reference:
                        lua.pushReference( argument.number );
                        break;

                    case Obj:
                    {
                        auto object = argument.object;
                        object->push( runner() );
                    }
                        break;

                    case Value:
                        argument.value->push( lua );
                        break;
                }
            }
            
            return m_list.size( );
        }
        
        unsigned int Arguments::push( Runner& runner, Arguments* arguments )
        {
            if ( arguments )
            {
                arguments->setRunner( runner );
            }
            return arguments ? arguments->push( runner ) : 0;
        }
    }
}
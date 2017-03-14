
#include "types.h"
#include "helpers.h"


namespace lua
{
    namespace types
    {                
        void Value::dump( tau::Pill& pill, const Value& value )
        {
            unsigned int offset = sizeof( Header );
            
            pill.setOffset( offset );
            value.dump( pill );
            
            pill.setOffset( 0 );
            Header header( pill.length() );
            
            pill.cbuffer().copy( 0, ( const char* ) &header, offset );
            pill.inc( offset );
        }

        void Value::dump( tau::Pill& pill ) const
        {
            pill.add(  &m_type, sizeof( m_type ) );
            data( pill );
        }

        Value* Value::create( tau::Pill& pill ) 
        {
            auto type = *pill.contents();
            auto value = dynamic_cast< Value* >( tau::grain( type ) );
            
            pill.move( sizeof( type ) );
            if ( value )
            {
                value->setType( type );
                value->init( pill );
            }
            
            return value;
        }
        
        const tau::Grain::Generators& Value::populate()
        {            
            ENTER();
            
            tau::add( LUA_TBOOLEAN, ( tau::Grain::Generator ) &Simple::create );
            tau::add( LUA_TNUMBER, ( tau::Grain::Generator ) &Simple::create );
            tau::add( LUA_TSTRING, ( tau::Grain::Generator ) &String::create );
            tau::add( LUA_TFUNCTION, ( tau::Grain::Generator ) &Function::create );
            tau::add( LUA_TTABLE, ( tau::Grain::Generator ) &Table::create );
                    
            return *tau::generators( LUA_TBOOLEAN );
        }

        Value* Value::load( tau::Pill& pill ) 
        {
            if ( pill.length() < sizeof( Header ) )
            {
                return NULL;
            }
            
            Header* header = ( Header* ) pill.contents();
            if ( pill.length() < sizeof( Header ) + header->size )
            {
                return NULL;
            }

            pill.move( sizeof( Header ) );
            auto value = create( pill );
            pill.setOffset( 0 );
            return value;
        }
        
        Function::~Function( )
        {
            cleanup();
        }
        
        void Function::cleanup()
        {
            for ( auto i = m_upvalues.begin( ); i != m_upvalues.end( ); i++ )
            {
                ( *i )->destroy();
            }
            
            m_upvalues.clear();
            m_script = NULL;
            m_reference = 0;
            m_buffer.clear();
        }
        
        void Table::cleanup()
        {
            for ( auto i = m_map.begin( ); i != m_map.end( ); i++ )
            {
                i->first->destroy();
                i->second->destroy();
            }
            
            m_map.clear();

            if ( m_mt )
            {
                m_mt->destroy();
                m_mt = NULL;
            }
        }

        Table::~Table( )
        {
            cleanup();
        }

        void Function::push( const State& lua ) const
        {
            if ( m_reference )
            {
                lua.pushReference( m_reference );
                return;
            }
            
            if ( m_script )
            {
                lua.load( *m_script );
                return;
            }
            

            lua.grow([ & ] ( ) { lua_load( lua, readerStatic, ( void* ) this, "" ); } );
            
            assert( lua_isfunction( lua, -1 ) );
            unsigned int count = 1;
            
            for ( auto i = m_upvalues.begin( ); i != m_upvalues.end( ); i++ )
            {
                ( *i )->push( lua );
                assert( lua_setupvalue( lua, -2, count ) );
                count++;
            }
        }

        void Table::push( const State& lua ) const
        {
            lua.table();

            for ( auto i = m_map.begin( ); i != m_map.end( ); i++ )
            {
                i->first->push( lua );
                i->second->push( lua );
                lua_settable( lua, -3 );
            }

            if ( m_mt )
            {
                m_mt->push( lua );
                lua_setmetatable( lua, -2 );
            }
        }

        void String::init( tau::Pill& pill )
        {
            auto length = *( unsigned int* ) pill.contents();
            auto size = sizeof( length );
            m_value.append( pill.contents() + size, length );
            pill.move( m_value.length() + size );
        }

        void String::data( tau::Pill& pill ) const
        {            
            unsigned int size = m_value.size( );
            pill.add( ( char* ) &size, sizeof( size ) );
            pill.add( m_value.data( ), m_value.size( ) );
        }

        void Simple::init( tau::Pill& pill )
        {
            m_value = *( long* ) pill.contents();
            pill.move( sizeof( m_value ) );
        }
        
        void Simple::data( tau::Pill& pill ) const
        {
            pill.add( ( char* ) &m_value, sizeof( m_value ) );
        }
        
        void Function::data( tau::Pill& pill ) const
        {
            unsigned int size = m_buffer.size( );
            pill.add( ( char* ) &size, sizeof( size ) );
            pill.add( m_buffer );

            size = m_upvalues.size();
            pill.add( ( char* ) &size, sizeof( size ) );
                    
            for ( auto i = m_upvalues.begin( ); i != m_upvalues.end( ); i++ )
            {
                auto value = *i;
                value->dump( pill );
            }
        }

        void Function::init( tau::Pill& pill )
        {
            auto size = *( unsigned int* ) pill.contents();
            auto offset = sizeof( size );
            
            m_buffer.add( pill.contents() + offset, size );
            offset += m_buffer.size( );
            unsigned int upvalues = *( unsigned int* ) ( pill.contents() + offset );
            offset += sizeof( unsigned int );
            
            pill.move( offset );
            
            while( m_upvalues.size() < upvalues )
            {
                m_upvalues.push_back( Value::create( pill ) );
            }
        }

        void Function::init( const State& lua, int index )
        {
            if ( lua_iscfunction( lua, index ) || !lua_isfunction( lua, index ) )
            {
                throw Exception( "cannot load function" );
            }
            
            if ( index != -1 )
            {
                lua.pushvalue( index );
            }
                
            lua_dump( lua, writerStatic, ( void* ) this );
            
            if ( index != -1 )
            {
                lua.pop( 1 );
            }

            unsigned int counter = 1;
            setId( lua, index );
            while ( true )
            {
                const char* name = lua_getupvalue( lua, index, counter );
                if ( !name )
                {
                    break;
                }
                
                Value* upvalue = NULL;
                if ( Value::check( lua.objectid( -1 ) ) )
                {
                    lua.pop( 1 );
                    continue;
                }
                
                upvalue = load( lua );
                if ( upvalue )
                {
                    m_upvalues.push_back( upvalue );
                }
                counter ++;
            }
        }

        bool Function::operator==( const Value& value ) const
        {
            const Function& function = dynamic_cast < const Function& > ( value );
            if ( m_buffer == function.m_buffer && m_upvalues.size() == function.m_upvalues.size() )
            {
                for ( int i = 0; i < m_upvalues.size(); i++ )
                {
                    auto left = m_upvalues[ i ];
                    auto right = function.m_upvalues[ i ];
                    
                    if ( !( *left == *right ) )
                    {
                        return false;
                    }
                }
                
                return true;
            }

            return false;
        }

        void Table::data( tau::Pill& pill ) const
        {
            unsigned int size = m_map.size( );
            
            pill.add( ( char* ) &size, sizeof( size ) );
            
            for ( auto i = m_map.begin( ); i != m_map.end( ); i++ )
            {
                i->first->dump( pill );
                i->second->dump( pill );
            }
            if ( m_mt )
            {
                m_mt->dump( pill );
            }
            else
            {
                Value value;
                value.dump( pill );
            }
        }

        Table::Values Table::parseTable( tau::Pill& pill )
        {
            auto count = *( unsigned int* ) pill.contents();
            pill.move( sizeof( count ) );

            Values map;

            for ( unsigned int i = 0; i < count; i++ )
            {
                
                Value* key = NULL;
                Value* value = NULL;

                while ( true )
                {
                    auto loaded = Value::create( pill );
                    
                    if ( !loaded )
                    {
                        assert( false );
                        break;
                    }
                    
                    if ( !key )
                    {
                        key = loaded;
                    }
                    else if ( !value )
                    {
                        value = loaded;
                    }

                    if ( key && value )
                    {
                        map[ key ] = value;
                        break;
                    }
                }
            }

            return map;
        }

        void Table::init( tau::Pill& pill )
        {
            m_map = parseTable( pill );
            m_mt = dynamic_cast< Table* >( Value::create( pill ) );
        }

        void Table::init( const State& lua, int index )
        {
            setId( lua, index );
            unsigned int mt = 0;
            if ( lua_getmetatable( lua, index ) )
            {
                m_mt = dynamic_cast < Table* > ( load( lua, -1, false ) );
                {
                    h::Stack stack( lua, 1 );
                    mt = stack.reference();
                }
                
                lua_newtable( lua );
                lua_setmetatable( lua, index - 1 );
            }

            lua_pushnil( lua );

            while ( lua_next( lua, index - 1 ) != 0 )
            {
                if ( Value::check( lua.objectid( -1 ) ) )
                {
                    lua.pop( 1 );
                    continue;
                }
                
                Value* value = load( lua, -1, true );
                Value* key = load( lua, -1, false );
                m_map[ key ] = value;
            }

            if ( mt )
            {
                lua.pushReference( mt );
                lua_setmetatable( lua, index - 1 );
                lua.unref( mt );
            }
        }

        bool Table::operator==( const Value& value ) const
        {
            const Table& map = dynamic_cast < const Table& > ( value );
            
            if ( m_map.size( ) != map.m_map.size( ) )
            {
                return false;
            }

            for ( auto i = m_map.begin( ); i != m_map.end( ); i++ )
            {
                auto second = ( const_cast< Table* >( &map ) )->value( i->first->tostring() );
                if ( !second )
                {
                    return false;
                }
                
                auto value = *i->second;
                if ( !( value == *second ) )
                {
                    return false;
                }
            }
            
            if ( m_mt )
            {
                if ( !map.m_mt )
                {
                    return false;
                }
                else
                {
                    return *m_mt == *map.m_mt;
                }
            }
            
            return true;
        }
        
        void Table::index()
        {
            
            if ( m_strings.size() )
            {
                return;
            }
            
            for ( auto i = m_map.begin(); i != m_map.end(); i++ )
            {
                auto key = i->first->tostring();
                auto value = i->second;
                
                m_strings[ key ] = i->second;
            }
            
            std::string empty;
            empty.append( "\0", 5 );
            m_strings[ empty ] = NULL;
        }

        const Value* Table::value( const std::string& key )
        {
            index();
            auto found = m_strings.find( key );
            if ( found != m_strings.end() )
            {
                return found->second;
            }
            
            return NULL;
        }
        
        bool Value::check( const std::string& id ) const
        {
            if ( m_id == id )
            {
                return true;
            }
            
            if ( m_parent )
            {
                return m_parent->check( id );
            }
            
            return false;
        }
        
        Value* Value::load( const State& lua, int index, bool pop, const Value* parent )
        {
            unsigned int type = lua_type( lua, index );
            auto value = dynamic_cast< Value* >( tau::grain( type ) );
                        
            try
            {
                if ( !value )
                {
                    throw Exception();
                }

                value->setParent( parent );
                value->setType(  type );
                value->init( lua, index );
                
            }
            catch( const Exception& )
            {
                
                if ( value )
                {
                    delete value;
                }
                
                value = NULL;
            }

            if ( pop )
            {
                lua.pop( 1 );
            }
            
            return value;
        }

        int Function::writerStatic( lua_State *lua, const void* buffer, size_t size, void* data )
        {
            return( reinterpret_cast < Function* > ( data ) )->writer( buffer, size );
        }

        int Function::writer( const void* data, size_t size )
        {
            m_buffer.add( ( const char* ) data, size );
            return 0;
        }

        const char* Function::readerStatic( lua_State* lua, void* data, size_t* size )
        {
            return( reinterpret_cast < Function* > ( data ) )->reader( size );
        }

        const char* Function::reader( size_t* size )
        {
            *size = m_buffer.length( );
            const char* data = m_buffer.data( );
            m_buffer.clear( );
            return data;
        }
        
        void Table::insert( const Type& pair )
        {
            m_map.insert( pair );
        }
        
        void Table::set( const std::string& key, Value* value )
        {
            insert( Type( new String( key ), value ) );
        }
     }
}

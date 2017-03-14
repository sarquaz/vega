#ifndef VEGA_TYPES_H
#define	VEGA_TYPES_H

#include "state.h"
#include <tau/common.h>


namespace lua
{
    namespace h
    {
        class Stack;
    }
    
    namespace types
    {        
        class Value : public  tau::ie::Tok
        {
            friend class Table;
            friend class Function;
            
        public:
            virtual ~Value( )
            {
            }

            virtual void push( const State& ) const
            {
            }

            char type( ) const
            {
                return m_type;
            }

            static Value* load( tau::Pill& );
            static void dump( tau::Pill& pill, const Value& value );
            static Value* load( const State& lua, int index = -1, bool pop = true, const Value* parent = NULL );
           
            virtual std::string tostring( ) const
            {
                return std::string( );
            }

            virtual bool operator ==( const Value& ) const
            {
                return true;
            }
            
            
            static const tau::Grain::Generators& populate();
            
        protected:
            virtual void init( const State& lua, int index = -1 ) 
            {
            }
            
            virtual void init( tau::Pill& pill )
            {
            }
            
            static Value* create( tau::Pill& pill );
            
            Value( unsigned int type = 0 )
            : m_type( type )
            {
            }
            
            void dump( tau::Pill& ) const;

            void setParent( const Value* parent )
            {
                m_parent = parent;
            }
            
            const Value* parent() const
            {
                return m_parent;
            }
            
            const std::string& id() const
            {
                return m_id;
            }
            
            void setId( const State& lua, int index )
            {
                m_id = lua.objectid( index );
            }
            
            bool check( const std::string& id ) const;
            
            void setType( char type )
            {
                m_type = type;
            }
            
        private:
#define TYPES_VERSION 1
            struct Header
            {
                unsigned int version;
                unsigned int size;

                Header( unsigned int _size )
                : version( TYPES_VERSION ), size( _size )
                {

                }
            };

            virtual void data( tau::Pill& ) const 
            {
            }
            
            virtual unsigned int hash( ) const
            {
                return typeid( *this ).hash_code();
            }
            
        private:
            const Value* m_parent;
            char m_type;
            std::string m_id;
        };

        typedef std::vector< Value* > Values;

        class String : public Value
        {
           
        public:
            String( const std::string& value )
            : m_value( value )
            {
            }
            
            String( const char* format, ... )
            {
                va_list args;
                va_start( args, format );
                m_value = String::format( format, args ); 
                va_end( args );
            }
            
            String( const char* format, va_list args )
            {
                m_value = String::format( format, args );
            }
            
            virtual void push( const State& lua ) const
            {
                lua.push( m_value );
            }
            
            virtual bool operator==( const Value& value ) const
            {
                return m_value == ( dynamic_cast < const String& > ( value ) ).m_value;
            }

            virtual std::string tostring( ) const
            {
                return m_value;
            }
            
            static std::string format( const char* format, va_list args )
            {
                char buffer[ 1024 ];
                ::vsnprintf( buffer, sizeof ( buffer ), format, args );
                return std::string( buffer );
            }
            
            static tau::Grain* create()
            {
                return dynamic_cast< tau::Grain* >( tau::line().tok( typeid( String ), [](){ return new String(); } ) );
            }
            
            static String* get( const std::string& value = " ")
            {
                auto string = dynamic_cast< String* >( create() );
                string->setValue( value );
                return string;
            }

        private:
            String()
            : Value( LUA_TSTRING )
            {
            }
            
            virtual void init( const State& lua, int index = -1 )
            {
                m_value = lua_tostring( lua, index );
            }
            
            virtual void init( tau::Pill& );
            void data( tau::Pill& ) const;
            
            virtual unsigned int hash( ) const
            {
                return typeid( *this ).hash_code();
            }
            
            virtual void cleanup()
            {                
                m_value.clear();
            }
            
            void setValue( const std::string& value )
            {
                m_value = value;
            }
            
        private:
            std::string m_value;
        };
        
        class Simple : public Value
        {
            
        public:            
            virtual void push( const State& lua ) const
            {
                ( Value::type() == LUA_TBOOLEAN ) ? lua.push( ( bool ) m_value ) : lua.push( ( long ) m_value );
            }
            
            virtual bool operator==( const Value& value ) const
            {
                return m_value == ( dynamic_cast < const Simple& > ( value ) ).m_value;
            }
            
            static tau::Grain* create()
            {
                return dynamic_cast< tau::Grain* >( tau::line().tok( typeid( Simple ), [](){ return new Simple(); } ) );
            }
            

        private:
            Simple(  )
            : m_type( 0 ), m_value( 0 )
            {
            }
            
            virtual void init( const State& lua, int index = -1 )
            {
                assert( Value::type() );
                m_value = ( Value::type() == LUA_TNUMBER ) ? lua_toboolean( lua, index ) : lua_tonumber( lua, index );
            }
            
            virtual void init( tau::Pill& );
            virtual void data( tau::Pill& ) const;
            
            virtual unsigned int hash( ) const
            {
                return typeid( *this ).hash_code();
            }
            
            virtual void cleanup()
            {
                m_value = 0;
            }

        private:
            long m_value;
            char m_type;
        };

        class Function : public Value
        {
        public:
            Function( const Function& value )
            : m_reference( value.m_reference ), m_script( value.m_script )
            {
                Value::setType( value.type() );
            }
            
            virtual void push( const State& ) const;
            virtual void data( tau::Pill& ) const;
            virtual bool operator==( const Value& ) const;

            virtual ~Function( );
            
            static tau::Grain* create()
            {
                auto function =  dynamic_cast< tau::Grain* >( tau::line().tok( typeid( Function ), [](){ return new Function(); } ) );
                return function;
            }
            
            static Function* get( unsigned int reference = 0 )
            {
                auto function = dynamic_cast< Function* >( create() );
                function->setReference( reference );
                return function;
            }
            
            static Function* get( const Script& script )
            {
                auto function = dynamic_cast< Function* >( Function::create() );
                function->setScript( script );
                return function;
            }
            
            void setScript( const Script& script )
            {
                m_script = &script;
            }
            
            void setReference( unsigned int reference )
            {
                m_reference = reference;
            }
            
       private:
           Function( )
            : m_reference( 0 ), m_script( NULL )
            {
            }
            
            virtual void init( const State& lua, int index = -1 );
            virtual void init( tau::Pill& );
            int writer( const void* buffer, size_t size );
            const char* reader( size_t* );
            static int writerStatic( lua_State *lua, const void* buffer, size_t size, void* data );
            static const char* readerStatic( lua_State* lua, void* data, size_t* size );
            
            virtual unsigned int hash( ) const
            {
                return typeid( *this ).hash_code();
            }
            
            virtual void cleanup();

        private:
            Values m_upvalues;
            tau::Pill m_buffer;
            unsigned int m_reference;
            const Script* m_script;
            
        };

        class Table : public Value
        {
        public:
            typedef std::pair< Value*, Value* > Type;
            
            virtual ~Table( );
            virtual void push( const State& ) const;
            virtual void data( tau::Pill& ) const;
            virtual bool operator==( const Value& ) const;

            typedef std::map< Value*, Value* > Values;
            const Values& map( ) const
            {
                return m_map;
            }
            void set( const std::string& key, Value* value );
            
            static tau::Grain* create()
            {
                return dynamic_cast< tau::Grain* >( tau::line().tok( typeid( Table ), [](){ return new Table(); } ) );
            }
            
            static Table* get()
            {
                return dynamic_cast< Table* >( create() );
            }

        private:
            Table( )
            : m_mt( NULL )
            {
            }
            
            virtual void cleanup();
            virtual void init( const State& lua, int index = -1 );
            virtual void init( tau::Pill& );
            
            const Value* value( const std::string& key );
            static Values parseTable( tau::Pill& );
            void insert( const Type& );
            
            typedef std::map< std::string, Value* > Strings;
            void index();
            
            virtual unsigned int hash( ) const
            {
                return typeid( *this ).hash_code();
            }
            
        private:
            Table* m_mt;
            Values m_map;
            Strings m_strings;
        };
    }
}
#endif	


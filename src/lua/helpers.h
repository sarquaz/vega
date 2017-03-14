#ifndef HELPERS_H
#define	HELPERS_H

#include "state.h"

namespace lua
{
    class Main;
    class Object;
    class Runner;
    
    namespace types
    {
        class Value;
    }
    
    namespace h
    {
        class Arguments;
        
        class Lua
        {
        public:
            virtual ~Lua()
            {
                pop();
            }
            
            void pop()
            {
                if ( m_pop && m_count )
                {
                    m_lua.pop( m_count );
                    m_count = 0;
                    m_index = 0;
                }
            }
            
            const State& lua() const
            {
                return m_lua;
            }
            int index() const
            {
                return m_index;
            }
            void setIndex( int index )
            {
                m_index = index;
            }
            void setPop( bool pop )
            {
                m_pop = pop;
            }
            bool getPop() const
            {
                return m_pop;
            }
            unsigned int count() const
            {
                return m_count;
            }
            
        protected:
            Lua( const State& lua )
            : m_lua( lua ), m_count( 1 ), m_index( -1 ), m_pop( false )
            {
            }
            
            void dec()
            {
                m_index --;
            }
            void inc()
            {
                m_index ++;
            }
            
            void setCount( unsigned int count )
            {
                m_count = count;
            }
            
        protected:
            const State& m_lua;
            int m_index;
            bool m_pop;
            
        private:
            unsigned int m_count;
        };
        
        class Data: public Lua
        {
        public:
            Data( const State& lua )
            : Lua( lua )
            {
                lua.userdata();
            }
            
            void operator()( lua_CFunction gc, void* data );
        };

        class Stack;
        
        class Table : public Lua
        {
            friend class Main;
            friend class Runner;
            
        public:
            typedef std::map< std::string, std::string > Strings;
            
            Table( const State& lua )
            : Lua( lua )
            {
                init( false );
            }

            Table( Stack& stack, int index = -1 );
            Table( const State& lua, const std::string& name, bool pop = true )
            : Lua( lua ), m_name( name )
            {
                init( pop );
            }

            void set( const std::string& name, lua_CFunction value, const Arguments& upvalues );
            void set( const std::string& name, const std::string& value );
            void set( const std::string& name, void* value );
            void set( const std::string& name, Table& table );
            void set( const std::string& name, unsigned int value );
            void setReference( const std::string& name, unsigned int ref );

            void insert( const std::string& value, int index = 0 );

            unsigned int reference() const
            {
                return m_lua.reference( index() );
            }

            bool boolean( const std::string& name );
            std::string string( const std::string& name );
            void* data( const std::string& name );
            Strings values( );
            
            void setCounter( int counter )
            {
                m_counter = counter;
            }
            
            void setfield( const std::string& name )
            {
                m_lua.setfield( name, Lua::index() );
            }
            
            void setmetatable( Table& table );
            void field( const std::string& name );
            void get( );

        private:
            void create( );
            void init( bool pop, unsigned int ref = 0, int index = -1, Stack* stack = NULL );
            
            bool global( ) const
            {
                return m_ref || m_name.length( );
            }

            bool is( ) const;
            
            template < class Set > void setfield( const std::string name, Set set );
            template < class Field > void field( const std::string& name, Field field, bool pop = true );

        private:
            std::string m_name;
            int m_counter;
            Stack* m_stack;
            unsigned int m_ref;
            bool m_loaded;
            bool m_create;
        };

        class Stack: public Lua
        {
        public:
            Stack( const State& lua, unsigned int top = 0 )
            : Lua( lua ),  m_runner( NULL ), m_count( 0 )
            {
                init( top );
            }
            
            Stack( Runner& runner );
            virtual ~Stack( )
            { 
            }

            Type type( ) const;
            long number( );
            Table table( );
            void* pointer( );
            int integer( );
            tau::Pill data();
            std::string string( );
            bool boolean( );
            types::Value* value( );
            unsigned int reference( );

            void setTop( unsigned int top )
            {
                Lua::setCount( top );
                m_index = -top;
            }

            unsigned int top( ) const
            {
                return Lua::count();
            }
        
            void push( Table& table );
            void push( const tau::Pill& pill )
            {
                push( pill.data( ), pill.length( ) );
            }
            void push( const char* data, unsigned int length );            
            void push( const std::string& string )
            {
                push( string.data( ), string.length( ) );
            }
            void push( void* data );
            void push( long  number );
            void push( int number );
            void push( bool value );
            void push( Object& object );
            void pushReference( unsigned int reference );
            void push( const types::Value&  value );
            
            unsigned int count( ) const
            {
                return m_count;
            }

            Runner& runner() 
            {
                assert( m_runner );
                return *m_runner;
            }

            void init( unsigned int top = 0 );
            void setRunner( Runner& runner )
            {
                m_runner = &runner;
            }
            
        private:
            template < class Push > void pushvalue( Push push );
            template < class Get > void get( Get get );
            
        private:
            Runner* m_runner;
            unsigned int m_count;
        };

        class Arguments
        {
            friend class Runner;
            
        public:
            Arguments()
            : m_runner( NULL )
            {
                
            }
            
            void add( void* pointer );
            void add( const tau::Pill& buffer );
            void add( unsigned int number );
            void addReference( unsigned int ref );
            void add( Object& object );
            void add( const types::Value& value );
            
            unsigned int push( const State& lua ) const;
            
            unsigned int size() const
            {
                return m_list.size();
            }
            
            static unsigned int push( Runner& runner, Arguments* arguments );
            
        private:
            
            Runner& runner() const
            {
                assert( m_runner );
                return *m_runner;
            }
            void setRunner( Runner& runner )
            {
                m_runner = &runner;
            }
            
            enum Type
            {
                Pointer,
                Pill,
                Number,
                Nil,
                Reference,
                Obj,
                Value
            };

            struct Argument
            {
                Type type;
                const tau::Pill* pill;
                long number;
                Object* object;
                const types::Value* value;
                void* pointer;

                Argument( Type _type )
                : pill( NULL ), type( _type ), number( 0 ), object( NULL ), value( NULL ), pointer( NULL )
                {
                }

            };

        private:
            std::list< Argument > m_list;
            Runner* m_runner;
        };
    }
}
#endif	
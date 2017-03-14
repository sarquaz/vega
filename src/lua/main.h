#ifndef MAIN_H
#define	MAIN_H

#include "state.h"
#include "helpers.h"
#include "types.h"

namespace lua
{
    class Object;
    typedef void ( Object::*Method )( h::Stack& );
    
    class Main: public State, public tau::in::Female, public tau::in::Male
    {
        friend class Runner;
        friend class Object;
        
    public:
#define EVENT_MAIN 20
        enum Events
        {
            Close = EVENT_MAIN
        };
        
        Main( const Script& script );
        
        virtual ~Main()
        {
            ENTER( );
            close( );
        }
                
        void open( );
        void close();
        void stop();
        
        Runner& runner();

        const Script& script() const
        {
            return m_script;
        }
        
        void set( Object& object );
                
        static std::string name()
        {
            return s_global;
        }
                
        bool success() const
        {
            return m_success;
        }

        static Main& get();
        
        class Router
        {
        public:
            Router( const Object& object )
            : m_reference( 0 )
            {
                add( object );  
            }
            
            unsigned int reference( ) const
            {
                return m_reference;
            }

            struct Call
            {
                Method method;
                std::string name;
                unsigned int id;

                typedef std::map< const std::string, Call > Map;

                Call( const std::string& _name, Method _method, unsigned int _id  )
                : method( _method ), name( _name ), id( _id )
                {
                }

                Call( const Call& call )
                : method( call.method ), name( call.name ), id( call.id )
                {
                }
            };
            
            const Call::Map& calls() const
            {
                return m_calls;
            }
            
        private:
            static int dispatch( lua_State* lua );
            void add( const Object& object );
            
            struct Dispatch
            {
                Object* object;
                Runner* _runner;
                
                Dispatch( )
                : object( NULL ), _runner( NULL )
                {
                    
                }
                
                Runner& runner()
                {
                    assert( _runner );
                    return *_runner;
                }

                void operator()( h::Stack& stack );
                void* instance( h::Stack& );
            };
            
        private:
            unsigned int m_reference; 
            Call::Map m_calls;
        };
        
    private:
        void gc();
        
        void runnerStopped( tau::Grain& grain );
        void timer( tau::Grain& grain );
        
        void init();
        
        lua_State* thread() const
        {
            return lua_newthread( m_lua );
        }
        
        typedef std::unordered_map< unsigned int, Router* > Routers;
        
        struct Index
        {
            Routers& routers;
            Object& object;
            
            Index( Object* _object )
            : routers( Main::get().m_routers ), object( *_object )
            {
                
            }
            
            void operator()( h::Table& table );
        };
        
    private:
        static std::string s_global;
        Routers m_routers;
        Script m_script;
        bool m_success;
        bool m_init;
    };
    
    class Object: public tau::in::Female
    {
        friend class Main;
        friend class Main::Router;
        friend class Runner;
        friend class h::Stack;  
        friend class h::Arguments;  
        

    public:
        typedef Main::Router::Call Call;
        typedef lua::Method Method;
        
        virtual ~Object( )
        {
        }
        
        const std::string& name( ) const
        {
            return m_name;
        }

        const Main& lua( ) const
        {
            return Main::get();
        }

        virtual unsigned int index( ) const
        {
            return typeid ( this ).hash_code( );
        }
        
        Call& call( ) const
        {
            assert( m_call );
            return *m_call;
        }
        
        static Object* get( void* data )
        {
            return Data::get( data );
        }
        
        bool global() const
        {
            return m_global;
        }

    protected:
        Object( );
        void method( const std::string& name, Method method = NULL, unsigned int id = 0 )
        {
            m_calls.insert( Call::Map::value_type( name, Call( name, method, id ) ) );
        }
        
        void setName( const std::string& name )
        {
            m_name = name;
        }

        Runner& runner( );
        Runner* assigned() const
        {
            return m_runners.first;
        }
    
        void setCall( Call* call )
        {
            m_call = call;
        }

        void resume( h::Arguments* arguments = NULL );
        virtual void suspend( );

        void error( const Exception& e );
        
        struct Data
        {
            Object* object;
            Data( Object* _object )
            : object( _object )
            {
            }
            
            static Object* get( void* data )
            {
                return data ? ( ( Data* ) data )->object : NULL;
            }
        };
        
        const Data* data() const
        {
            return &m_data;
        }
        
        void setGlobal( bool global )
        {
            m_global = global;
        }
        
        void clear();

    private:
        Runner* current() const
        {
            return m_runners.second;
        }
        
        virtual void onIndex( const Main::Router& router, h::Table& table )
        {
        }
        
        void push( Runner& runner );
        void operator()( Call& call, h::Stack& stack );
        const Call::Map& calls() const
        {
            return m_calls;
        }
                
        virtual bool onProcess( const std::string& name, h::Stack& stack )
        {
            return false;
        }
        
        static int gcStatic( lua_State* lua );
        virtual void gc( ) = 0;

        typedef std::pair< Runner*, Runner* > Runners;

        void assign( Runner& );
        void setRunner( Runner& runner )
        {
            m_runners.second = &runner;
        }
        
        void runnerStopped( tau::Grain& );
        virtual void onAssignedStop()
        {
            
        }
        
        virtual void onRunnerStop( Runner& )
        {
            
        }
        
    protected:
        Call* m_call;

    private:
        std::string m_name;
        Data m_data; 
        bool m_global;
        Call::Map m_calls;
        Runners m_runners;
    };
    
    class Runner : public State, public tau::in::Male, public tau::in::Female
    {
        friend class Main;
        friend class Main::Router;
        friend class Object;
        friend class State;
        
    public:
        typedef std::list< Runner* > List;
        
#define EVENT_RUNNER 10
        enum Events
        {
            Stop = EVENT_RUNNER
        };
        
        virtual ~Runner( )
        {
        }
        
        void setStart( types::Function& start )
        {
            m_start = &start;
        }
        
        void run( Object* object = NULL, h::Arguments* arguments = NULL );
        void suspend( Object* object )
        {
            m_way.object = object;
            setStatus( Suspended );
        }
        void stop();
        
        std::string id() const
        {
            return tau::u::fprint( "0x%lx", ( long ) m_lua );
        }
        
        enum Status
        {
            Stopped,
            Started,
            Running,
            Suspended,
            Error
        };
        bool suspended() const
        {
            return m_status == Suspended; 
        }
        bool started() const
        {
            return m_status == Started;
        }
        bool running() const
        {
            return m_status == Running || m_status == Suspended;
        }
        bool success() const
        {
            return m_status != Error;
        }        
        bool stopped() const
        {
            return m_status == Stopped || m_status == Error;
        }
        const Status& status() const
        {
            return m_status;
        }
        
        void exception( const Exception& e )
        {
            m_exception = new Exception( e );
        }
        
        bool skip( Object* object ) const;
        
        struct Way
        {
            Object* object;
            Object::Call* call;
            Runner& runner;
            
            Way( Runner& _runner )
            : object( NULL ), call( NULL ), runner( _runner )
            {
            }
            
            void operator()( Object* _object, Object::Call* _call );
            void clear()
            {
                object = NULL;
                call = NULL;
            }
        };
        
        Way& way()
        {
            return m_way;
        }
        const Way& way() const
        {
            return m_way;
        }
        
        void next();
        
    private:
        Runner( );
        void start( );
        virtual void cleanup( );
        
        int yield( ) const
        {
            ENTER();
            return lua_yield( m_lua, 0 );
        }
        
        void setStatus( Status status )
        {
            m_status = status;
        }
        
        bool errors();
        void clear();
                
        virtual unsigned int hash() const
        {
            return typeid( *this ).hash_code();
        }
        
        static Runner* create( );
        void init();
        
        virtual void destroy();
        struct Data
        {
            Runner* runner;
            
            Data( Runner* _runner )
            : runner( _runner )
            {
            }
        };
        
        static Runner& get( void* data )
        {
            auto runner =  data ? ( static_cast< Data* >( data ) )->runner : NULL;
            assert( runner );
            return *runner;
        }
        
        Main& main() const
        {
            return Main::get();
        }
        
        void mainStopped( tau::Grain& grain )
        {
            ENTER();
            stop();
        }
        
        void timer( tau::Grain& grain );
        
    private:
        unsigned int m_reference;
        types::Function* m_start;
        Status m_status;
        Exception* m_exception;
        Data m_data;
        Way m_way;
    };
}
#endif	


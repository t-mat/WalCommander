#pragma once

class Tlsf;
struct lua_State;
struct luaL_Reg;

#if !defined(LUA_WRAPPER_NS)
#  define LUA_WRAPPER_NS lua
#endif

namespace LUA_WRAPPER_NS {
	class StateBase {
	protected:
		StateBase();
		virtual ~StateBase();
		void open();
		void close();

	public:
		operator lua_State* ();
		int loadString(const char* str, size_t strBytes);
		int loadString(const char* str);

		using luaFunction = int(*)(lua_State *L);
		void registerFunction(const char* name, luaFunction f);
		int pcall(int nargs, int nresults, int msgh);

		void registerLibs(const luaL_Reg* libs, size_t libsCount);
		int eval(const char* luaScriptString);

	protected:
		static StateBase* getThat(lua_State* ls);

		static void* luacb_alloc_static(void *ud, void *ptr, size_t osize, size_t nsize);
		virtual void* alloc(void* ptr, size_t osize, size_t nsize) = 0;

		static int luacb_panic_static(lua_State* ls);
		virtual int panic(lua_State* ls) = 0;

	private:
		struct {
			lua_State* L {};
		} lua {};
	};

	class State : public StateBase {
	public:
		State(void* memory, size_t memoryBytes = 4 * 1024 * 1024);
		~State();
		void* alloc(void* ptr, size_t osize, size_t nsize) override;
		int panic(lua_State* ls) override;
		void heapWalk();

	protected:
		Tlsf* tlsf;
	};
}

void initKeyMap();
unsigned remapKey(unsigned mode, unsigned key);
wal::cevent_key remapKey(unsigned mode, const wal::cevent_key& key );

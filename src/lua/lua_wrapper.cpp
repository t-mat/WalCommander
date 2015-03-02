#include <functional>
extern "C" {
#include "../deps/lua/src/lua.h"
}
#include "lua_wrapper.h"
#include "tlsf_wrapper.h"

LUA_WRAPPER_NS::StateBase::StateBase() {}

LUA_WRAPPER_NS::StateBase::~StateBase() {
	close();
}

void LUA_WRAPPER_NS::StateBase::open() {
	close();

	lua.L = lua_newstate(luacb_alloc_static, this);
	if(lua.L) {
		lua_atpanic(lua.L, luacb_panic_static);
	}
}

void LUA_WRAPPER_NS::StateBase::close() {
	if(lua.L) {
		lua_close(lua.L);
		lua.L = nullptr;
	}
}

LUA_WRAPPER_NS::StateBase::operator lua_State* () {
	return lua.L;
}

int LUA_WRAPPER_NS::StateBase::loadString(const char* str, size_t strBytes) {
	struct MyArg {
		const char* str;
		size_t size;
	};

	MyArg myArg {};
	myArg.str = str;
	myArg.size = strBytes;

	const char* name = nullptr;
	const char* mode = nullptr;

	const auto getS = [](lua_State* L, void* ud, size_t* size) -> const char* {
		auto* myArg = reinterpret_cast<MyArg*>(ud);
		if(myArg->size == 0) {
			return nullptr;
		}
		*size = myArg->size;
		myArg->size = 0;
		return myArg->str;
	};
	return lua_load((*this), getS, &myArg, name, mode);
}

int LUA_WRAPPER_NS::StateBase::loadString(const char* str) {
	return loadString(str, strlen(str));
}

void LUA_WRAPPER_NS::StateBase::registerFunction(const char* name, lua_CFunction f) {
	return lua_register(lua.L, name, f);
}

int LUA_WRAPPER_NS::StateBase::pcall(int nargs, int nresults, int msgh) {
	return lua_pcall(lua.L, nargs, nresults, msgh);
}

LUA_WRAPPER_NS::StateBase* LUA_WRAPPER_NS::StateBase::getThat(lua_State* ls) {
	void* ud {};
	lua_getallocf(ls, &ud);
	return reinterpret_cast<StateBase*>(ud);
}

void* LUA_WRAPPER_NS::StateBase::luacb_alloc_static(void *ud, void *ptr, size_t osize, size_t nsize) {
	auto* that = reinterpret_cast<StateBase*>(ud);
	return that->alloc(ptr, osize, nsize);
}

int LUA_WRAPPER_NS::StateBase::luacb_panic_static(lua_State* ls) {
	return getThat(ls)->panic(ls);
}

LUA_WRAPPER_NS::State::State(void* memory, size_t memoryBytes) {
//	const size_t poolBytes = 1024 * 1024 * 16;
//	const size_t poolBytes2 = Tlsf::controlSize() + poolBytes;
//	memPool.resize(poolBytes2);

//	tlsf = Tlsf::create(memPool.data(), memPool.size());
	tlsf = Tlsf::create(memory, memoryBytes);

	open();
}

LUA_WRAPPER_NS::State::~State() {
	close();
	tlsf->destroy();
}

void* LUA_WRAPPER_NS::State::alloc(void* ptr, size_t osize, size_t nsize) {
	if(0 == nsize) {
		tlsf->free(ptr);
		return nullptr;
	} else {
		return tlsf->realloc(ptr, nsize);
	}
}

int LUA_WRAPPER_NS::State::panic(lua_State* ls) {
	printf("PANIC: unprotected error in call to Lua API (%s)\n"
		   , lua_tostring(ls, -1));
	return 0;  // return to Lua to abort
}

void LUA_WRAPPER_NS::State::heapWalk() {
	const auto walker = [&] (const void* ptr, size_t size, int used) {
		printf("\t%p %s size: %x\n"
			   , ptr
			   , used ? "used" : "free"
			   , (unsigned int)size
		);
	};
	tlsf->walk(walker);
}

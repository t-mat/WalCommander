#include <windows.h>
#include <functional>
#include <map>
#include <vector>
#include <mutex>
#include <memory>

#include "ncwin.h"
#include "IntrusivePtr.h"
#include "swl_wincore.h"

extern "C" {
#include "../deps/lua/src/lua.h"
#include "../deps/lua/src/lauxlib.h"
}

#include "lua_wrapper.h"
#include "tlsf_wrapper.h"

LUAMOD_API int wal_luaopen_vk(lua_State *L);

namespace {

struct Constant {
	const char*	name;
	int			value;
};

static const Constant vkConstants[] = {
	{	"PANEL",				NCWin::MODE::PANEL		},
	{	"TERMINAL",				NCWin::MODE::TERMINAL	},
	{	"VIEW",					NCWin::MODE::VIEW		},
	{	"EDIT",					NCWin::MODE::EDIT		},
	{	"ESCAPE",				VK_ESCAPE				},
	{	"TAB",					VK_TAB					},
	{	"RETURN",				VK_RETURN				},
	{	"NUMPAD_RETURN",		VK_NUMPAD_RETURN		},
	{	"BACK",					VK_BACK					},
	{	"LEFT",					VK_LEFT					},
	{	"RIGHT",				VK_RIGHT				},
	{	"HOME",					VK_HOME					},
	{	"END",					VK_END					},
	{	"UP",					VK_UP					},
	{	"DOWN",					VK_DOWN					},
	{	"SPACE",				VK_SPACE				},
	{	"DELETE",				VK_DELETE				},
	{	"NEXT",					VK_NEXT					},
	{	"PRIOR",				VK_PRIOR				},
	{	"OEM_PLUS",				VK_OEM_PLUS				},
	{	"ADD",					VK_ADD					},
	{	"SUBTRACT",				VK_SUBTRACT				},
	{	"MULTIPLY",				VK_MULTIPLY				},
	{	"DIVIDE",				VK_DIVIDE				},
	{	"SLASH",				VK_SLASH				},
	{	"BACKSLASH",			VK_BACKSLASH			},
	{	"GRAVE",				VK_GRAVE				},
	{	"INSERT",				VK_INSERT				},
	{	"LMETA",				VK_LMETA				},
	{	"RMETA",				VK_RMETA				},
	{	"LCONTROL",				VK_LCONTROL				},
	{	"RCONTROL",				VK_RCONTROL				},
	{	"LSHIFT",				VK_LSHIFT				},
	{	"RSHIFT",				VK_RSHIFT				},
	{	"LMENU",				VK_LMENU				},
	{	"RMENU",				VK_RMENU				},
	{	"BRACKETLEFT",			VK_BRACKETLEFT			},
	{	"BRACKETRIGHT",			VK_BRACKETRIGHT			},
	{	"0",					VK_0,					},
	{	"1",					VK_1,					},
	{	"2",					VK_2,					},
	{	"3",					VK_3,					},
	{	"4",					VK_4,					},
	{	"5",					VK_5,					},
	{	"6",					VK_6,					},
	{	"7",					VK_7,					},
	{	"8",					VK_8,					},
	{	"9",					VK_9,					},
	{	"A",					VK_A,					},
	{	"B",					VK_B,					},
	{	"C",					VK_C,					},
	{	"D",					VK_D,					},
	{	"E",					VK_E,					},
	{	"F",					VK_F,					},
	{	"G",					VK_G,					},
	{	"H",					VK_H,					},
	{	"I",					VK_I,					},
	{	"J",					VK_J,					},
	{	"K",					VK_K,					},
	{	"L",					VK_L,					},
	{	"M",					VK_M,					},
	{	"N",					VK_N,					},
	{	"O",					VK_O,					},
	{	"P",					VK_P,					},
	{	"Q",					VK_Q,					},
	{	"R",					VK_R,					},
	{	"S",					VK_S,					},
	{	"T",					VK_T,					},
	{	"U",					VK_U,					},
	{	"V",					VK_V,					},
	{	"W",					VK_W,					},
	{	"X",					VK_X,					},
	{	"Y",					VK_Y,					},
	{	"Z",					VK_Z,					},
	{	"F1",					VK_F1,					},
	{	"F2",					VK_F2,					},
	{	"F3",					VK_F3,					},
	{	"F4",					VK_F4,					},
	{	"F5",					VK_F5,					},
	{	"F6",					VK_F6,					},
	{	"F7",					VK_F7,					},
	{	"F8",					VK_F8,					},
	{	"F9",					VK_F9,					},
	{	"F10",					VK_F10,					},
	{	"F11",					VK_F11,					},
	{	"F12",					VK_F12,					},
};


using KeyMap = std::map<unsigned, unsigned>;
using KeyMaps = std::map<NCWin::MODE, KeyMap>;
KeyMaps keyMaps;
std::once_flag keyMapsInitFlag;

std::unique_ptr<LUA_WRAPPER_NS::State> luaState;
std::vector<char> luaMemPool;

int walvk_keyRemap(lua_State *L)
{
	const lua_Integer argMode = luaL_checkinteger(L, 1);
	const lua_Integer argNewKey = luaL_checkinteger(L, 2);
	const lua_Integer argOldKey = luaL_checkinteger(L, 3);

	keyMaps[(NCWin::MODE)argMode][argNewKey] = argOldKey;

	return 0;
}

} // anonymous namespace


LUAMOD_API int wal_luaopen_vk(lua_State *L)
{
	static const luaL_Reg vklib[] =
	{
		{ "keyRemap", walvk_keyRemap },
		// placeholders
		{ "A", nullptr },
		{ nullptr, nullptr }
	};

	const auto registerConstant = [&](const char* name, int value) {
		lua_pushinteger(L, value);
		lua_setfield(L, -2, name);
	};

	using namespace wal;

	luaL_newlib(L, vklib);
	for(const auto& c : vkConstants) {
		registerConstant(c.name, c.value);
	}

	return 1;
}


void initKeyMap()
{
	std::call_once(keyMapsInitFlag, [&]() {
		luaMemPool.resize(4 * 1024 * 1024);
		luaState = std::unique_ptr<LUA_WRAPPER_NS::State>(
			new LUA_WRAPPER_NS::State { luaMemPool.data(), luaMemPool.size() }
		);

		static const luaL_Reg libs[] =
		{
			{ "walvk", wal_luaopen_vk },
		};

		luaState->registerLibs(libs, sizeof(libs)/sizeof(libs[0]));

//		// Panel
//		keyMaps[NCWin::MODE::PANEL][VK_V] = VK_F3;	// 'V' -> VK_F3
//
//		// Text Viewer
//		keyMaps[NCWin::MODE::VIEW][VK_LEFT] = VK_PRIOR;	// Left -> PageUp
//		keyMaps[NCWin::MODE::VIEW][VK_RIGHT] = VK_NEXT;	// Right -> PageDown
		const char* testLuaCode = R"(
			walvk.keyRemap(walvk.PANEL, walvk.V, walvk.F3)
			walvk.keyRemap(walvk.VIEW, walvk.LEFT, walvk.PRIOR)
			walvk.keyRemap(walvk.VIEW, walvk.RIGHT, walvk.NEXT)
		)";

		luaState->eval(testLuaCode);

	});
}

unsigned remapKey( unsigned mode_, unsigned key )
{
	const auto mode = (NCWin::MODE) mode_;
	const auto it0 = keyMaps.find( mode );
	if ( it0 != keyMaps.end() )
	{
		const auto& keymap = it0->second;
		const auto it1 = keymap.find( key );
		if( it1 != keymap.end() )
		{
			key = it1->second;
		}
	}
	return key;
}

wal::cevent_key remapKey(unsigned mode_, const wal::cevent_key& key )
{
	const auto mode = (NCWin::MODE) mode_;
	const int Key = key.Key();
	const int Mod = key.Mod();
	const unsigned FullKey0 = ( Key & 0xFFFF ) + ( Mod << 16 );
	auto newkey = remapKey( mode, FullKey0 );
	return wal::cevent_key {
		key.Type(),
		static_cast<int>(newkey & 0xFFFF),
		(newkey >> 16) & 0xFFFFU,
		key.Count(),
		key.Char(),
		key.IsFromMouseWheel()
	};
}

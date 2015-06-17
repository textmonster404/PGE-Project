#ifndef BINDING_LEVEL_CLASSWRAPPER_LVL_NPC_H
#define BINDING_LEVEL_CLASSWRAPPER_LVL_NPC_H

#include <scenes/level/lvl_npc.h>

#include <luabind/luabind.hpp>
#include <lua_inclues/lua.hpp>

class Binding_Level_ClassWrapper_LVL_NPC : public LVL_Npc, public luabind::wrap_base
{
public:
    Binding_Level_ClassWrapper_LVL_NPC();
    virtual ~Binding_Level_ClassWrapper_LVL_NPC();

    virtual void lua_onLoop();
    virtual void lua_onLoopTimed(float tickTime);
    virtual void lua_onInit();

    static void def_lua_onLoop(LVL_Npc* base) { base->LVL_Npc::lua_onLoop(); }
    static void def_lua_onLoopTimed(LVL_Npc* base, float tickTime) { base->LVL_Npc::lua_onLoopTimed(tickTime); }
    static void def_lua_onInit(LVL_Npc* base) { base->LVL_Npc::lua_onInit(); }

    static luabind::scope bindToLua();
};

#endif // BINDING_LEVEL_CLASSWRAPPER_LVL_NPC_H

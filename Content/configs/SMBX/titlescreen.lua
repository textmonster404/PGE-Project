
local deb_i = 0

function onLoop()
    deb_i = deb_i + 1
    Renderer.printText("A TEST STRING FROM LUA", 100, 100)
    Renderer.printText("Ticks passed: "..deb_i, 100, 130)
end


function __native_event(eventObj, ...)
    local eventFuncToFind = "on"..eventObj.eventName:sub(0, 1):upper()..eventObj.eventName:sub(2)
    Logger.debug(eventFuncToFind)
    if(type(_G[eventFuncToFind]) == "function")then
        _G[eventFuncToFind](...)
    end
end



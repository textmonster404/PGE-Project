class 'peachPlayer'

function peachPlayer:__init(plr_obj)
    self.plr_obj = plr_obj
    if(self.plr_obj.stateID==1)then
        self.plr_obj.health = 1
    --elseif(self.plr_obj.stateID>=2)then
    --    self.plr_obj.health = 2
    end
end

function peachPlayer:onLoop(tickTime)
    if(Settings.isDebugInfoShown())then
        Renderer.printText("I am Peach! Health: "..tostring(self.plr_obj.health).." ID:"..tostring(self.plr_obj.characterID), 100, 230, 0, 15, 0xFFFF0055)
        Renderer.printText("Player x: "..tostring(self.plr_obj.x), 100, 260, 0, 15, 0xFFFF0055)
        Renderer.printText("Player y: "..tostring(self.plr_obj.y), 100, 300, 0, 15, 0xFFFF0055)
    end    
end

function peachPlayer:onHarm(harmEvent)
    processPlayerHarm(self.plr_obj, harmEvent)
end

function peachPlayer:onTakeNpc(npcObj)
    ProcessPlayerPowerUP(self.plr_obj, npcObj)
end

return peachPlayer


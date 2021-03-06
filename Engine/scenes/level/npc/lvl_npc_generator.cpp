/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2017 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../lvl_npc.h"
#include "../../scene_level.h"
#include <Utils/maths.h>

void LVL_Npc::updateGenerator(double tickTime)
{
    if(!m_isGenerator)
        return;

    generatorTimeLeft -= tickTime;
    if(generatorTimeLeft <= 0)
    {
        generatorTimeLeft += data.generator_period * 100.0;
        if(!contacted_npc.empty())
            return;
        if(!contacted_players.empty())
            return;
        LevelNPC def = data;
        def.x = Maths::lRound(posX());
        def.y = Maths::lRound(posY());
        def.generator = false;
        def.layer = "Spawned NPCs";
        m_scene->spawnNPC(def,
                         static_cast<LevelScene::NpcSpawnType>(generatorType),
                         static_cast<LevelScene::NpcSpawnDirection>(generatorDirection), false);
    }
}

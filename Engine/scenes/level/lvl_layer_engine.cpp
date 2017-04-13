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

#include "lvl_layer_engine.h"
#include <data_configs/config_manager.h>
#include "../scene_level.h"

LVL_LayerEngine::LVL_LayerEngine(LevelScene *_parent) :
    m_scene(_parent),
    m_layers()
{
    // Automatically initialize special layers
    {
        Layer &db = m_layers[DESTROYED_LAYER_NAME];
        db.m_layerType = Layer::T_DESTROYED_BLOCKS;
        db.m_visible = false;
    }
    {
        Layer &db = m_layers[SPAWNED_LAYER_NAME];
        db.m_layerType = Layer::T_SPAWNED_NPCs;
        db.m_visible = true;
    }
}

void LVL_LayerEngine::spawnSmokeAt(double x, double y)
{
    SpawnEffectDef smoke = ConfigManager::g_setup_effects.m_smoke;
    smoke.startX = x;
    smoke.startY = y;
    m_scene->launchEffect(smoke, true);
}

LVL_LayerEngine::Layer &LVL_LayerEngine::getLayer(const std::string &lyr)
{
    LayersTable::iterator li = m_layers.find(lyr);
    if(li == m_layers.end())
    {
        Layer &lr = m_layers[lyr];
        lr.m_layerType = Layer::T_REGULAR;
        lr.m_visible = true;
        return lr;
    }
    return li->second;
}


void LVL_LayerEngine::hide(std::string layer, bool smoke)
{
    Layer &lyr = getLayer(layer);
    for(Layer::Members::iterator it = lyr.m_members.begin();
        it != lyr.m_members.end();
        it++)
    {
        PGE_Phys_Object *body = *it;
        if(!body->isVisible())
            continue;
        body->hide();
        if(smoke)
            spawnSmokeAt(body->posCenterX(), body->posCenterY());
    }

    if(lyr.m_layerType == Layer::T_REGULAR)
        lyr.m_visible = false;
}

void LVL_LayerEngine::show(std::string layer, bool smoke)
{
    Layer &lyr = getLayer(layer);
    /*
    if(lyr.m_layerType == Layer::T_DESTROYED_BLOCKS)
    {
        //Restore all destroyed and modified blocks into their initial states
        for(Layer::Members::iterator it = lyr.m_members.begin();
            it != lyr.m_members.end();
            it++)
        {
            LVL_Block *blk = dynamic_cast<LVL_Block *>(*it);
            if(!blk)
                continue;
            //lyr.m_members.remove(intptr_t(blk));
            bool wasInvisible = !blk->isVisible() || blk->m_destroyed;
            blk->setVisible(true);
            blk->init(true);//Force re-initialize block from initial data
            if(smoke && wasInvisible)
            {
                PGE_Phys_Object::Momentum momentum = blk->m_momentum_relative;
                Layer &lyrn = getLayer(blk->data.layer);//Get block's original layer
                momentum.x -= lyrn.m_rtree.m_offsetX;
                momentum.y -= lyrn.m_rtree.m_offsetY;
                spawnSmokeAt(momentum.centerX(), momentum.centerY());
            }
        }

        //Clear layer from objects
        lyr.m_members.clear();
    }
    else
    {*/
        for(Layer::Members::iterator it = lyr.m_members.begin();
            it != lyr.m_members.end();
            it++)
        {
            PGE_Phys_Object *body = *it;
            if(body->isVisible())
                continue;
            body->show();
            if(smoke)
                spawnSmokeAt(body->posCenterX(), body->posCenterY());
        }
    //}

    if(lyr.m_layerType == Layer::T_REGULAR)
        lyr.m_visible = true;
}

void LVL_LayerEngine::toggle(std::string layer, bool smoke)
{
    Layer &lyr = getLayer(layer);
    bool viz = !lyr.m_visible;
    if(viz && (lyr.m_layerType == Layer::T_DESTROYED_BLOCKS))
    {
        show(layer);
        return;
    }
    else if(!viz && (lyr.m_layerType == Layer::T_SPAWNED_NPCs))
    {
        hide(layer);
        return;
    }

    for(Layer::Members::iterator it = lyr.m_members.begin(); it != lyr.m_members.end(); it++)
    {
        PGE_Phys_Object *body = *it;
        body->setVisible(viz);
        if(smoke)
            spawnSmokeAt(body->posCenterX(), body->posCenterY());
    }

    if(lyr.m_layerType == Layer::T_REGULAR)
        lyr.m_visible = viz;
}

void LVL_LayerEngine::registerItem(std::string layer, PGE_Phys_Object *item, bool keepAbsPos)
{
    //Register item in the layer
    Layer &lyr = getLayer(layer);
    //if( (item->type == PGE_Phys_Object::LVLBGO)||
    //    (item->type == PGE_Phys_Object::LVLBlock)||
    //    (item->type == PGE_Phys_Object::LVLWarp)||
    //    (item->type == PGE_Phys_Object::LVLPhysEnv) )
    if(item->m_bodytype == PGE_Phys_Object::Body_STATIC)
    {
        setItemMovable(lyr, item, true, keepAbsPos);
        if(item->type == PGE_Phys_Object::LVLBlock)
        {
            LVL_Block *blk = dynamic_cast<LVL_Block *>(item);
            if(blk)
            {
                if(blk->m_destroyed)//Mark block as destroyed objects
                    lyr.m_destroyedObjects++;
            }
        }
    }

    lyr.m_members.insert(item);
    if(lyr.m_layerType == Layer::T_REGULAR)
        item->setVisible(lyr.m_visible);
}

void LVL_LayerEngine::removeRegItem(std::string layer, PGE_Phys_Object *item, bool keepAbsPos)
{
    //Remove item from the layer
    Layer &lyr = m_layers[layer];
    //if( (item->type == PGE_Phys_Object::LVLBGO)||
    //    (item->type == PGE_Phys_Object::LVLBlock)||
    //    (item->type == PGE_Phys_Object::LVLWarp)||
    //    (item->type == PGE_Phys_Object::LVLPhysEnv) )
    if(item->m_bodytype == PGE_Phys_Object::Body_STATIC)
    {
        setItemMovable(lyr, item, false, keepAbsPos);
        if(item->type == PGE_Phys_Object::LVLBlock)
        {
            LVL_Block *blk = dynamic_cast<LVL_Block *>(item);
            if(blk)
            {
                if(blk->m_destroyed)//Remove from list of destroyed objects
                    lyr.m_destroyedObjects--;
            }
        }
    }
    lyr.m_members.erase(item);
}

void LVL_LayerEngine::moveToAnotherLayerItem(std::string oldLayer, std::string newLayer, PGE_Phys_Object *item, bool keepAbsPos)
{
    removeRegItem(oldLayer, item, keepAbsPos);
    registerItem(newLayer, item, keepAbsPos);
}

void LVL_LayerEngine::setItemMovable(LVL_LayerEngine::Layer &lyr, PGE_Phys_Object *item, bool enabled, bool keepAbsPos)
{
    if(enabled)
    {
        if(!lyr.m_rtree.m_scene)
        {
            lyr.m_rtree.m_scene = m_scene;
            lyr.m_rtree.setSize(1.0, 1.0);
            lyr.m_rtree.m_momentum = item->m_momentum;
            //Keep initial position of layer itself
            lyr.m_rtree.m_momentum_relative = lyr.m_rtree.m_momentum;
            item->m_parent = &lyr.m_rtree;
            // Register tree on the root
            m_scene->registerElement(&lyr.m_rtree);
        }
        else
        {
            if(lyr.m_rtree.left() > item->left())
                lyr.m_rtree.setLeft(item->left());
            if(lyr.m_rtree.top() > item->top())
                lyr.m_rtree.setTop(item->top());
            if(lyr.m_rtree.right() < item->right())
                lyr.m_rtree.setRight(item->right());
            if(lyr.m_rtree.bottom() < item->bottom())
                lyr.m_rtree.setBottom(item->bottom());
            //Keep initial position of layer itself
            lyr.m_rtree.m_momentum_relative = lyr.m_rtree.m_momentum;
            lyr.m_rtree.m_momentum_relative.x += lyr.m_rtree.m_offsetX;
            lyr.m_rtree.m_momentum_relative.y += lyr.m_rtree.m_offsetY;
            lyr.m_rtree.m_momentum_relative.oldx += lyr.m_rtree.m_offsetXold;
            lyr.m_rtree.m_momentum_relative.oldy += lyr.m_rtree.m_offsetYold;
            lyr.m_rtree.m_momentum_relative.saveOld();
            item->m_parent = &lyr.m_rtree;
            //item->_syncPosition();
        }
        item->m_momentum_relative   = item->m_momentum;
        if(keepAbsPos)
        {
            item->m_momentum_relative.x += lyr.m_rtree.m_offsetX;
            item->m_momentum_relative.y += lyr.m_rtree.m_offsetY;
            item->m_momentum_relative.oldx += lyr.m_rtree.m_offsetXold;
            item->m_momentum_relative.oldy += lyr.m_rtree.m_offsetYold;
            item->m_momentum_relative.saveOld();
        }
        //Syncronize modified momentum and register in the SubTree
        item->_syncPosition();
        //Untegister from root
        m_scene->unregisterElement(item);
    }
    else
    {
        if(lyr.m_rtree.m_scene)
        {
            if(keepAbsPos && item->m_parent)
            {
                item->m_momentum_relative.x -= lyr.m_rtree.m_offsetX;
                item->m_momentum_relative.y -= lyr.m_rtree.m_offsetY;
                item->m_momentum_relative.oldx -= lyr.m_rtree.m_offsetXold;
                item->m_momentum_relative.oldy -= lyr.m_rtree.m_offsetYold;
                item->m_momentum = item->m_momentum_relative;
            }
            item->m_parent = nullptr;
            item->_syncPosition();
            //Unregister from layer's subtree body
            lyr.m_rtree.unregisterElement(item);
            //Register in the root
            m_scene->registerElement(item);
        }
    }
}

void LVL_LayerEngine::installLayerMotion(std::string layer, double speedX, double speedY)
{
    MovingLayersTable::iterator mv = m_movingLayers.find(layer);
    if(mv != m_movingLayers.end())
    {
        MovingLayer &l = mv->second;
        l.m_speedX = speedX;
        l.m_speedY = speedY;
    }
    else
    {
        if((speedX == 0.0) && (speedY == 0.0))
            return;//Don't store zero-speed layers!
        LayersTable::iterator li = m_layers.find(layer);
        if(li == m_layers.end())
            return;
        Layer &lyr = li->second;
        MovingLayer l;
        l.m_speedX = speedX;
        l.m_speedY = speedY;
        l.m_members = &lyr.m_members;
        l.m_subtree = &lyr.m_rtree;
        m_movingLayers.insert({layer, l});
    }
}

void LVL_LayerEngine::processMoving(double tickTime)
{
    if(m_movingLayers.empty())
        return;

    std::vector<std::string> remove_list;
    for(MovingLayersTable::iterator layerIt = m_movingLayers.begin();
        layerIt != m_movingLayers.end();
        layerIt++)
    {
        MovingLayer &l = layerIt->second;
        l.m_subtree->m_momentum.velX    = l.m_speedX;
        l.m_subtree->m_momentum.velXsrc = l.m_speedX;
        l.m_subtree->m_momentum.velY    = l.m_speedY;
        if(l.m_subtree->m_scene)
        {
            l.m_subtree->iterateStep(tickTime);
            l.m_subtree->m_offsetX    = l.m_subtree->m_momentum_relative.x - l.m_subtree->m_momentum.x;
            l.m_subtree->m_offsetY    = l.m_subtree->m_momentum_relative.y - l.m_subtree->m_momentum.y;
            l.m_subtree->m_offsetXold = l.m_subtree->m_momentum_relative.x - l.m_subtree->m_momentum.oldx;
            l.m_subtree->m_offsetYold = l.m_subtree->m_momentum_relative.y - l.m_subtree->m_momentum.oldy;
            l.m_subtree->_syncPosition();
        }
//        for(Layer::Members::iterator it = l.m_members->begin(); it != l.m_members->end(); it++)
//        {
//            PGE_Phys_Object *obj = it->second;
//            //Don't iterate playable characters
//            if(obj->type == PGE_Phys_Object::LVLPlayer)
//                continue;

//            //obj->setSpeed(l.m_speedX, l.m_speedY);

//            ////Don't iterate activated NPC's!
//            //if(obj->type == PGE_Phys_Object::LVLNPC)
//            //{
//            //    LVL_Npc *npc = dynamic_cast<LVL_Npc *>(obj);
//            //    SDL_assert_release(npc);
//            //    if(npc)
//            //    {
//            //        if(npc->isActivated /* &&
//            //           !npc->isGenerator &&
//            //           !npc->is_scenery*/)
//            //            continue;
//            //    }
//            //    else
//            //        continue;
//            //}
//            ////obj->iterateStep(tickTime, true);
//            //if((l.m_speedX == 0.0) && (l.m_speedY == 0.0))
//            //{
//            //    if(obj->m_bodytype == PGE_physBody::Body_STATIC)
//            //    {
//            //        //obj->m_momentum.y = obj->m_momentum.y < 0 ? floor(obj->m_momentum.y) : ceil(obj->m_momentum.y);
//            //        obj->m_momentum.saveOld();
//            //    }
//            //}
//            //obj->_syncPosition();
//        }
        if((l.m_speedX == 0.0) && (l.m_speedY == 0.0))
            remove_list.push_back(layerIt->first);
    }
    //Remove zero-speed layers
    for(size_t i = 0; i < remove_list.size(); i++)
        m_movingLayers.erase(remove_list[i]);
}

bool LVL_LayerEngine::isEmpty(std::string layer)
{
    LayersTable::iterator i = m_layers.find(layer);
    return (i == m_layers.end()) || ((i->second.m_members.size() - i->second.m_destroyedObjects) == 0);
}

void LVL_LayerEngine::clear()
{
    m_movingLayers.clear();
    m_layers.clear();
}


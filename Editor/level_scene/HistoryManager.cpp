/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2014 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "lvlscene.h"
#include "item_block.h"
#include "item_bgo.h"

void LvlScene::addRemoveHistory(LevelData removedItems)
{
    //add cleanup redo elements
    cleanupRedoElements();
    //add new element
    HistoryOperation rmOperation;
    rmOperation.type = HistoryOperation::LEVELHISTORY_REMOVE;
    rmOperation.data = removedItems;
    operationList.push_back(rmOperation);
    historyIndex++;

    historyChanged = true;
}

void LvlScene::addPlaceHistory(LevelData placedItems)
{
    //add cleanup redo elements
    cleanupRedoElements();
    //add new element
    HistoryOperation plOperation;
    plOperation.type = HistoryOperation::LEVELHISTORY_PLACE;
    plOperation.data = placedItems;
    operationList.push_back(plOperation);
    historyIndex++;

    historyChanged = true;
}

void LvlScene::addMoveHistory(LevelData sourceMovedItems, LevelData targetMovedItems)
{
    cleanupRedoElements();

    //calc new Pos:
    long baseX=0, baseY=0;

    //set first base
    if(!targetMovedItems.blocks.isEmpty()){
        baseX = targetMovedItems.blocks[0].x;
        baseY = targetMovedItems.blocks[0].y;
    }else if(!targetMovedItems.bgo.isEmpty()){
        baseX = targetMovedItems.bgo[0].x;
        baseY = targetMovedItems.bgo[0].y;
    }else if(!targetMovedItems.npc.isEmpty()){
        baseX = targetMovedItems.npc[0].x;
        baseY = targetMovedItems.npc[0].y;
    }

    foreach (LevelBlock block, targetMovedItems.blocks) {
        if(block.x<baseX){
            baseX = block.x;
        }
        if(block.y<baseY){
            baseY = block.y;
        }
    }
    foreach (LevelBGO bgo, targetMovedItems.bgo){
        if(bgo.x<baseX){
            baseX = bgo.x;
        }
        if(bgo.y<baseY){
            baseY = bgo.y;
        }
    }
    foreach (LevelNPC npc, targetMovedItems.npc){
        if(npc.x<baseX){
            baseX = npc.x;
        }
        if(npc.y<baseY){
            baseY = npc.y;
        }
    }

    HistoryOperation mvOperation;
    mvOperation.type = HistoryOperation::LEVELHISTORY_MOVE;
    mvOperation.data = sourceMovedItems;
    mvOperation.x = baseX;
    mvOperation.y = baseY;
    operationList.push_back(mvOperation);
    historyIndex++;

    historyChanged = true;
}

void LvlScene::historyBack()
{
    historyIndex--;
    HistoryOperation lastOperation = operationList[historyIndex];

    switch( lastOperation.type )
    {
    case HistoryOperation::LEVELHISTORY_REMOVE:
    {
        //revert remove
        LevelData deletedData = lastOperation.data;

        foreach (LevelBlock block, deletedData.blocks)
        {
            //place them back
            unsigned int newID = (++LvlData->blocks_array_id);

            //use it so redo can find faster via arrayID
            BlocksArrayIDForwarder[block.array_id] = newID;

            block.array_id = newID;
            LvlData->blocks.push_back(block);
            placeBlock(block);

        }

        foreach (LevelBGO bgo, deletedData.bgo)
        {
            //place them back
            unsigned int newID = (++LvlData->bgo_array_id);

            //use it so redo can find faster via arrayID
            BGOsArrayIDForwarder[bgo.array_id] = newID;

            bgo.array_id = newID;
            LvlData->bgo.push_back(bgo);
            placeBGO(bgo);

        }

        //refresh Animation control
        if(opts.animationEnabled) stopAnimation();
        if(opts.animationEnabled) startBlockAnimation();

        break;
    }
    case HistoryOperation::LEVELHISTORY_PLACE:
    {
        //revert place
        LevelData placeData = lastOperation.data;
        //get newest data
        LevelData updatedData;
        foreach (LevelBlock oldblock, placeData.blocks)
        {
            unsigned int newestArrayID = oldblock.array_id;
            while(BlocksArrayIDForwarder.contains(newestArrayID))
            {
                newestArrayID = BlocksArrayIDForwarder[newestArrayID];
            }
            oldblock.array_id = newestArrayID;
            updatedData.blocks.push_back(oldblock);
        }

        foreach (LevelBGO oldbgo, placeData.bgo)
        {
            unsigned int newestArrayID = oldbgo.array_id;
            while(BGOsArrayIDForwarder.contains(newestArrayID))
            {
                newestArrayID = BGOsArrayIDForwarder[newestArrayID];
            }
            oldbgo.array_id = newestArrayID;
            updatedData.bgo.push_back(oldbgo);
        }

        QMap<int, LevelBlock> sortedBlock;
        foreach (LevelBlock block, updatedData.blocks)
        {
            sortedBlock[block.array_id] = block;
        }

        QMap<int, LevelBGO> sortedBGO;
        foreach (LevelBGO bgo, updatedData.bgo)
        {
            sortedBGO[bgo.array_id] = bgo;
        }

        bool blocksFinished = false;
        bool bgosFinished = false;
        foreach (QGraphicsItem* item, items())
        {
            if(item->data(0).toString()=="Block")
            {
                if(sortedBlock.size()!=0)
                {
                    QMap<int, LevelBlock>::iterator beginItem = sortedBlock.begin();
                    unsigned int currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()>currentArrayId)
                    {
                        //not found
                        sortedBlock.erase(beginItem);
                    }
                    //but still test if the next blocks, is the block we search!
                    beginItem = sortedBlock.begin();
                    currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()==currentArrayId)
                    {
                        ((ItemBlock*)item)->removeFromArray();
                        removeItem(item);
                        sortedBlock.erase(beginItem);
                    }
                }
                else
                {
                    blocksFinished = true;
                }
            }
            else
            if(item->data(0).toString()=="BGO")
            {
                if(sortedBGO.size()!=0)
                {
                    QMap<int, LevelBGO>::iterator beginItem = sortedBGO.begin();
                    unsigned int currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()>currentArrayId)
                    {
                        //not found
                        sortedBGO.erase(beginItem);
                    }
                    //but still test if the next blocks, is the block we search!
                    beginItem = sortedBGO.begin();
                    currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()==currentArrayId)
                    {
                        ((ItemBGO *)item)->removeFromArray();
                        removeItem(item);
                        sortedBGO.erase(beginItem);
                    }
                }
                else
                {
                    bgosFinished = true;
                }
            }
            if(blocksFinished&&bgosFinished)
            {
                break;
            }
        }
        break;
    }
    case HistoryOperation::LEVELHISTORY_MOVE:
    {
        //revert move
        LevelData movedSourceData = lastOperation.data;

        //get newest data
        LevelData updatedData;
        foreach (LevelBlock oldblock, movedSourceData.blocks)
        {
            unsigned int newestArrayID = oldblock.array_id;
            while(BlocksArrayIDForwarder.contains(newestArrayID))
            {
                newestArrayID = BlocksArrayIDForwarder[newestArrayID];
            }
            oldblock.array_id = newestArrayID;
            updatedData.blocks.push_back(oldblock);
        }

        foreach (LevelBGO oldbgo, movedSourceData.bgo)
        {
            unsigned int newestArrayID = oldbgo.array_id;
            while(BGOsArrayIDForwarder.contains(newestArrayID))
            {
                newestArrayID = BGOsArrayIDForwarder[newestArrayID];
            }
            oldbgo.array_id = newestArrayID;
            updatedData.bgo.push_back(oldbgo);
        }

        QMap<int, LevelBlock> sortedBlock;
        foreach (LevelBlock block, updatedData.blocks)
        {
            sortedBlock[block.array_id] = block;
        }

        QMap<int, LevelBGO> sortedBGO;
        foreach (LevelBGO bgo, updatedData.bgo)
        {
            sortedBGO[bgo.array_id] = bgo;
        }

        bool blocksFinished = false;
        bool bgosFinished = false;
        foreach (QGraphicsItem* item, items())
        {
            if(item->data(0).toString()=="Block")
            {
                if(sortedBlock.size()!=0)
                {
                    QMap<int, LevelBlock>::iterator beginItem = sortedBlock.begin();
                    unsigned int currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()>currentArrayId)
                    {
                        //not found
                        sortedBlock.erase(beginItem);
                    }
                    //but still test if the next blocks, is the block we search!
                    beginItem = sortedBlock.begin();
                    currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()==currentArrayId)
                    {
                        //do move
                        item->setPos(QPointF((*beginItem).x,(*beginItem).y));
                        ((ItemBlock *)(item))->blockData.x = (long)item->scenePos().x();
                        ((ItemBlock *)(item))->blockData.y = (long)item->scenePos().y();
                        ((ItemBlock *)(item))->arrayApply();
                    }
                }
                else
                {
                    blocksFinished = true;
                }
            }
            else
            if(item->data(0).toString()=="BGO")
            {
                if(sortedBGO.size()!=0)
                {
                    QMap<int, LevelBGO>::iterator beginItem = sortedBGO.begin();
                    unsigned int currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()>currentArrayId)
                    {
                        //not found
                        sortedBGO.erase(beginItem);
                    }
                    //but still test if the next blocks, is the block we search!
                    beginItem = sortedBGO.begin();
                    currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()==currentArrayId)
                    {
                        //do move
                        item->setPos(QPointF((*beginItem).x,(*beginItem).y));
                        ((ItemBGO *)(item))->bgoData.x = (long)item->scenePos().x();
                        ((ItemBGO *)(item))->bgoData.y = (long)item->scenePos().y();
                        ((ItemBGO *)(item))->arrayApply();
                    }
                }
                else
                {
                    bgosFinished = true;
                }
            }
            if(blocksFinished&&bgosFinished)
            {
                break;
            }
        }




        break;
    }
    default:
        break;
    }

    LvlData->modified = true;

    historyChanged = true;
}

void LvlScene::historyForward()
{

    HistoryOperation lastOperation = operationList[historyIndex];

    switch( lastOperation.type )
    {
    case HistoryOperation::LEVELHISTORY_REMOVE:
    {
        //redo remove
        LevelData deletedData = lastOperation.data;
        //get newest data
        LevelData updatedData;
        foreach (LevelBlock oldblock, deletedData.blocks)
        {
            unsigned int newestArrayID = oldblock.array_id;
            while(BlocksArrayIDForwarder.contains(newestArrayID))
            {
                newestArrayID = BlocksArrayIDForwarder[newestArrayID];
            }
            oldblock.array_id = newestArrayID;
            updatedData.blocks.push_back(oldblock);
        }

        foreach (LevelBGO oldbgo, deletedData.bgo)
        {
            unsigned int newestArrayID = oldbgo.array_id;
            while(BGOsArrayIDForwarder.contains(newestArrayID))
            {
                newestArrayID = BGOsArrayIDForwarder[newestArrayID];
            }
            oldbgo.array_id = newestArrayID;
            updatedData.bgo.push_back(oldbgo);
        }


        QMap<int, LevelBlock> sortedBlock;
        foreach (LevelBlock block, updatedData.blocks)
        {
            sortedBlock[block.array_id] = block;
        }

        QMap<int, LevelBGO> sortedBGO;
        foreach (LevelBGO bgo, updatedData.bgo)
        {
            sortedBGO[bgo.array_id] = bgo;
        }

        bool blocksFinished = false;
        bool bgosFinished = false;
        foreach (QGraphicsItem* item, items()){
            if(item->data(0).toString()=="Block")
            {
                if(sortedBlock.size()!=0)
                {
                    QMap<int, LevelBlock>::iterator beginItem = sortedBlock.begin();
                    unsigned int currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()>currentArrayId)
                    {
                        //not found
                        sortedBlock.erase(beginItem);
                    }
                    //but still test if the next blocks, is the block we search!
                    beginItem = sortedBlock.begin();
                    currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()==currentArrayId)
                    {
                        ((ItemBlock*)item)->removeFromArray();
                        removeItem(item);
                        sortedBlock.erase(beginItem);
                    }
                }
                else
                {
                    blocksFinished = true;
                }
            }
            else
            if(item->data(0).toString()=="BGO")
            {
                if(sortedBGO.size()!=0)
                {
                    QMap<int, LevelBGO>::iterator beginItem = sortedBGO.begin();
                    unsigned int currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()>currentArrayId)
                    {
                        //not found
                        sortedBGO.erase(beginItem);
                    }
                    //but still test if the next blocks, is the block we search!
                    beginItem = sortedBGO.begin();
                    currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()==currentArrayId)
                    {
                        ((ItemBGO *)item)->removeFromArray();
                        removeItem(item);
                        sortedBGO.erase(beginItem);
                    }
                }
                else
                {
                    bgosFinished = true;
                }
            }
            if(blocksFinished&&bgosFinished)
            {
                break;
            }
        }



        break;
    }
    case HistoryOperation::LEVELHISTORY_PLACE:
    {
        //redo place
        LevelData placedData = lastOperation.data;

        foreach (LevelBlock block, placedData.blocks)
        {
            //place them back
            unsigned int newID = (++LvlData->blocks_array_id);

            //use it so redo can find faster via arrayID
            BlocksArrayIDForwarder[block.array_id] = newID;

            block.array_id = newID;
            LvlData->blocks.push_back(block);
            placeBlock(block, true);
        }

        foreach (LevelBGO bgo, placedData.bgo)
        {
            //place them back
            unsigned int newID = (++LvlData->bgo_array_id);
            //use it so redo can find faster via arrayID
            BGOsArrayIDForwarder[bgo.array_id] = newID;

            bgo.array_id = newID;
            LvlData->bgo.push_back(bgo);
            placeBGO(bgo, true);
        }

        //refresh Animation control
        if(opts.animationEnabled) stopAnimation();
        if(opts.animationEnabled) startBlockAnimation();

    }
    case HistoryOperation::LEVELHISTORY_MOVE:
    {

        //revert move
        LevelData movedSourceData = lastOperation.data;
        long targetPosX = lastOperation.x;
        long targetPosY = lastOperation.y;

        //calc new Pos:
        long baseX=0, baseY=0;

        //set first base
        if(!movedSourceData.blocks.isEmpty()){
            baseX = movedSourceData.blocks[0].x;
            baseY = movedSourceData.blocks[0].y;
        }else if(!movedSourceData.bgo.isEmpty()){
            baseX = movedSourceData.bgo[0].x;
            baseY = movedSourceData.bgo[0].y;
        }else if(!movedSourceData.npc.isEmpty()){
            baseX = movedSourceData.npc[0].x;
            baseY = movedSourceData.npc[0].y;
        }

        foreach (LevelBlock block, movedSourceData.blocks) {
            if(block.x<baseX){
                baseX = block.x;
            }
            if(block.y<baseY){
                baseY = block.y;
            }
        }
        foreach (LevelBGO bgo, movedSourceData.bgo){
            if(bgo.x<baseX){
                baseX = bgo.x;
            }
            if(bgo.y<baseY){
                baseY = bgo.y;
            }
        }
        foreach (LevelNPC npc, movedSourceData.npc){
            if(npc.x<baseX){
                baseX = npc.x;
            }
            if(npc.y<baseY){
                baseY = npc.y;
            }
        }

        //get newest data
        LevelData updatedData;
        foreach (LevelBlock oldblock, movedSourceData.blocks)
        {
            unsigned int newestArrayID = oldblock.array_id;
            while(BlocksArrayIDForwarder.contains(newestArrayID))
            {
                newestArrayID = BlocksArrayIDForwarder[newestArrayID];
            }
            oldblock.array_id = newestArrayID;
            updatedData.blocks.push_back(oldblock);
        }

        foreach (LevelBGO oldbgo, movedSourceData.bgo)
        {
            unsigned int newestArrayID = oldbgo.array_id;
            while(BGOsArrayIDForwarder.contains(newestArrayID))
            {
                newestArrayID = BGOsArrayIDForwarder[newestArrayID];
            }
            oldbgo.array_id = newestArrayID;
            updatedData.bgo.push_back(oldbgo);
        }

        QMap<int, LevelBlock> sortedBlock;
        foreach (LevelBlock block, updatedData.blocks)
        {
            sortedBlock[block.array_id] = block;
        }

        QMap<int, LevelBGO> sortedBGO;
        foreach (LevelBGO bgo, updatedData.bgo)
        {
            sortedBGO[bgo.array_id] = bgo;
        }

        bool blocksFinished = false;
        bool bgosFinished = false;
        foreach (QGraphicsItem* item, items()){
            if(item->data(0).toString()=="Block")
            {
                if(sortedBlock.size()!=0)
                {
                    QMap<int, LevelBlock>::iterator beginItem = sortedBlock.begin();
                    unsigned int currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()>currentArrayId)
                    {
                        //not found
                        sortedBlock.erase(beginItem);
                    }
                    //but still test if the next blocks, is the block we search!
                    beginItem = sortedBlock.begin();
                    currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()==currentArrayId)
                    {
                        long diffX = (*beginItem).x - baseX;
                        long diffY = (*beginItem).y - baseY;

                        item->setPos(QPointF(targetPosX+diffX,targetPosY+diffY));
                        ((ItemBlock *)(item))->blockData.x = (long)item->scenePos().x();
                        ((ItemBlock *)(item))->blockData.y = (long)item->scenePos().y();
                        ((ItemBlock *)(item))->arrayApply();

                    }
                }
                else
                {
                    blocksFinished = true;
                }
            }
            else
            if(item->data(0).toString()=="BGO")
            {
                if(sortedBGO.size()!=0)
                {
                    QMap<int, LevelBGO>::iterator beginItem = sortedBGO.begin();
                    unsigned int currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()>currentArrayId)
                    {
                        //not found
                        sortedBGO.erase(beginItem);
                    }
                    //but still test if the next blocks, is the block we search!
                    beginItem = sortedBGO.begin();
                    currentArrayId = (*beginItem).array_id;
                    if((unsigned int)item->data(2).toInt()==currentArrayId)
                    {
                        long diffX = (*beginItem).x - baseX;
                        long diffY = (*beginItem).y - baseY;

                        item->setPos(QPointF(targetPosX+diffX,targetPosY+diffY));
                        ((ItemBGO *)(item))->bgoData.x = (long)item->scenePos().x();
                        ((ItemBGO *)(item))->bgoData.y = (long)item->scenePos().y();
                        ((ItemBGO *)(item))->arrayApply();
                    }
                }
                else
                {
                    bgosFinished = true;
                }
            }
            if(blocksFinished&&bgosFinished)
            {
                break;
            }
        }
        break;
    }
    default:
        break;
    }

    historyIndex++;

    historyChanged = true;
}

int LvlScene::getHistroyIndex()
{
    return historyIndex;
}

void LvlScene::cleanupRedoElements()
{
    if(canRedo())
    {
        int lastSize = operationList.size();
        for(int i = historyIndex; i < lastSize; i++)
        {
            operationList.pop_back();
        }
    }
}

bool LvlScene::canUndo()
{
    return historyIndex > 0;
}

bool LvlScene::canRedo()
{
    return historyIndex < operationList.size();
}

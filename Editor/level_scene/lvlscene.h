/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2014 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef LVLSCENE_H
#define LVLSCENE_H

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QProgressDialog>
#include <QMenu>

#include <QGraphicsSceneMouseEvent>

#include <QGraphicsItemAnimation>

#include <QKeyEvent>
#include <QBitmap>
#include <QPainter>
#include <QMessageBox>
#include <QApplication>
#include <QtCore>
#include <QDebug>


#include "../file_formats/lvl_filedata.h"
#include "../file_formats/npc_filedata.h"
#include "../data_configs/data_configs.h"

#include "../common_features/logger.h"

#include "resizer/item_resizer.h"

#include "../data_configs/custom_data.h"

struct LevelEditingSettings
{
    bool animationEnabled;
    bool collisionsEnabled;
};

class LvlScene : public QGraphicsScene
{
    Q_OBJECT
public:
    LvlScene(dataconfigs &configs, LevelData &FileData, QObject *parent = 0);
    ~LvlScene();

    bool grid;

    void SwitchEditingMode(int EdtMode);
    int EditingMode; // 0 - selecting,  1 - erasing, 2 - placeNewObject
                     // 3 - drawing water/sand zone, 4 - placing from Buffer
    bool EraserEnabled;
    bool PasteFromBuffer;

    bool DrawMode; //Placing/drawing on map, disable selecting and dragging items
    bool disableMoveItems;
    bool contextMenuOpened;

    enum EditMode
    {
        MODE_Selecting=0,
        MODE_Erasing,
        MODE_PlacingNew,
        MODE_DrawSquare,
        MODE_PasteFromClip,
        MODE_Resizing,
        MODE_SelectingOnly
    };


    // ////////////ItemPlacers///////////
    void setItemPlacer(int itemType, unsigned long itemID=1, int dType=0);
    void setSquareDrawer();
    enum placingItemType
    {
        PLC_Block=0,
        PLC_BGO,
        PLC_NPC,
        PLC_Water,
        PLC_Door,
        PLC_PlayerPoint,
        PLC_Section
    };
    int placingItem;
    QGraphicsItem * cursor;
    void placeItemUnderCursor();
    void resetCursor();

    // //////////////////////////////////

    //Event Flags
    //bool wasPasted;  Now is trash
    //bool doCopy;
    //bool doCut;
    bool historyChanged;
    bool resetPosition;
    //bool SyncLayerList;
    //bool resetResizingSection;

    //Copy function
    LevelData copy(bool cut = false);
    void paste(LevelData BufferIn, QPoint pos);

    LevelEditingSettings opts;

    //void makeSectionBG(int x, int y, int h, int w);
    void makeSectionBG(QProgressDialog &progress);

    void InitSection(int sect);

    void drawSpace();
    void ChangeSectionBG(int BG_Id, int SectionID=-1);
    void DrawBG(int x, int y, int w, int h, int sctID, QPixmap &srcimg, QPixmap &srcimg2, obj_BG &bgsetup);

    void loadUserData(QProgressDialog &progress);

    void setBlocks(QProgressDialog &progress);
    void setBGO(QProgressDialog &progress);
    void setNPC(QProgressDialog &progress);
    void setWaters(QProgressDialog &progress);
    void setDoors(QProgressDialog &progress);
    void setPlayerPoints();

    void doorPointsSync(long arrayID, bool remove=false);

    QPixmap getNPCimg(unsigned long npcID);

    void applyLayersVisible();

    void startBlockAnimation();
    void stopAnimation();

    void setLocked(int type, bool lock);

    //Array Sort functions
    void sortBlockArray(QVector<LevelBlock > &blocks);
    void sortBlockArrayByPos(QVector<LevelBlock > &blocks);
    void sortBGOArray(QVector<LevelBGO > &bgos);

    QVector<UserBGs > uBGs;
    QVector<UserBGOs > uBGOs;
    QVector<UserBlocks > uBlocks;
    QVector<UserNPCs > uNPCs;

    QGraphicsItem * itemCollidesWith(QGraphicsItem * item);

    LevelData  * LvlData;

    LevelData LvlBuffer;

    dataconfigs * pConfigs;

    //Object Indexing:
    QVector<blocksIndexes > index_blocks;
    QVector<bgoIndexes > index_bgo;
    QVector<npcIndexes > index_npc;

    bool lock_bgo;
    bool lock_block;
    bool lock_npc;
    bool lock_door;
    bool lock_water;

    bool IsMoved;
    bool haveSelected;

    //default objects Z value
    int blockZ; // standart block
    int blockZs; // sizeble block
    int blockZl; // lava block
    int playerZ; //playerPointZ
    int bgoZf; // foreground BGO
    int bgoZb; // backround BGO
    int npcZf; // foreground NPC
    int npcZs; // standart NPC
    int npcZb; // background NPC (vines)
    int doorZ;
    int waterZ;
    int bgZ;
    int spaceZ1; // interSection space layer
    int spaceZ2;

    // //////////////////////Resizer////////////////////////
    ItemResizer * pResizer; //reisizer pointer
    void setSectionResizer(bool enabled, bool accept=false);
    void setBlockResizer(QGraphicsItem *targetBlock, bool enabled, bool accept=false);

    // ////////////HistoryManager///////////////////
    struct HistoryOperation{
        enum HistoryType{
            LEVELHISTORY_REMOVE = 0,               //Removed from map
            LEVELHISTORY_PLACE,                    //Placed new
            LEVELHISTORY_MOVE,                     //moved
            LEVELHISTORY_CHANGEDSETTINGS,          //changed settings of items
            LEVELHISTORY_RESIZESECTION,
            LEVELHISTORY_CHANGEDLAYER,
            LEVELHISTORY_RESIZEBLOCK,
            LEVELHISTORY_PLACEDOOR,
            LEVELHISTORY_ADDWARP,
            LEVELHISTORY_REMOVEWARP
        };
        HistoryType type;
        //used most of Operations
        LevelData data;
        LevelData data_mod;
        //subtype (if needed)
        int subtype;
        //for move
        long x, y;
        //misc
        QVariant extraData;
    };
    struct CallbackData{
        QGraphicsItem* item;
        HistoryOperation* hist;
        //custom data
        long x, y;
    };

    enum SettingSubType{
        SETTING_INVISIBLE = 0, //extraData: bool [Activated?]
        SETTING_SLIPPERY,      //extraData: bool [Activated?]
        SETTING_FRIENDLY,      //extraData: bool [Activated?]
        SETTING_BOSS,          //extraData: bool [Activated?]
        SETTING_NOMOVEABLE,    //extraData: bool [Activated?]
        SETTING_MESSAGE,       //extraData: QList<QVariant[String]> [Old Text, New Text]
        SETTING_DIRECTION,     //extraData: QList<QVariant[int]> [Old Dir, New Dir]
        SETTING_CHANGENPC,     //extraData: QList<QVariant[int]> [Old NPC ID, New NPC ID]
        SETTING_WATERTYPE      //extraData: bool [IsWater = true, IsQuicksand = false]
    };

    //typedefs
    typedef void (LvlScene::*callBackLevelBlock)(CallbackData, LevelBlock);
    typedef void (LvlScene::*callBackLevelBGO)(CallbackData, LevelBGO);
    typedef void (LvlScene::*callBackLevelNPC)(CallbackData, LevelNPC);
    typedef void (LvlScene::*callBackLevelWater)(CallbackData, LevelWater);
    typedef void (LvlScene::*callBackLevelDoors)(CallbackData, LevelDoors, bool); //bool isEntrance [true = entrance, false = exit]
    //add historys
    /*
     * NOTE: when use History with Doors, LevelDoors MUST be posted individual.
     * If Door Entrance: LevelDoors.isSetIn = true and LevelDoors.isSetOut = false
     * If Door Exit: LevelDoors.isSetOut = true and LevelDoors.isSetIn = false
     *
     */
    void addRemoveHistory(LevelData removedItems);
	void addPlaceHistory(LevelData placedItems);
    void addPlaceDoorHistory(int array_id, bool isEntrance, long x, long y);
    void addMoveHistory(LevelData sourceMovedItems, LevelData targetMovedItems);
    void addChangeSettingsHistory(LevelData modifiedItems, SettingSubType subType, QVariant extraData);
    void addResizeSectionHistory(int sectionID, long oldLeft, long oldTop, long oldRight, long oldBottom,
                                 long newLeft, long newTop, long newRight, long newBottom);
    void addChangedLayerHistory(LevelData changedItems, QString newLayerName);
    void addResizeBlockHistory(LevelBlock bl, long oldLeft, long oldTop, long oldRight, long oldBottom,
                               long newLeft, long newTop, long newRight, long newBottom);
    void addAddWarpHistory(int array_id, int listindex, int doorindex);
    void addRemoveWarpHistory(LevelDoors removedDoor);
    //history modifiers
    void historyBack();
    void historyForward();
    void cleanupRedoElements();
    //history information
    int getHistroyIndex();
    bool canUndo();
    bool canRedo();
    //Callbackfunctions: Move
    void historyRedoMoveBlocks(CallbackData cbData, LevelBlock data);
    void historyRedoMoveBGO(CallbackData cbData, LevelBGO data);
    void historyRedoMoveNPC(CallbackData cbData, LevelNPC data);
    void historyRedoMoveWater(CallbackData cbData, LevelWater data);
    void historyRedoMoveDoors(CallbackData cbData, LevelDoors data, bool isEntrance);
    void historyUndoMoveBlocks(CallbackData cbData, LevelBlock data);
    void historyUndoMoveBGO(CallbackData cbData, LevelBGO data);
    void historyUndoMoveNPC(CallbackData cbData, LevelNPC data);
    void historyUndoMoveWater(CallbackData cbData, LevelWater data);
    void historyUndoMoveDoors(CallbackData cbData, LevelDoors data, bool isEntrance);
    //Callbackfunctions: Remove
    void historyRemoveBlocks(CallbackData cbData, LevelBlock data);
    void historyRemoveBGO(CallbackData cbData, LevelBGO data);
    void historyRemoveNPC(CallbackData cbData, LevelNPC data);
    void historyRemoveWater(CallbackData cbData, LevelWater data);
    //Callbackfunctions: [Change Settings] Hide
    void historyUndoSettingsInvisibleBlock(CallbackData cbData, LevelBlock data);
    void historyRedoSettingsInvisibleBlock(CallbackData cbData, LevelBlock data);
    //Callbackfunctions: [Change Settings] Invisible
    void historyUndoSettingsSlipperyBlock(CallbackData cbData, LevelBlock data);
    void historyRedoSettingsSlipperyBlock(CallbackData cbData, LevelBlock data);
    //Callbackfunctions: [Change Settings] Friendly
    void historyUndoSettingsFriendlyNPC(CallbackData cbData, LevelNPC data);
    void historyRedoSettingsFriendlyNPC(CallbackData cbData, LevelNPC data);
    //Callbackfunctions: [Change Settings] Boss
    void historyUndoSettingsBossNPC(CallbackData cbData, LevelNPC data);
    void historyRedoSettingsBossNPC(CallbackData cbData, LevelNPC data);
    //Callbackfunctions: [Change Settings] NoMoveable
    void historyUndoSettingsNoMoveableNPC(CallbackData cbData, LevelNPC data);
    void historyRedoSettingsNoMoveableNPC(CallbackData cbData, LevelNPC data);
    //Callbackfunctions: [Change Settings] Message
    void historyUndoSettingsMessageNPC(CallbackData cbData, LevelNPC data);
    void historyRedoSettingsMessageNPC(CallbackData cbData, LevelNPC data);
    //Callbackfunctions: [Change Settings] Direction
    void historyUndoSettingsDirectionNPC(CallbackData cbData, LevelNPC data);
    void historyRedoSettingsDirectionNPC(CallbackData cbData, LevelNPC data);
    //Callbackfunctions: [Change Settings] Included NPC
    void historyUndoSettingsChangeNPCBlocks(CallbackData cbData, LevelBlock data);
    void historyRedoSettingsChangeNPCBlocks(CallbackData cbData, LevelBlock data);
    //Callbackfunctions: [Change Settings] Water Type
    void historyUndoSettingsTypeWater(CallbackData cbData, LevelWater data);
    void historyRedoSettingsTypeWater(CallbackData cbData, LevelWater data);
    //Callbackfunctions: Change Layer
    void historyUndoChangeLayerBlocks(CallbackData cbData, LevelBlock data);
    void historyUndoChangeLayerBGO(CallbackData cbData, LevelBGO data);
    void historyUndoChangeLayerNPC(CallbackData cbData, LevelNPC data);
    void historyUndoChangeLayerWater(CallbackData cbData, LevelWater data);
    void historyRedoChangeLayerBlocks(CallbackData cbData, LevelBlock data);
    void historyRedoChangeLayerBGO(CallbackData cbData, LevelBGO data);
    void historyRedoChangeLayerNPC(CallbackData cbData, LevelNPC data);
    void historyRedoChangeLayerWater(CallbackData cbData, LevelWater data);
    //Callbackfunctions: Change sizeable Block size
    void historyUndoResizeBlock(CallbackData cbData, LevelBlock data);
    void historyRedoResizeBlock(CallbackData cbData, LevelBlock data);
    //Callbackfunctions: Undo place of Doors
    void historyRemoveDoors(CallbackData cbData, LevelDoors door, bool isEntrance);
    //History functions requiring callback-functions
    void findGraphicsItem(LevelData toFind, HistoryOperation * operation, CallbackData customData,
                          callBackLevelBlock clbBlock, callBackLevelBGO clbBgo,
                          callBackLevelNPC clbNpc, callBackLevelWater clbWater,
                          callBackLevelDoors clbDoor,
                          bool ignoreBlock = false,
                          bool ignoreBGO = false, 
                          bool ignoreNPC = false,
                          bool ignoreWater = false,
                          bool ignoreDoors = false);

    void findGraphicsDoor(int array_id, HistoryOperation* operation, CallbackData customData,
                          callBackLevelDoors clbDoors, bool isEntrance);
    //miscellaneous
    QPoint calcTopLeftCorner(LevelData* data);
    // ////////////////////////////////////////////
    void openProps();



public slots:
    void selectionChanged();

protected:
    //void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void keyReleaseEvent ( QKeyEvent * keyEvent );

private:

    QGraphicsItem * itemCollidesCursor(QGraphicsItem * item);

    void placeBlock(LevelBlock &block, bool toGrid=false);
    void placeBGO(LevelBGO &bgo, bool toGrid=false);
    void placeNPC(LevelNPC &npc, bool toGrid=false);
    void placeWater(LevelWater &water, bool toGrid=false);
    void placePlayerPoint(PlayerPoint plr, bool init=false);

    void placeDoor(LevelDoors &door, bool toGrid=false);
    void placeDoorEnter(LevelDoors &door, bool toGrid=false, bool init=false);
    void placeDoorExit(LevelDoors &door, bool toGrid=false, bool init=false);

    void removeItemUnderCursor();

    QPoint applyGrid(QPoint source, int gridSize, QPoint gridOffset=QPoint(0,0) );

    void setSectionBG(LevelSection section);

    QPixmap uBlockImg;

    QBitmap npcmask;
    QPixmap uNpcImg;

    QPixmap uBgoImg;

    QPixmap tImg;//Tempotary buffer

    QVector<qreal > Z;
    //qreal sbZ;

    QMenu blockMenu;
    QMenu bgoMenu;
    QMenu npcMenu;
    QMenu waterMenu;
    QMenu DoorMenu;

    // The item being dragged.
    QGraphicsItem *mDragged;
    // The distance from the top left of the item to the mouse position.
    QPointF mDragOffset;

    // ////////////////HistoryManager///////////////////
    int historyIndex;
    QList<HistoryOperation> operationList;
    // /////////////////////////////////////////////////

};

#endif // LVLSCENE_H

// header
#include "protobotmemory.h"


//-----------------------------------------------------------------------------
/*!
*/
ProtoBotMemory::ProtoBotMemory()
 : _extra_x(0),
   _extra_y(0),
   _bomb_stone_x(0),
   _bomb_stone_y(0),
   _bomb_stone_count(0)
{
}


//-----------------------------------------------------------------------------
/*!
*/
void ProtoBotMemory::reset()
{
   invalidateExtraPosition();
   invalidateBombStonePosition();
}


//-----------------------------------------------------------------------------
/*!
*/
void ProtoBotMemory::invalidateExtraPosition()
{
   _extra_x = -1;
   _extra_y = -1;
}


//-----------------------------------------------------------------------------
/*!
   \param x extra x position
   \param y extra y position
*/
void ProtoBotMemory::setExtraPosition(int x, int y)
{
   _extra_x = x;
   _extra_y = y;
}



//-----------------------------------------------------------------------------
/*!
   \return \c true if extra position is valid
*/
bool ProtoBotMemory::isExtraPositionValid() const
{
   return _extra_x != -1;
}


//-----------------------------------------------------------------------------
/*!
   \return extra x position
*/
int ProtoBotMemory::getExtraPositionX() const
{
   return _extra_x;
}


//-----------------------------------------------------------------------------
/*!
   \return extra y position
*/
int ProtoBotMemory::getExtraPositionY() const
{
   return _extra_y;
}


//-----------------------------------------------------------------------------
/*!
*/
void ProtoBotMemory::invalidateBombStonePosition()
{
   _bomb_stone_x = -1;
   _bomb_stone_y = -1;
   _bomb_stone_count = -1;
}


//-----------------------------------------------------------------------------
/*!
   \param x bomb stone position x
   \param y bomb stone position y
*/
void ProtoBotMemory::setBombStonePosition(int x, int y)
{
   _bomb_stone_x = x;
   _bomb_stone_y = y;
}


//-----------------------------------------------------------------------------
/*!
   \return \c true if bomb stone is valid
*/
bool ProtoBotMemory::isBombStonePositionValid() const
{
   return _bomb_stone_x != -1;
}


//-----------------------------------------------------------------------------
/*!
   \return bomb stone position x
*/
int ProtoBotMemory::getBombStonePositionX() const
{
   return _bomb_stone_x;
}


//-----------------------------------------------------------------------------
/*!
   \return bomb stone position y
*/
int ProtoBotMemory::getBombStonePositionY() const
{
   return _bomb_stone_y;
}


//-----------------------------------------------------------------------------
/*!
   \param count bomb bomb stone count
*/
void ProtoBotMemory::setBombStoneCount(int count)
{
   _bomb_stone_count = count;
}


//-----------------------------------------------------------------------------
/*!
   \return bomb stone neighbor count
*/
int ProtoBotMemory::getBombStoneCount() const
{
   return _bomb_stone_count;
}

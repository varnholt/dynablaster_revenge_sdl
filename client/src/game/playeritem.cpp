#include "playeritem.h"
#include "math/matrix.h"
#include "nodes/mesh.h"
#include "animation/motionmixer.h"

PlayerItem::PlayerItem(int id, const std::string& nick, Constants::Color color)
    : _id(id),
      _color(color),
      _nick(nick),
      _pos(0,0,0)
{
}

PlayerItem::~PlayerItem()
{
   if (_mesh)
      delete _mesh;
}

void PlayerItem::kill()
{
   _killed= true;
   _mesh->setFrame( 0.0f );
}

void PlayerItem::win()
{
   _win= true;
   _mesh->setFrame( 0.0f );
}

bool PlayerItem::isWinner() const
{
   return _win;
}

void PlayerItem::setKilled(bool killed)
{
   _killed= killed;
}

bool PlayerItem::isKilled() const
{
   return _killed;
}

int PlayerItem::getID() const
{
	return _id;
}

Constants::Color PlayerItem::getColor() const
{
   return _color;
}

const std::string& PlayerItem::getNick() const
{
	return _nick;
}

void PlayerItem::setMaterial(Material *mat)
{
	_material= mat;
}

Material* PlayerItem::getMaterial() const
{
	return _material;
}

void PlayerItem::setMesh(Mesh *mesh)
{
	_mesh= mesh;
}

Mesh* PlayerItem::getMesh() const
{
   return _mesh;
}

void PlayerItem::setRotation(float rot)
{
	_rot= rot;
	update();
}

void PlayerItem::setPosition(float x, float y)
{
	_pos= Vector(x,-y,0);
   update();
}

const Vector &PlayerItem::getPosition() const
{
   return _pos;
}


void PlayerItem::update()
{
   Matrix pos= Matrix::position(_pos.x, _pos.y, 0.0f);
   Matrix mat= Matrix::rotateZ(_rot);
   Matrix scale= Matrix::scale(3.0f, 3.0f, 3.0f); //= Matrix::scale(0.03f, 0.03f, 0.03f);
	_mesh->setUserTransformable(true);
	_mesh->setTransform(mat * scale * pos);
}

void PlayerItem::setSpeed(float speed)
{
   _speed= speed;

   // MotionMixer *mixer= _mesh->getMotionMixer();
   if (speed > 0.0f)
   {
      float sp= (_speed-0.05) * 20.0;
      if (sp>1.0f) speed=1.0f;
      if (sp<0.0f) speed=0.0f;
      _anim_blend= 1.0f - sp;
   }
   else
   {
//      _stand_blend= 0.0f;
   }
}


float PlayerItem::getSpeed() const
{
   return _speed;
}


void PlayerItem::setFlash(float flash)
{
   _flash= flash;
}


void PlayerItem::animate(float /*time*/, float delta)
{
   if (_flash >= delta*0.05f)
      _flash -= delta*0.05f;
   else
      _flash= 0.0f;

   if (_mesh)
      _mesh->setRenderParameter(0, _flash);

   if (_speed > 0.0f || _killed || _win)
   {
      if (_stand_blend > 0.0f)
      {
         // blend from stand to actual animation pose
         _stand_blend -= delta*0.2;
//         _stand_blend = 0.0f;
         if (_stand_blend <= 0.0f)
            _stand_blend= 0.0f;
      }

      float frame= _mesh->getFrame() + (_speed+1.0f) * 100.0 * delta;
      if (!_killed && !_win)
         while (frame > 4000.0) frame -= 4000.0;
      _mesh->setFrame( frame );
   }
   else
   {
      if (_stand_blend < 1.0f)
      {
         // blend into stand pose
         _stand_blend += delta*0.1;
         if (_stand_blend >= 1.0f)
         {
            // nearest pose to "stand", start with left or right foot
            if (_left_foot)
               _mesh->setFrame( 833.0 );
            else
               _mesh->setFrame( 4000 - 833.0 );
            _left_foot= !_left_foot;
            _stand_blend= 1.0f;
         }
      }
   }

   MotionMixer *mixer= _mesh->getMotionMixer();
   if (mixer)
   {
      if (_killed)
         mixer->setAnimation(3, 3, 1.0f, 3, 0.0f);
      else if (_win)
         mixer->setAnimation(4, 4, 1.0f, 4, 0.0f);
      else
         mixer->setAnimation(1, 2, 1.0f-_anim_blend, 0, _stand_blend);
   }
}




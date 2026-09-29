// template map container using red/black tree
// maps key -> value

#pragma once

#include "array.h"

template <class KeyType, class ValueType>
class RbTreeMap
{
public:
   class Node
   {
   public:
      // constructor
      Node(const KeyType& key, const ValueType& value) : _key(key), _value(value)
      {
      }

      bool isBlack() const
      {
         return _color == 'B';
      }

      bool isRed() const
      {
         return _color == 'R';
      }

      void setRed()
      {
         _color = 'R';
      }

      void setBlack()
      {
         _color = 'B';
      }

      void setColor(char color)
      {
         _color = color;
      }

      char color() const
      {
         return _color;
      }

      Node* next()
      {
         Node* x = this;
         if (x->_right && x->_right->_parent != nullptr)
         {
            x = x->_right;

            while (x->_left && x->_left->_parent != nullptr)
            {
               x = x->_left;
            }

            return x;
         }

         Node* y = x->_parent;

         while (y->_parent != nullptr && x == y->_right)
         {
            x = y;
            y = y->_parent;
         }

         if (y->_key > _key)
         {
            return y;
         }
         return nullptr;
      }

      Node* _left = nullptr;
      Node* _right = nullptr;
      Node* _parent = nullptr;
      char _color = 0;
      KeyType _key;
      ValueType _value;
   };

   class Iterator
   {
   public:
      Iterator() = default;

      Iterator(Node* node) : _node(node)
      {
      }

      const KeyType& key() const
      {
         return _node->_key;
      }

      const ValueType& value() const
      {
         return _node->_value;
      }

      Node* node() const
      {
         return _node;
      }

      Iterator& operator++(int)
      {
         _node = _node->next();
         return *this;
      }

      bool operator!=(const Iterator& other)
      {
         return (_node != other.node());
      }

   private:
      Node* _node = nullptr;
   };

   // constructor
   RbTreeMap() = default;
   RbTreeMap(const RbTreeMap&) = delete;
   RbTreeMap& operator=(const RbTreeMap&) = delete;

   // destructor
   ~RbTreeMap()
   {
      postOrderDelete(_root);
   }

   Iterator begin() const
   {
      Node* node = _root;
      while (node->_left)
      {
         node = node->_left;
      }
      return Iterator(node);
   }

   Iterator find(const KeyType& key) const
   {
      Node* node = findNode(key);
      return Iterator(node);
   }

   Iterator end() const
   {
      return Iterator(nullptr);
   }

   // get list of values
   Array<ValueType> valueList() const
   {
      Array<ValueType> list;
      traverseValues(_root, list);
      return list;
   }

   // get list of keys
   Array<KeyType> keyList() const
   {
      Array<KeyType> list;
      traverseKeys(_root, list);
      return list;
   }

   // map contains key?
   bool contains(const KeyType& key)
   {
      Node* x = findNode(key);
      return (x != nullptr);
   }

   // insert key
   bool insert(const KeyType& key, const ValueType& value)
   {
      Node *x, *y, *z;

      y = nullptr;
      x = _root;

      while (x != nullptr)
      {
         y = x;
         if (key < x->_key)
         {
            x = x->_left;
         }
         else if (key > x->_key)
         {
            x = x->_right;
         }
         else
         {
            x->_value = value;
            return false;
         }
      }

      z = new Node(key, value);
      z->_parent = y;
      z->setRed();

      if (y == nullptr)
      {
         _root = z;
      }
      else
      {
         if (key < y->_key)
         {
            y->_left = z;
         }
         else
         {
            y->_right = z;
         }
      }

      fixup(z);

      return true;
   }

   // remove key
   bool remove(const KeyType& key)
   {
      Node* x = findNode(key);
      if (x == nullptr)
      {
         return false;
      }
      remove(x);
      delete x;
      return true;
   }

private:
   Node* findNode(const KeyType& key) const
   {
      Node* x = _root;

      while (x != nullptr)
      {
         if (key < x->_key)
         {
            x = x->_left;
         }
         else if (key > x->_key)
         {
            x = x->_right;
         }
         else
         {
            return x;
         }
      }

      return nullptr;
   }

   // delete key
   bool remove(Node* z)
   {
      Node *x, *y;

      if (z->_left == nullptr || z->_right == nullptr)
      {
         y = z;
      }
      else
      {
         y = z->next();
      }

      if (y->_left != nullptr)
      {
         x = y->_left;
      }
      else
      {
         x = y->_right;
      }

      x->_parent = y->_parent;
      if (y->_parent == nullptr)
      {
         _root = x;
      }
      else
      {
         if (y == y->_parent->_left)
         {
            y->_parent->_left = x;
         }
         else
         {
            y->_parent->_right = x;
         }
      }

      if (y != z)
      {
         y->set(z);
      }

      if (y->isBlack())
      {
         delete_fixup(x);
      }

      return true;
   }

   // left rotate
   int leftRotate(Node* x)
   {
      Node* y;
      y = x->_right;
      x->_right = y->_left;
      if (y->_left)
      {
         y->_left->_parent = x;
      }
      y->_parent = x->_parent;

      if (x->_parent == nullptr)
      {
         _root = y;
      }
      else if (x == x->_parent->_left)
      {
         x->_parent->_left = y;
      }
      else
      {
         x->_parent->_right = y;
      }
      y->_left = x;
      x->_parent = y;
      return 0;
   }

   // right rotate
   int rightRotate(Node* x)
   {
      Node* y;

      y = x->_left;
      x->_left = y->_right;
      if (y->_right)
      {
         y->_right->_parent = x;
      }
      y->_parent = x->_parent;

      if (x->_parent == nullptr)
      {
         _root = y;
      }
      else if (x == x->_parent->_right)
      {
         x->_parent->_right = y;
      }
      else
      {
         x->_parent->_left = y;
      }
      y->_right = x;
      x->_parent = y;
      return 0;
   }

   // fix up
   int fixup(Node* z)
   {
      Node* y;

      while (z->_parent && z->_parent->isRed())
      {
         if (z->_parent == z->_parent->_parent->_left)
         {
            y = z->_parent->_parent->_right;

            if (y && y->isRed())
            {
               z->_parent->setBlack();
               y->setBlack();
               z->_parent->_parent->setRed();
               z = z->_parent->_parent;
            }
            else
            {
               if (z == z->_parent->_right)
               {
                  z = z->_parent;
                  leftRotate(z);
               }

               z->_parent->setBlack();
               z->_parent->_parent->setRed();
               rightRotate(z->_parent->_parent);
            }
         }
         else
         {
            y = z->_parent->_parent->_left;

            if (y && y->isRed())
            {
               z->_parent->setBlack();
               y->setBlack();
               z->_parent->_parent->setRed();
               z = z->_parent->_parent;
            }
            else
            {
               if (z == z->_parent->_left)
               {
                  z = z->_parent;
                  rightRotate(z);
               }
               z->_parent->setBlack();
               z->_parent->_parent->setRed();
               leftRotate(z->_parent->_parent);
            }
         }
      }

      _root->setBlack();

      return 0;
   }

   // delete fixup
   int delete_fixup(Node* x)
   {
      Node* w;

      while (x != _root && x->isBlack())
      {
         if (x == x->_parent->_left)
         {
            w = x->_parent->_right;

            if (w->isRed())
            {
               w->setBlack();
               x->_parent->setRed();
               leftRotate(x->_parent);
               w = x->_parent->_right;
            }

            if (w->_left->isBlack() && w->_right->isBlack())
            {
               w->setRed();
               x = x->_parent;
            }
            else if (w->_right->isBlack())
            {
               w->_left->setBlack();
               w->setRed();
               rightRotate(w);
               w = x->_parent->_right;
            }

            w->setColor(x->_parent->color());
            x->_parent->setBlack();
            w->_right->setBlack();
            leftRotate(x->_parent);
            x = _root;
         }
         else
         {
            w = x->_parent->_left;

            if (w->isRed())
            {
               w->setBlack();
               x->_parent->setRed();
               rightRotate(x->_parent);
               w = x->_parent->_left;
            }

            if (w->_right->isBlack() && w->_left->isBlack())
            {
               w->setRed();
               x = x->_parent;
            }
            else if (w->_left->isBlack())
            {
               w->_right->setBlack();
               w->setRed();
               leftRotate(w);
               w = x->_parent->_left;
            }

            w->setColor(x->_parent->color());
            x->_parent->setBlack();
            w->_left->setBlack();
            rightRotate(x->_parent);
            x = _root;
         }
      }

      x->setBlack();

      return 0;
   }

   // in-order traverse
   void traverseValues(Node* n, Array<ValueType>& list) const
   {
      if (n != nullptr)
      {
         traverseValues(n->_left, list);
         list.add(n->_value);
         traverseValues(n->_right, list);
      }
   }

   // in-order traverse
   void traverseKeys(Node* n, Array<KeyType>& list) const
   {
      if (n != nullptr)
      {
         traverseKeys(n->_left, list);
         list.add(n->_key);
         traverseKeys(n->_right, list);
      }
   }

   // recursive delete of all nodes
   void postOrderDelete(Node* n)
   {
      if (n != nullptr)
      {
         postOrderDelete(n->_left);
         postOrderDelete(n->_right);
         delete n;
      }
   }

private:
   Node* _root = nullptr;
};

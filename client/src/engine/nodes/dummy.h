// dummy object
// just contains a transformation, thus describing an oriented point in space
// usually represents bones (and objects the exporter couldn't otherwise identify)

#pragma once

#include "node.h"

class Stream;

class Dummy : public Node
{
public:
   Dummy(Node* parent = nullptr);
   void load(Stream* stream) override;
   void write(Stream* stream) override;
};

#pragma once

#include "light.h"

class DirLight : public Light
{
public:
   DirLight(Node* parent = nullptr);
};

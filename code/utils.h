#pragma once
#include "long_march.h"
#include "set"

using namespace long_march;

class Application;

struct Buffer;

template <class Ty>
class StaticBuffer;

template <class Ty>
class DynamicBuffer;

class DynamicBufferBase;

void IgnoreResult(VkResult result);

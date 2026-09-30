#pragma once

#include "astra/ui/item/menu/menu.h"

class AstraGlassServices;
class AstraDinoJumpController;

struct AstraPortPages {
    astra::Tile *root = nullptr;
    astra::List *secondPage = nullptr;
    astra::List *cameraPage = nullptr;
    astra::List *audioPage = nullptr;
    astra::List *radioPage = nullptr;
    astra::List *loraPage = nullptr;
    astra::List *networkPage = nullptr;
    astra::List *gamePage = nullptr;
    astra::List *devicePage = nullptr;
};

AstraPortPages buildAstraPortPages(AstraGlassServices &services);

AstraPortPages buildAstraPortPages(AstraGlassServices &services,
                                   AstraDinoJumpController &dinoJump);

AstraPortPages buildAstraPortPages(bool &test,
                                   unsigned char &testIndex,
                                   unsigned char &testSlider);

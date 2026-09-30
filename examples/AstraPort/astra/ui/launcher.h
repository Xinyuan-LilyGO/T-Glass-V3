//
// Created by Fir on 2024/2/2.
//
#pragma once
#ifndef ASTRA_CORE_SRC_ASTRA_UI_SCHEDULER_H_
#define ASTRA_CORE_SRC_ASTRA_UI_SCHEDULER_H_

#include "item/menu/menu.h"
#include "item/selector/selector.h"
#include "item/camera/camera.h"
#include "../../AstraGestureControl.h"

namespace astra {

class Launcher {
private:
  Menu* currentMenu;
  Widget* currentWidget = nullptr;
  Selector* selector;
  Camera* camera;

  uint64_t time = 0;

public:
  void popInfo(std::string _info, uint16_t _time);

  void init(Menu* _rootPage);

  bool open();
  bool close();
  void applyGestureAction(astra_gesture_control::Action action);

  void update(bool present = true);

  Camera* getCamera() { return camera; }
  Selector* getSelector() { return selector; }
};
}

#endif //ASTRA_CORE_SRC_ASTRA_UI_SCHEDULER_H_

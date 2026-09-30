//
// Created by Fir on 2024/2/2.
//

#include "launcher.h"

namespace astra {

void Launcher::popInfo(std::string _info, uint16_t _time) {
  static bool init = false;
  static unsigned long long int beginTime = this->time;;
  static bool onRender = false;

  if (!init) {
    init = true;
    beginTime = this->time;
    onRender = true;
  }

  float wPop = HAL::getFontWidth(_info) + 2 * getUIConfig().popMargin;  //宽度
  float hPop = HAL::getFontHeight() + 2 * getUIConfig().popMargin;  //高度
  float yPop = 0 - hPop - 8; //从屏幕上方滑入
  float yPopTrg = (HAL::getSystemConfig().screenHeight - hPop) / 3;  //目标位置 中间偏上
  float xPop = (HAL::getSystemConfig().screenWeight - wPop) / 2;  //居中

  while (onRender) {
    time++;

    HAL::canvasClear();
    /*渲染一帧*/
    currentMenu->render(camera->getPosition());
    selector->render(camera->getPosition());
    camera->update(currentMenu, selector);
    /*渲染一帧*/

    HAL::setDrawType(0);
    HAL::drawRBox(xPop - 4, yPop - 4, wPop + 8, hPop + 8, getUIConfig().popRadius + 2);
    HAL::setDrawType(1);  //反色显示
    HAL::drawRFrame(xPop - 1, yPop - 1, wPop + 2, hPop + 2, getUIConfig().popRadius);  //绘制一个圆角矩形
    HAL::drawChinese(xPop + getUIConfig().popMargin,
                     yPop + getUIConfig().popMargin + HAL::getFontHeight(),
                     _info);  //绘制文字

    HAL::canvasUpdate();

    Animation::move(&yPop, yPopTrg, getUIConfig().popSpeed);  //动画

    //这里条件可以加上一个如果按键按下 就滑出
    if (time - beginTime >= _time) yPopTrg = 0 - hPop - 8;  //滑出

    HAL::keyScan();
    if (HAL::getAnyKey()) {
      for (unsigned char i = 0; i < key::KEY_NUM; i++)
        if (HAL::getKeyMap()[i] == key::CLICK) yPopTrg = 0 - hPop - 8;  //滑出
      std::fill(HAL::getKeyMap(), HAL::getKeyMap() + key::KEY_NUM, key::INVALID);
    }

    if (yPop == 0 - hPop - 8) {
      onRender = false;  //退出条件
      init = false;
    }
  }
}

void Launcher::init(Menu *_rootPage) {
  if (_rootPage == nullptr) return;

  currentMenu = _rootPage;

  camera = new Camera(0, 0);
  _rootPage->childPosInit(camera->getPosition());

  selector = new Selector();
  selector->inject(_rootPage);

  camera->init(_rootPage->getType());
  currentMenu->invokeEnterCallback();
}

/**
 * @brief 打开选中的页面
 *
 * @return 是否成功打开
 * @warning 仅可调用一次
 */
bool Launcher::open() {

  if (currentMenu == nullptr || currentMenu->getItemNum() == 0) {
    return false;
  }

  //如果当前页面指向的当前item没有后继 那就返回false
  Menu *nextMenu = currentMenu->getNextMenu();
  if (nextMenu == nullptr) {
    popInfo("unreferenced page!", 600);
    return false;
  }
  if (nextMenu->hasAction()) {
    const char *message = nextMenu->invokeAction();
    if (message != nullptr) popInfo(message, 600);
    return true;
  }
  if (nextMenu->getItemNum() == 0 && !nextMenu->canOpenWithoutChildren()) {
    popInfo("empty page!", 600);
    return false;
  }

  currentMenu->rememberCameraPos(camera->getPositionTrg());

  currentMenu->invokeExitCallback();
  currentMenu->deInit();  //先析构（退场动画）再挪动指针

  currentMenu = nextMenu;
  currentMenu->forePosInit();
  currentMenu->childPosInit(camera->getPosition());
  currentMenu->invokeEnterCallback();

  selector->inject(currentMenu);
  //selector->go(currentPage->selectIndex);

  return true;
}

/**
 * @brief 关闭选中的页面
 *
 * @return 是否成功关闭
 * @warning 仅可调用一次
 */
bool Launcher::close() {
  if (currentMenu == nullptr) return false;

  if (currentMenu->getPreview() == nullptr) {
    popInfo("unreferenced page!", 600);
    return false;
  }

  currentMenu->rememberCameraPos(camera->getPositionTrg());

  currentMenu->invokeExitCallback();
  currentMenu->deInit();  //先析构（退场动画）再挪动指针

  currentMenu = currentMenu->getPreview();
  currentMenu->forePosInit();
  currentMenu->childPosInit(camera->getPosition());
  currentMenu->invokeEnterCallback();

  selector->inject(currentMenu);
  //selector->go(currentPage->selectIndex);

  return true;
}

void Launcher::applyGestureAction(astra_gesture_control::Action action) {
  if (currentMenu == nullptr) return;

  if (astra_gesture_control::shouldCloseLeafPage(action,
                                                  currentMenu->getItemNum() != 0)) {
    if (!close()) return;
    // Opening the leaf page again would immediately re-enter the same status page.
    if (action == astra_gesture_control::Action::Open) return;
  }

  switch (action) {
    case astra_gesture_control::Action::Previous:
      if (selector != nullptr) selector->goPreview();
      break;
    case astra_gesture_control::Action::Next:
      if (selector != nullptr) selector->goNext();
      break;
    case astra_gesture_control::Action::Open:
      open();
      break;
    case astra_gesture_control::Action::Close:
      close();
      break;
    case astra_gesture_control::Action::None:
    case astra_gesture_control::Action::TogglePlayback:
    default:
      break;
  }
}

void Launcher::update(bool present) {
  HAL::canvasClear();

  currentMenu->render(camera->getPosition());
  if (currentWidget != nullptr) currentWidget->render(camera->getPosition());
  selector->render(camera->getPosition());
  camera->update(currentMenu, selector);

//  if (time == 500) selector->go(3);  //test
//  if (time == 800) open();  //test
//  if (time == 1200) selector->go(0);  //test
//  if (time == 1500) selector->go(1z);  //test
//  if (time == 1800) selector->go(6);  //test
//  if (time == 2100) selector->go(1);  //test
//  if (time == 2300) selector->go(0);  //test
//  if (time == 2500) open();  //test
//  if (time == 2900) close();
//  if (time == 3200) selector->go(0);  //test
//  if (time >= 3250) time = 0;  //test

  HAL::keyScan();

  if (*HAL::getKeyFlag() == key::KEY_PRESSED) {
    *HAL::getKeyFlag() = key::KEY_NOT_PRESSED;
    for (unsigned char i = 0; i < key::KEY_NUM; i++) {
      if (HAL::getKeyMap()[i] == key::CLICK) {
        if (!currentMenu->invokeClickCallback(i)) {
          if (i == 0) { selector->goPreview(); }//selector去到上一个项目
          else if (i == 1) { selector->goNext(); }//selector去到下一个项目
        }
      } else if (HAL::getKeyMap()[i] == key::PRESS) {
        if (i == 0) { close(); }//退出当前项目
        else if (i == 1) { open(); }//打开当前项目
      }
    }
    std::fill(HAL::getKeyMap(), HAL::getKeyMap() + key::KEY_NUM, key::INVALID);
    *HAL::getKeyFlag() = key::KEY_NOT_PRESSED;
  }

  if (present) HAL::canvasUpdate();

  time = HAL::millis();
}
}

//
// Created by Fir on 2024/1/21.
//

//每个列表应该有 1. 文本（需要显示的内容） 2. 跳转向量表（对应1，每一行都要跳转到哪里）
//其中文本以string形式 向量表以vector形式 以类的形式整合在一起 编写相应的构造函数 调用构造函数即是新建一个页面
//通过最后一项跳转向量表可以判断其类型 比如跳转至下一页 或者返回上一级 或者是复选框 或者是弹窗 或者是侧边栏弹窗 等等
//做一个astra类作为总的框架 在ui命名空间里

//分为
//驱动层 - 界面层（包括各种列表 磁铁 复选框 侧边栏 弹窗等） - 处理层（各种坐标的变换等）

//传进去的有三个东西 第一个是文字（要显示的东西） 第二个是类别（普通列表 跳转列表 复选框） 第三个是特征向量（指向复选框和弹窗对应的参数 要跳转的地方等）

#include "menu.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

constexpr std::uint16_t kListMarqueeWidth = 104;
constexpr std::uint32_t kListMarqueeStepMs = 350;

std::size_t nextUtf8CodePoint(const std::string &text, std::size_t offset) {
  if (offset >= text.size()) return text.size();

  const unsigned char first = static_cast<unsigned char>(text[offset]);
  std::size_t length = 1;
  if ((first & 0x80U) == 0) {
    length = 1;
  } else if ((first & 0xE0U) == 0xC0U) {
    length = 2;
  } else if ((first & 0xF0U) == 0xE0U) {
    length = 3;
  } else if ((first & 0xF8U) == 0xF0U) {
    length = 4;
  }
  return std::min(offset + length, text.size());
}

std::size_t utf8CodePointCount(const std::string &text) {
  std::size_t count = 0;
  for (std::size_t offset = 0; offset < text.size(); ++count) {
    offset = nextUtf8CodePoint(text, offset);
  }
  return count;
}

std::size_t utf8ByteOffset(const std::string &text, std::size_t codePoint) {
  std::size_t offset = 0;
  while (codePoint-- > 0 && offset < text.size()) {
    offset = nextUtf8CodePoint(text, offset);
  }
  return offset;
}

std::uint16_t textWidth(const std::string &text) {
  std::string measured = text;
  return HAL::getFontWidth(measured);
}

std::string makeMarqueeText(const std::string &text) {
  const std::size_t codePointCount = utf8CodePointCount(text);
  if (codePointCount == 0 || textWidth(text) <= kListMarqueeWidth) return text;

  constexpr std::size_t kGapCodePoints = 3;
  const std::size_t cycleLength = codePointCount + kGapCodePoints;
  const std::size_t startCodePoint =
      (static_cast<std::uint32_t>(HAL::millis()) / kListMarqueeStepMs) % cycleLength;
  std::size_t sourceOffset = utf8ByteOffset(text, startCodePoint);
  std::string result;
  std::size_t cyclePosition = startCodePoint;

  for (std::size_t rendered = 0; rendered < cycleLength; ++rendered) {
    std::string unit;
    if (cyclePosition < codePointCount) {
      const std::size_t nextOffset = nextUtf8CodePoint(text, sourceOffset);
      unit = text.substr(sourceOffset, nextOffset - sourceOffset);
      sourceOffset = nextOffset;
    } else {
      unit = " ";
    }

    std::string candidate = result + unit;
    if (textWidth(candidate) > kListMarqueeWidth) break;
    result.swap(candidate);

    cyclePosition = (cyclePosition + 1) % cycleLength;
    if (cyclePosition == 0) sourceOffset = 0;
  }
  return result.empty() ? text.substr(0, nextUtf8CodePoint(text, 0)) : result;
}

} // namespace

/**
 *     ·  ·     ·   ·
 *  ·   ·  ·  ·   ·
 *  循此苦旅，直抵群星。
 *  ad astra per aspera.
 * happy Chinese new year!
 *      新年快乐！
 * ·   ·   ·      ·
 *   ·   ·    · ·   ·
 */

namespace astra {
Menu::Position Menu::getItemPosition(unsigned char _index) const { return childMenu[_index]->position; }

std::vector<unsigned char> Menu::generateDefaultPic() {
  return std::vector<unsigned char>(120, 0xFF);
}

unsigned char Menu::getItemNum() const { return childMenu.size(); }

Menu *Menu::getNextMenu() const {
  if (childMenu.empty() || selectIndex >= childMenu.size()) return nullptr;
  return childMenu[selectIndex];
}

Menu *Menu::getPreview() const { return parent; }

void Menu::setAction(ActionCallback callback, void *context) {
  actionCallback_ = callback;
  actionContext_ = context;
}

bool Menu::hasAction() const {
  return actionCallback_ != nullptr;
}

const char *Menu::invokeAction() const {
  if (actionCallback_ == nullptr) return nullptr;
  return actionCallback_(actionContext_);
}

void Menu::setClickCallback(ClickCallback callback, void *context) {
  clickCallback_ = callback;
  clickContext_ = context;
}

bool Menu::invokeClickCallback(unsigned char keyIndex) const {
  if (clickCallback_ == nullptr) return false;
  return clickCallback_(clickContext_, keyIndex);
}

void Menu::setMarqueeEnabled(bool enabled) {
  marqueeEnabled_ = enabled;
  marqueeSourceWidth_ = enabled && marqueeText_.empty() ? textWidth(displayTitle()) : 0;
  marqueeFrameStep_ = 0xFFFFFFFFU;
  marqueeFrame_.clear();
  if (!enabled) marqueeText_.clear();
}

void Menu::setMarqueeText(const std::string &text) {
  marqueeText_ = text;
  marqueeSourceWidth_ = textWidth(marqueeText_);
  marqueeFrameStep_ = 0xFFFFFFFFU;
  marqueeFrame_.clear();
}

bool Menu::marqueeEnabled() const {
  return marqueeEnabled_;
}

const std::string &Menu::marqueeText() const {
  return marqueeText_;
}

const std::string &Menu::currentMarqueeText() {
  const std::string &source = marqueeText_.empty() ? displayTitle() : marqueeText_;
  if (marqueeSourceWidth_ <= kListMarqueeWidth) return source;

  const std::uint32_t frameStep =
      static_cast<std::uint32_t>(HAL::millis()) / kListMarqueeStepMs;
  if (marqueeFrameStep_ != frameStep) {
    marqueeFrame_ = makeMarqueeText(source);
    marqueeFrameStep_ = frameStep;
  }
  return marqueeFrame_;
}

void Menu::setLocalizedTitle(const std::string &nativeTitle,
                             const std::string &englishTitle,
                             LanguageCallback callback,
                             void *context) {
  title = nativeTitle;
  englishTitle_ = englishTitle;
  languageCallback_ = callback;
  languageContext_ = context;
  marqueeSourceWidth_ = marqueeEnabled_ && marqueeText_.empty() ? textWidth(displayTitle()) : 0;
  marqueeFrameStep_ = 0xFFFFFFFFU;
  marqueeFrame_.clear();
}

const std::string &Menu::displayTitle() const {
  if (languageCallback_ != nullptr && languageCallback_(languageContext_)) {
    return englishTitle_;
  }
  return title;
}

void Menu::setEnterCallback(LifecycleCallback callback, void *context) {
  enterCallback_ = callback;
  enterContext_ = context;
}

void Menu::setExitCallback(LifecycleCallback callback, void *context) {
  exitCallback_ = callback;
  exitContext_ = context;
}

void Menu::invokeEnterCallback() const {
  if (enterCallback_ != nullptr) enterCallback_(enterContext_);
}

void Menu::invokeExitCallback() const {
  if (exitCallback_ != nullptr) exitCallback_(exitContext_);
}

void Menu::init(const std::vector<float>& _camera) { }

void Menu::deInit() {
  //todo 未实现完全
  Animation::exit();
}

bool Menu::addItem(Menu *_page) {
  if (_page == nullptr) return false;
  if (!_page->childWidget.empty()) return false;

    _page->parent = this;
    this->childMenu.push_back(_page);
    this->forePosInit();
    return true;
}

bool Menu::addItem(Menu *_page, Widget *_anyWidget) {
  if (_anyWidget == nullptr) return false;
  if (this->addItem(_page)) {
    _page->childWidget.push_back(_anyWidget);
    _anyWidget->parent = _page;
    _anyWidget->init();
    return true;
  } else return false;
}

void List::childPosInit(const std::vector<float> &_camera) {
  unsigned char _index = 0;

  for (auto _iter : childMenu) {
    _iter->position.x = astraConfig.listTextMargin;
    _iter->position.xTrg = astraConfig.listTextMargin;
    _iter->position.yTrg = _index * astraConfig.listLineHeight;

    _index++;

    //受展开开关影响的坐标初始化
    //根页面有开场动画 所以不需要从头展开
    if (_iter->parent->parent == nullptr) { _iter->position.y = _iter->position.yTrg; continue; }
    if (astraConfig.listUnfold) { _iter->position.y = _camera[1] - astraConfig.listLineHeight;
      continue; } //text unfold from top.
  }
}

void List::forePosInit() {
  positionForeground.xBarTrg = systemConfig.screenWeight - astraConfig.listBarWeight;

  //受展开开关影响的坐标初始化
  if (astraConfig.listUnfold) positionForeground.hBar = 0;  //bar unfold from top.
  else positionForeground.hBar = positionForeground.hBarTrg;

  //始终执行的坐标初始化
  positionForeground.xBar = systemConfig.screenWeight;
}

List::List() {
  this->title = "-unknown";
  this->pic = generateDefaultPic();

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

List::List(const std::string &_title) {
  this->title = _title;
  this->pic = generateDefaultPic();

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

List::List(const std::vector<unsigned char> &_pic) {
  this->title = "-unknown";
  this->pic = _pic;

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

List::List(const std::string &_title, const std::vector<unsigned char> &_pic) {
  this->title = _title;
  this->pic = _pic;

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

void List::render(const std::vector<float> &_camera) {
  Item::updateConfig();

  if (childMenu.empty()) return;

  HAL::setDrawType(1);
  //allow x > screen height, y > screen weight.
  //scan all children, draw text and widget on the list.
  for (std::size_t index = 0; index < childMenu.size(); ++index) {
    auto *_iter = childMenu[index];
    //绘制控件在列表中的指示器
    if (!_iter->childWidget.empty()) {
      for (auto _widget : _iter->childWidget) {
        _widget->renderIndicator(
            systemConfig.screenWeight - astraConfig.checkBoxRightMargin - astraConfig.checkBoxWidth,
            _iter->position.y + astraConfig.checkBoxTopMargin,
            _camera);
      }
    }
    //绘制文字
    const std::string *title = &_iter->displayTitle();
    if (index == selectIndex && _iter->marqueeEnabled()) {
      title = &_iter->currentMarqueeText();
    }
    HAL::drawChinese(_iter->position.x + _camera[0],
                     _iter->position.y + astraConfig.listTextHeight +
                     astraConfig.listTextMargin + _camera[1],
                     *title);
    //这里的yTrg在addItem的时候就已经确定了
    Animation::move(&_iter->position.y, _iter->position.yTrg, astraConfig.listAnimationSpeed);
  }

  //draw bar.
  positionForeground.hBarTrg = (selectIndex + 1) * ((float) systemConfig.screenHeight / getItemNum());
  //画指示线
  HAL::drawHLine(systemConfig.screenWeight - astraConfig.listBarWeight, 0, astraConfig.listBarWeight);
  HAL::drawHLine(systemConfig.screenWeight - astraConfig.listBarWeight,
                 systemConfig.screenHeight - 1,
                 astraConfig.listBarWeight);
  HAL::drawVLine(systemConfig.screenWeight - ceil((float) astraConfig.listBarWeight / 2.0f),
                 0,
                 systemConfig.screenHeight);
  //draw bar.
  HAL::drawBox(positionForeground.xBar, 0, astraConfig.listBarWeight, positionForeground.hBar);

  //light mode.
  if (astraConfig.lightMode) {
    HAL::setDrawType(2);
    HAL::drawBox(0, 0, systemConfig.screenWeight, systemConfig.screenHeight);
    HAL::setDrawType(1);
  }

  Animation::move(&positionForeground.hBar, positionForeground.hBarTrg, astraConfig.listAnimationSpeed);
  Animation::move(&positionForeground.xBar, positionForeground.xBarTrg, astraConfig.listAnimationSpeed);
}

void Tile::childPosInit(const std::vector<float> &_camera) {
  unsigned char _index = 0;

  for (auto _iter : childMenu) {
    _iter->position.y = 0;
    _iter->position.xTrg = systemConfig.screenWeight / 2 - astraConfig.tilePicWidth / 2 +
                           (_index) * (astraConfig.tilePicMargin + astraConfig.tilePicWidth);
    _iter->position.yTrg = astraConfig.tilePicTopMargin;

    _index++;

    if (_iter->parent->parent == nullptr) { _iter->position.x = _iter->position.xTrg; continue; }
    if (astraConfig.tileUnfold) { _iter->position.x = _camera[0] - astraConfig.tilePicWidth; continue; } //unfold from left.
  }
}

void Tile::forePosInit() {
  positionForeground.yBarTrg = 0;
  positionForeground.yArrowTrg = systemConfig.screenHeight - astraConfig.tileArrowBottomMargin;
  positionForeground.yDottedLineTrg = systemConfig.screenHeight - astraConfig.tileDottedLineBottomMargin;

  if (astraConfig.tileUnfold) positionForeground.wBar = 0;  //bar unfold from left.
  else positionForeground.wBar = positionForeground.wBarTrg;

  //position.y = -astraConfig.tilePicHeight * 2;

  //始终执行的坐标初始化
  //底部箭头和虚线的初始化
  positionForeground.yArrow = systemConfig.screenHeight;
  positionForeground.yDottedLine = systemConfig.screenHeight;

  //顶部进度条的从上方滑入的初始化
  positionForeground.yBar = 0 - astraConfig.tileBarHeight; //注意这里是坐标从屏幕外滑入 而不是height从0变大
}

Tile::Tile() {
  this->title = "-unknown";
  this->pic = generateDefaultPic();

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

Tile::Tile(const std::string &_title) {
  this->title = _title;
  this->pic = generateDefaultPic();

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

Tile::Tile(const std::vector<unsigned char> &_pic) {
  this->title = "-unknown";
  this->pic = _pic;

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

Tile::Tile(const std::string &_title, const std::vector<unsigned char> &_pic) {
  this->title = _title;
  this->pic = _pic;

  this->selectIndex = 0;

  this->parent = nullptr;
  this->childMenu.clear();
  this->childWidget.clear();

  this->position = {};
  this->positionForeground = {};
}

void Tile::render(const std::vector<float> &_camera) {
  Item::updateConfig();

  if (childMenu.empty()) return;

  HAL::setDrawType(1);
  //draw pic.
  for (auto _iter : childMenu) {
    HAL::drawBMP(_iter->position.x + _camera[0],
                 astraConfig.tilePicTopMargin + _camera[1],
                 astraConfig.tilePicWidth,
                 astraConfig.tilePicHeight,
                 _iter->pic.data());
    //这里的xTrg在addItem的时候就已经确定了
    Animation::move(&_iter->position.x,
                    _iter->position.xTrg,
                    astraConfig.tileAnimationSpeed);
  }

  //draw bar.
  //在屏幕最上方 两个像素高
  positionForeground.wBarTrg = (selectIndex + 1) * ((float) systemConfig.screenWeight / getItemNum());
  HAL::drawBox(0, positionForeground.yBar, positionForeground.wBar, astraConfig.tileBarHeight);

  //draw left arrow.
  HAL::drawHLine(astraConfig.tileArrowMargin, positionForeground.yArrow, astraConfig.tileArrowWidth);
  HAL::drawPixel(astraConfig.tileArrowMargin + 1, positionForeground.yArrow + 1);
  HAL::drawPixel(astraConfig.tileArrowMargin + 2, positionForeground.yArrow + 2);
  HAL::drawPixel(astraConfig.tileArrowMargin + 1, positionForeground.yArrow - 1);
  HAL::drawPixel(astraConfig.tileArrowMargin + 2, positionForeground.yArrow - 2);

  //draw right arrow.
  HAL::drawHLine(systemConfig.screenWeight - astraConfig.tileArrowWidth - astraConfig.tileArrowMargin,
                 positionForeground.yArrow,
                 astraConfig.tileArrowWidth);
  HAL::drawPixel(systemConfig.screenWeight - astraConfig.tileArrowWidth, positionForeground.yArrow + 1);
  HAL::drawPixel(systemConfig.screenWeight - astraConfig.tileArrowWidth - 1, positionForeground.yArrow + 2);
  HAL::drawPixel(systemConfig.screenWeight - astraConfig.tileArrowWidth, positionForeground.yArrow - 1);
  HAL::drawPixel(systemConfig.screenWeight - astraConfig.tileArrowWidth - 1, positionForeground.yArrow - 2);

  //draw left button.
  HAL::drawHLine(astraConfig.tileBtnMargin, positionForeground.yArrow + 2, 9);
  HAL::drawBox(astraConfig.tileBtnMargin + 2, positionForeground.yArrow + 2 - 4, 5, 4);

  //draw the right button.
  HAL::drawHLine(systemConfig.screenWeight - astraConfig.tileBtnMargin - 9, positionForeground.yArrow + 2, 9);
  HAL::drawBox(systemConfig.screenWeight - astraConfig.tileBtnMargin - 9 + 2,
               positionForeground.yArrow + 2 - 4,
               5,
               4);

  //draw dotted line.
  HAL::drawHDottedLine(0, positionForeground.yDottedLine, systemConfig.screenWeight);

  Animation::move(&positionForeground.yDottedLine, positionForeground.yDottedLineTrg, astraConfig.tileAnimationSpeed);
  Animation::move(&positionForeground.yArrow, positionForeground.yArrowTrg, astraConfig.tileAnimationSpeed);
  Animation::move(&positionForeground.wBar, positionForeground.wBarTrg, astraConfig.tileAnimationSpeed);
  Animation::move(&positionForeground.yBar, positionForeground.yBarTrg, astraConfig.tileAnimationSpeed);
}
}

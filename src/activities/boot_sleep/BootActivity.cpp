#include "BootActivity.h"

#include <GfxRenderer.h>

#include "LanguageMapper.h"
#include "fontIds.h"
#include "images/RabbitLarge.h"  // HelloRuru 兔子 Logo

void BootActivity::onEnter() {
  Activity::onEnter();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  // Miranda: 整個 logo+caption+status cluster 上移 40px
  //          原本 cluster 視覺中心比幾何中心低 23px、讀起來有「掉下去」感
  //          上移後視覺中心對齊面板光學中心
  const int CLUSTER_OFFSET_Y = -40;
  const int rabbitTopY = (pageHeight - 128) / 2 + CLUSTER_OFFSET_Y;
  renderer.drawImage(RabbitLarge, (pageWidth - 128) / 2, rabbitTopY, 128, 128);
  // BOLD 品牌名 + Regular 狀態文字 = mono 層級
  renderer.drawCenteredText(UI_10_FONT_ID, rabbitTopY + 128 + 12, "RURU-READER-SUPER", true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(SMALL_FONT_ID, rabbitTopY + 128 + 36, getChineseName("BOOTING"));
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight - 30, CROSSPOINT_VERSION);
  renderer.displayBuffer();
}

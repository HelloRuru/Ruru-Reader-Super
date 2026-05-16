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
  // RuruSuper: 中央兔子 + RURU-READER-SUPER 文字
  renderer.drawImage(RabbitLarge, (pageWidth - 128) / 2, (pageHeight - 128) / 2, 128, 128);
  renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2 + 70, "RURU-READER-SUPER", true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight / 2 + 95, getChineseName("BOOTING"));
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight - 30, CROSSPOINT_VERSION);
  renderer.displayBuffer();
}

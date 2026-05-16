#include "LyraFlowTheme.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <SDCardManager.h>  // ChineseType 用 SdMan 取代 Carousel 的 Storage

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "RecentBooksStore.h"
// #include "activities/reader/BookReadingStats.h"  // ChineseType 沒此系統，stats 參數恆為 nullptr
#include "components/UITheme.h"
#include "components/icons/book.h"
#include "components/icons/chart.h"
#include "components/icons/cover.h"
#include "components/icons/folder.h"
#include "components/icons/hotspot.h"
#include "components/icons/library.h"
#include "components/icons/recent.h"
#include "components/icons/settings2.h"
#include "components/icons/transfer.h"
#include "components/icons/wifi.h"
#include "fontIds.h"

namespace {
// stage15.25 (修正螢幕尺寸誤判):
//   螢幕實際是 480×800 不是 480×540！之前佈局只用到 540 → 下方 260px 空白
//   現在用真實 800px、cover 拉大 +25%（240→300）、整體有大量呼吸
constexpr int centerCoverWidth = 210;
constexpr int centerCoverHeight = 300;
// Miranda Cover Flow 方案 A: 每邊三本（近/中/遠），深度遞減
constexpr int sideCoverWidth = 58;
constexpr int sideInnerHeight = 270;   // 近
constexpr int sideMidHeight = 240;     // 中（介於近遠之間）
constexpr int sideOuterHeight = 200;   // 遠（比舊版 240 更傾斜，深度感更強）
constexpr int bookCornerRadius = 6;

// Menu visuals — kept in sync with LyraTheme's anonymous-namespace constants
// so the Flow override looks identical to the parent's button menu.
constexpr int menuTileCornerRadius = 6;
constexpr int menuTilePadding = 8;
constexpr int menuIconSize = 32;

// Same lookup as LyraTheme's iconForName(icon, 32). Duplicated here because
// that helper is file-local to LyraTheme.cpp.
const uint8_t* lyraFlowMenuIcon(UIIcon icon) {
  switch (icon) {
    case UIIcon::Folder:
      return FolderIcon;
    case UIIcon::Book:
      return BookIcon;
    case UIIcon::Chart:
      return ChartIcon;
    case UIIcon::Recent:
      return RecentIcon;
    case UIIcon::Settings:
      return Settings2Icon;
    case UIIcon::Transfer:
      return TransferIcon;
    case UIIcon::Library:
      return LibraryIcon;
    case UIIcon::Wifi:
      return WifiIcon;
    case UIIcon::Hotspot:
      return HotspotIcon;
    default:
      return nullptr;
  }
}

// Erase pixels outside a rounded-corner mask so a rectangular bitmap blit
// looks like a rounded book cover. Same trick as the reference FlowTheme.
void cutRoundedCorners(GfxRenderer& renderer, int x, int y, int w, int h, int r) {
  const int rSq = r * r;
  for (int dy = 0; dy < r; dy++) {
    for (int dx = 0; dx < r; dx++) {
      const int distSq = (r - dx) * (r - dx) + (r - dy) * (r - dy);
      if (distSq > rSq) {
        renderer.drawPixel(x + dx, y + dy, false);
        renderer.drawPixel(x + w - 1 - dx, y + dy, false);
        renderer.drawPixel(x + w - 1 - dx, y + h - 1 - dy, false);
        renderer.drawPixel(x + dx, y + h - 1 - dy, false);
      }
    }
  }
}
}  // namespace

void LyraFlowTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                        const int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                        bool& bufferRestored, std::function<bool()> storeCoverBuffer,
                                        const BookReadingStats* stats, float /*progressPercent*/) const {
  if (recentBooks.empty()) {
    // ChineseType 移植：drawEmptyRecents helper 在 Carousel upstream 還沒實作
    // 暫時什麼都不畫（空白首頁）
    return;
  }

  const int pageWidth = renderer.getScreenWidth();
  // stage15.25 (螢幕實際 800px、空間充裕):
  //   書名 pill 從 rect.y+15、約佔 30
  //   centerY = rect.y+80（書名跟封面之間呼吸 35px）
  //   封面 300 → 結束 rect.y+380
  //   brand 從 rect.y+420 開始（封面後呼吸 40px）
  const int centerY = rect.y + 80;
  const int centerX = pageWidth / 2;
  const int count = static_cast<int>(recentBooks.size());

  // selectorIndex >= count means HomeActivity has navigated past the books and
  // is highlighting a menu item; in that case we keep the carousel visible but
  // drop the selection border. HomeActivity may encode the preferred center as
  // (count + lastBookIndex) so the carousel keeps the user's place when they
  // pop into the menu. Decode if in range, otherwise fall back to book 0.
  const bool hasSelection = (selectorIndex >= 0 && selectorIndex < count);
  int curIdx = 0;
  if (hasSelection) {
    curIdx = selectorIndex;
  } else {
    const int decoded = selectorIndex - count;
    if (decoded >= 0 && decoded < count) curIdx = decoded;
  }

  // The carousel chrome (header date, footer hints, etc.) is drawn by
  // HomeActivity, not by us. We have nothing static to cache here, so just
  // honor the buffer-snapshot protocol the same way the other themes do.
  if (bufferRestored) {
    coverRendered = true;
    coverBufferStored = true;
  } else {
    coverRendered = true;
    coverBufferStored = storeCoverBuffer();
  }

  // --- Side covers (perspective-projected, drawn outside-in so the center
  //     can land cleanly on top of any near-book overlap) ---
  // Miranda Cover Flow: depth 0=near, 1=mid, 2=far (本邊離中央越遠越扁)
  auto drawStackedCover = [&](int idx, bool isLeft, int depth) {
    // 三段高度遞減模擬透視
    auto pickHeight = [](int d, bool inner) {
      if (d == 0) return inner ? sideInnerHeight : sideMidHeight;     // near
      if (d == 1) return inner ? sideMidHeight : sideOuterHeight;     // mid
      return inner ? sideOuterHeight : 170;                            // far（更扁）
    };
    const int hL = isLeft ? pickHeight(depth, true) : pickHeight(depth, false);
    const int hR = isLeft ? pickHeight(depth, false) : pickHeight(depth, true);
    const int hMax = std::max(hL, hR);
    // 三段 X 位置：near 80/335、mid 30/385、far -10/430（far 稍微切到螢幕邊）
    const int sideLeftXs[3]  = {80, 30, -10};
    const int sideRightXs[3] = {335, 385, 430};
    const int drawX = isLeft ? sideLeftXs[depth] : sideRightXs[depth];
    const int drawY = centerY + (centerCoverHeight / 2) - (hMax / 2);

    const std::string coverPath = UITheme::getCoverThumbPath(recentBooks[idx].coverBmpPath, centerCoverHeight);
    bool drawn = false;
    if (!coverPath.empty()) {
      FsFile file;
      if (SdMan.openFileForRead("HOME", coverPath, file)) {
        Bitmap bitmap(file);
        if (bitmap.parseHeaders() == BmpReaderError::Ok) {
          // drawPerspectiveBitmap is OR-style (only writes black), so any
          // white area of the cover would show through to whatever side
          // cover was drawn beneath us. Pre-clear the bbox to opaque white.
          renderer.fillRect(drawX, drawY, sideCoverWidth, hMax, false);
          renderer.drawPerspectiveBitmap(bitmap, drawX, drawY, sideCoverWidth, hL, hR);
          drawn = true;
        }
        file.close();
      }
    }
    if (!drawn) {
      // No cover — paint a trapezoidal silhouette so側邊書本還是有斜角輪廓
      // 不再用矩形 + return，避免少了 perspective slant 像方塊蓋章。
      const int topL_n = (hMax - hL) / 2;
      const int topR_n = (hMax - hR) / 2;
      const int botL_n = topL_n + hL - 1;
      const int botR_n = topR_n + hR - 1;
      const int rightX_n = drawX + sideCoverWidth - 1;
      const int xs[4] = {drawX, rightX_n, rightX_n, drawX};
      const int ys[4] = {drawY + topL_n, drawY + topR_n, drawY + botR_n, drawY + botL_n};
      renderer.fillPolygon(xs, ys, 4, /*black=*/true);
      return;  // 已經畫好梯形剪影
    }
    // 2px trapezoidal outline matching the perspective shape — keeps every
    // side book visibly framed so the center book reads as part of a row of
    // books, not a single floating cover. The trapezoid is column-centered
    // vertically inside the (sideCoverWidth × hMax) bbox.
    const int topL = (hMax - hL) / 2;
    const int topR = (hMax - hR) / 2;
    const int botL = topL + hL - 1;
    const int botR = topR + hR - 1;
    const int rightX = drawX + sideCoverWidth - 1;
    renderer.drawLine(drawX, drawY + topL, rightX, drawY + topR, 2, true);    // top edge (slanted)
    renderer.drawLine(drawX, drawY + botL, rightX, drawY + botR, 2, true);    // bottom edge (slanted)
    // Verticals use fillRect, not drawLine — drawLine ignores its thickness
    // arg for purely vertical strokes (x1 == x2), so the previous 4 px width
    // was rendering as 1 px regardless. fillRect gives explicit control.
    constexpr int verticalEdgeWidth = 2;
    renderer.fillRect(drawX, drawY + topL, verticalEdgeWidth, hL, true);                    // left edge
    renderer.fillRect(rightX - verticalEdgeWidth + 1, drawY + topR, verticalEdgeWidth, hR,  // right edge
                      true);
    // The bottom slant's perpendicular thickness leaks pixels into the two
    // rows starting just below the bbox bottom (the row at drawY + hMax
    // is part of the visible outline, so we leave it). Wipe rows hMax+1
    // and hMax+2 to catch the hangnail wherever it lands.
    renderer.fillRect(drawX, drawY + hMax + 1, sideCoverWidth, 2, false);
  };

  // Miranda Cover Flow: 每邊 3 本（近/中/遠），共 6 本 + 中央 1 本 = 7 本可見
  const int idx_lN = (curIdx + count - 1) % count;  // left-near
  const int idx_lM = (curIdx + count - 2) % count;  // left-mid
  const int idx_lF = (curIdx + count - 3) % count;  // left-far
  const int idx_rN = (curIdx + 1) % count;          // right-near
  const int idx_rM = (curIdx + 2) % count;          // right-mid
  const int idx_rF = (curIdx + 3) % count;          // right-far

  // 由外往內畫，遠的先、近的最後 — 才不會被近的書封遮到
  if (count >= 7) drawStackedCover(idx_lF, true, 2);   // left-far
  if (count >= 6) drawStackedCover(idx_rF, false, 2);  // right-far
  if (count >= 5) drawStackedCover(idx_lM, true, 1);   // left-mid
  if (count >= 4) drawStackedCover(idx_rM, false, 1);  // right-mid
  if (count >= 2) drawStackedCover(idx_lN, true, 0);   // left-near
  if (count >= 3) drawStackedCover(idx_rN, false, 0);  // right-near

  // --- Center cover. Peek the bitmap dimensions first so the slot, outline,
  //     and selection border match the cover's true aspect ratio (otherwise
  //     drawBitmap aspect-fits but our 220×320 chrome leaves a white sliver
  //     for narrower covers, e.g. 1720×2600 which is taller than 220:320). ---
  int actualCoverWidth = centerCoverWidth;
  int actualCoverHeight = centerCoverHeight;
  const std::string cp = UITheme::getCoverThumbPath(recentBooks[curIdx].coverBmpPath, centerCoverHeight);
  FsFile cf;
  const bool centerOpened = !cp.empty() && SdMan.openFileForRead("HOME", cp, cf);
  Bitmap centerBitmap(cf);
  bool centerParsed = false;
  if (centerOpened) {
    if (centerBitmap.parseHeaders() == BmpReaderError::Ok && centerBitmap.getWidth() > 0 &&
        centerBitmap.getHeight() > 0) {
      const int srcW = centerBitmap.getWidth();
      const int srcH = centerBitmap.getHeight();
      const float fitScale = std::min(static_cast<float>(centerCoverWidth) / static_cast<float>(srcW),
                                      static_cast<float>(centerCoverHeight) / static_cast<float>(srcH));
      actualCoverWidth = std::min(centerCoverWidth, static_cast<int>(std::round(srcW * fitScale)));
      actualCoverHeight = std::min(centerCoverHeight, static_cast<int>(std::round(srcH * fitScale)));
      centerParsed = true;
    }
  }

  const int cX = centerX - actualCoverWidth / 2;
  // Vertical-center within the original 320-tall slot in case a cover is wider
  // than tall (very rare in practice).
  const int actualY = centerY + (centerCoverHeight - actualCoverHeight) / 2;

  // Clear behind it so any side-cover overlap doesn't bleed through.
  renderer.fillRect(cX, actualY, actualCoverWidth, actualCoverHeight, false);

  if (centerParsed) {
    renderer.drawBitmap(centerBitmap, cX, actualY, actualCoverWidth, actualCoverHeight);
    cutRoundedCorners(renderer, cX, actualY, actualCoverWidth, actualCoverHeight, bookCornerRadius);
  } else {
    // Placeholder: black lower-2/3 with the cover icon, matches reference fallback.
    renderer.fillRoundedRect(cX, actualY + actualCoverHeight / 3, actualCoverWidth, 2 * actualCoverHeight / 3,
                             bookCornerRadius, false, false, true, true, Color::Black);
    renderer.drawIcon(CoverIcon, cX + actualCoverWidth / 2 - 16, actualY + actualCoverHeight / 2 - 16, 32, 32);
  }
  renderer.drawRoundedRect(cX, actualY, actualCoverWidth, actualCoverHeight, 2, bookCornerRadius, true);

  if (hasSelection) {
    renderer.drawRoundedRect(cX - 2, actualY - 2, actualCoverWidth + 4, actualCoverHeight + 4, 4,
                             bookCornerRadius + 2, true);
  }

  if (centerOpened) cf.close();
  (void)stats;  // BookReadingStats 之後再用，先 mute warning

  // ============================================================
  // RuruSuper: cover +25% + menu 置底
  //   書名 SMALL pill 在上、cover 300 在中、RURU-READER SMALL 在下、menu 置底
  //   字級用 SMALL_FONT_ID 省空間（書名跟 brand 都改 SMALL）
  // ============================================================
  //   書名 SMALL pill 在上、cover 300 在中、RURU-READER SMALL 在下、menu 置底
  //   字級用 SMALL_FONT_ID 省空間（書名跟 brand 都改 SMALL）
  // ============================================================
  (void)stats;

  // Miranda Cover Flow: 中央書封下方加閱讀進度條（細 2px、貼著 cover 底邊 + 8px gap）
  if (recentBooks[curIdx].progressPercent >= 0) {
    const int progBarY = actualY + actualCoverHeight + 8;
    const int progBarH = 2;
    const int progBarW = actualCoverWidth;
    const int progBarX = cX;
    const int progFillW = (progBarW * recentBooks[curIdx].progressPercent) / 100;
    renderer.fillRectDither(progBarX, progBarY, progBarW, progBarH, Color::LightGray);
    if (progFillW > 0) {
      renderer.fillRect(progBarX, progBarY, progFillW, progBarH, true);
    }
  }

  // 取書名
  std::string filename = recentBooks[curIdx].title.empty() ? recentBooks[curIdx].path : recentBooks[curIdx].title;
  if (recentBooks[curIdx].title.empty()) {
    const size_t lastSlash = filename.find_last_of('/');
    if (lastSlash != std::string::npos) filename = filename.substr(lastSlash + 1);
    const size_t lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos && lastDot > 0) filename = filename.substr(0, lastDot);
  }

  // 書名 UI_10 pill（cover 上方、800px 螢幕空間充裕、用大字級）
  const std::string truncatedTitle =
      renderer.truncatedText(UI_10_FONT_ID, filename.c_str(), pageWidth - 80, EpdFontFamily::BOLD);
  const int titleWidth = renderer.getTextWidth(UI_10_FONT_ID, truncatedTitle.c_str(), EpdFontFamily::BOLD);
  const int titleH = renderer.getLineHeight(UI_10_FONT_ID);
  const int titlePadX = 18;
  const int titlePadY = 7;  // stage15.25: 大字級配大 padding
  const int titleBgW = titleWidth + titlePadX * 2;
  const int titleBgH = titleH + titlePadY * 2;
  const int titleBgX = centerX - titleBgW / 2;
  const int titleBgY = rect.y + 15;  // stage15.25: 頂部留更多呼吸
  renderer.fillRoundedRect(titleBgX, titleBgY, titleBgW, titleBgH, 8, Color::Black);
  renderer.drawText(UI_10_FONT_ID, titleBgX + titlePadX, titleBgY + titlePadY, truncatedTitle.c_str(),
                    false, EpdFontFamily::BOLD);

  // stage15.26: 嚕寶要把 RURU-READER / HAPPY READING! 那塊整段刪掉
  //   首頁底部就只剩 menu、cover 下方乾淨
}

void LyraFlowTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                   const std::function<std::string(int index)>& buttonLabel,
                                   const std::function<UIIcon(int index)>& rowIcon) const {
  // stage15.17 + 15.18: 改成 2x2 grid + icon
  //   嚕寶要的 4 格：挑選一本書 / 最近閱讀 / Wifi / 設定（4 個主要功能）
  //   每格佔半寬、上半畫 icon、下半畫文字、選中那格加圓角灰底
  //   stage15.18: HomeActivity 給的 rect.height 算錯（會超出螢幕），自己 clamp
  if (buttonCount >= 4) {
    const int gridCols = 2;
    const int gridRows = 2;
    const int gap = 8;
    const int sidePad = 24;
    const int topOffset = 0;   // stage15.24: 拿掉內部 topOffset、menu 完全置底
    constexpr int iconSize = 32;
    constexpr int iconLabelGap = 4;

    // 算可用空間：用螢幕高度減 rect.y 減底部按鈕提示區（40）當實際可用範圍
    // 避免 HomeActivity 給的 rect.height 算錯導致 cell 超出螢幕
    const int screenH = renderer.getScreenHeight();
    const int bottomReserve = 10;  // stage15.20: ButtonHints 拿掉了、底部只留 10px 邊距
    const int gridStartY = rect.y + topOffset;  // 跟封面下緣的閱讀時間拉開呼吸距離
    const int availableH = std::max(0, screenH - gridStartY - bottomReserve);
    const int safeH = std::min(std::max(0, rect.height - topOffset), availableH);
    if (safeH < 100) return;

    const int cellW = (rect.width - sidePad * 2 - gap) / gridCols;
    const int cellH = std::min(110, (safeH - gap) / gridRows);
    // stage15.23 修：cellH 守門從 50 降到 35（嚕寶說 menu 一定夠放）
    //   icon 縮成 24x24 + 文字緊貼 = 35 仍能畫
    if (cellW < 60 || cellH < 35) return;

    for (int i = 0; i < std::min(buttonCount, 4); ++i) {
      const int col = i % 2;
      const int row = i / 2;
      const int cellX = rect.x + sidePad + col * (cellW + gap);
      const int cellY = gridStartY + row * (cellH + gap);
      const bool selected = (i == selectedIndex);

      // stage15.20 (米蘭達選項 3: 底線無框):
      //   拿掉 fillRoundedRect Color::LightGray、拿掉 drawRoundedRect 描邊
      //   只用 icon + 文字、選中那格底下加 3px 粗黑線標記（如 navbar tab）
      //   e-paper partial refresh 最快、去 AI 框框感

      // icon + label 整組垂直置中
      const int labelH = renderer.getLineHeight(UI_10_FONT_ID);
      const int contentH = iconSize + iconLabelGap + labelH;
      const int contentTop = cellY + (cellH - contentH) / 2;

      if (rowIcon) {
        const UIIcon icon = rowIcon(i);
        const uint8_t* iconBitmap = lyraFlowMenuIcon(icon);
        if (iconBitmap) {
          const int iconX = cellX + (cellW - iconSize) / 2;
          renderer.drawIcon(iconBitmap, iconX, contentTop, iconSize, iconSize);
        }
      }
      const std::string labelStr = buttonLabel(i);
      // 選中時字體加粗、強化辨識
      const EpdFontFamily::Style labelStyle = selected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
      const int labelW = renderer.getTextWidth(UI_10_FONT_ID, labelStr.c_str(), labelStyle);
      const int labelX = cellX + (cellW - labelW) / 2;
      const int labelY = contentTop + iconSize + iconLabelGap;
      renderer.drawText(UI_10_FONT_ID, labelX, labelY, labelStr.c_str(), true, labelStyle);

      // 選中標記：文字下方 3px 粗黑短線（寬度=文字寬 + 4px 兩側 padding）
      if (selected) {
        const int markY = labelY + labelH + 2;
        renderer.drawLine(labelX - 2, markY, labelX + labelW + 2, markY, 3, true);
      }
    }
    return;
  }

  // buttonCount < 4 走原本 LyraFlow 分頁邏輯（保險 fallback）
  const auto& menuMetrics = UITheme::getInstance().getMetrics();
  const int rowStep = menuMetrics.menuRowHeight + menuMetrics.menuSpacing;
  // Reserve a thin strip at the bottom for page-indicator dots. Reserving
  // unconditionally keeps tile geometry stable whether dots are visible or not.
  constexpr int dotSize = 10;
  constexpr int dotSpacing = 8;
  constexpr int dotStripHeight = 18;
  const int usableHeight = std::max(0, rect.height - dotStripHeight);
  const int pageItems = std::max(1, usableHeight / rowStep);
  const int safeSelectedIndex = std::max(0, selectedIndex);

  // Two-anchor pagination: page 1 = items [0..pageItems), page 2 =
  // last `pageItems` items. The pages overlap; we resolve which one
  // to render via a sticky bit so the cursor "pulls" the visible
  // window with it asymmetrically — page 2 holds while the cursor
  // scrolls up through the overlap, and only flips back to page 1
  // when the cursor crosses page 2's top boundary.
  const bool needsPaging = buttonCount > pageItems;
  const int page2StartIndex = std::max(0, buttonCount - pageItems);
  if (!needsPaging) {
    stickyMenuPage2 = false;
  } else if (safeSelectedIndex >= pageItems) {
    // Cursor is in page 2's exclusive zone — we're definitely on page 2.
    stickyMenuPage2 = true;
  } else if (safeSelectedIndex < page2StartIndex) {
    // Cursor crossed page 2's top boundary going up — back to page 1.
    stickyMenuPage2 = false;
  }
  // Else: cursor in the overlap zone, keep whichever page we were on.
  const bool onPage2 = needsPaging && stickyMenuPage2;
  const int pageStartIndex = onPage2 ? page2StartIndex : 0;
  const int totalPages = needsPaging ? 2 : 1;
  const int currentPage = onPage2 ? 1 : 0;

  for (int i = pageStartIndex; i < buttonCount && i < pageStartIndex + pageItems; ++i) {
    const int displayIndex = i - pageStartIndex;
    const int tileWidth = rect.width - menuMetrics.contentSidePadding * 2;
    const Rect tileRect{rect.x + menuMetrics.contentSidePadding, rect.y + displayIndex * rowStep, tileWidth,
                        menuMetrics.menuRowHeight};

    const bool selected = (i == selectedIndex);
    if (selected) {
      renderer.fillRoundedRect(tileRect.x, tileRect.y, tileRect.width, tileRect.height, menuTileCornerRadius,
                               Color::LightGray);
    }

    const std::string labelStr = buttonLabel(i);
    const char* label = labelStr.c_str();
    int textX = tileRect.x + 16;
    const int lineHeight = renderer.getLineHeight(UI_12_FONT_ID);
    const int textY = tileRect.y + (menuMetrics.menuRowHeight - lineHeight) / 2;

    if (rowIcon != nullptr) {
      const UIIcon icon = rowIcon(i);
      if (icon == UIIcon::BookmarkIcon) {
        // Match the status-bar bookmark ribbon shape (LyraTheme parity).
        const int ribbonWidth = 16;
        const int ribbonHeight = 22;
        const int notchSize = 6;
        const int iconX = textX + (menuIconSize - ribbonWidth) / 2;
        const int iconY = textY + 4;
        const int centerX = iconX + ribbonWidth / 2;
        const int polyX[5] = {iconX, iconX + ribbonWidth, iconX + ribbonWidth, centerX, iconX};
        const int polyY[5] = {iconY, iconY, iconY + ribbonHeight, iconY + ribbonHeight - notchSize,
                              iconY + ribbonHeight};
        renderer.fillPolygon(polyX, polyY, 5, true);
        textX += menuIconSize + menuTilePadding + 2;
      } else {
        const uint8_t* iconBitmap = lyraFlowMenuIcon(icon);
        if (iconBitmap != nullptr) {
          renderer.drawIcon(iconBitmap, textX, textY + 3, menuIconSize, menuIconSize);
          textX += menuIconSize + menuTilePadding + 2;
        }
      }
    }

    renderer.drawText(UI_12_FONT_ID, textX, textY, label, true);
  }

  // Page-indicator dots — pattern lifted from RecentBooksGridActivity::render.
  // Anchor at the same vertical offset above the button hints as Recent Books
  // does (rect.y + rect.height == pageHeight - buttonHintsHeight for the home
  // menu rect, so this formula resolves to the same Y as Recent Books's
  // pageHeight - buttonHintsHeight - verticalSpacing - 4).
  if (totalPages > 1) {
    const int totalDotWidth = totalPages * dotSize + (totalPages - 1) * dotSpacing;
    const int dotsStartX = rect.x + (rect.width - totalDotWidth) / 2;
    const int dotY = rect.y + rect.height - menuMetrics.verticalSpacing - 4;
    constexpr int dotRadius = dotSize / 2;  // 5 → fully-circular bullet on 10x10
    for (int p = 0; p < totalPages; ++p) {
      const int dx = dotsStartX + p * (dotSize + dotSpacing);
      if (p == currentPage) {
        renderer.fillRoundedRect(dx, dotY, dotSize, dotSize, dotRadius, Color::Black);
      } else {
        renderer.drawRoundedRect(dx, dotY, dotSize, dotSize, 1, dotRadius, true);
      }
    }
  }
}

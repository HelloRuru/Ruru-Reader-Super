# NOTICE

This project, **Ruru-Reader-Super**, is a fork of
[SamLaio/crosspoint-chinesetype](https://github.com/SamLaio/crosspoint-chinesetype)
and is distributed under the **GNU Affero General Public License v3** (AGPL-3.0),
the same license as the upstream project. See [LICENSE](LICENSE) for full terms.

本專案 **Ruru-Reader-Super** 是
[SamLaio/crosspoint-chinesetype](https://github.com/SamLaio/crosspoint-chinesetype)
的 fork，採用與上游相同的 **GNU Affero General Public License v3**（AGPL-3.0）授權。
完整條款請見 [LICENSE](LICENSE)。

---

## Copyright holders / 著作權人

```
Copyright (c) 2025 Dave Allie       (crosspoint-reader original)
Copyright (c) 2024 uxjulia          (CrossInk fork)
Copyright (c) 2024 chintanvajariya  (CrossInk-Carousel fork)
Copyright (c) 2024 icannotttt       (crosspoint-chinesetype upstream)
Copyright (c) 2024-2026 SamLaio     (crosspoint-chinesetype TW fork — base for this project)
Copyright (c) 2026 HelloRuru / Kaoru Tsai  (Ruru-Reader-Super UI overhaul)
```

---

## HelloRuru modifications / HelloRuru 修改範圍

The following code contributions are authored by HelloRuru / Kaoru Tsai,
licensed under AGPL-3.0 (same as upstream):

由 HelloRuru / 蔡依庭 所貢獻、同樣採用 AGPL-3.0 授權的修改包含：

- **Lyra theme overhaul** — `src/components/themes/lyra/LyraTheme.cpp/.h`
- **LyraFlow Carousel theme** — `src/components/themes/lyra/LyraFlowTheme.cpp/.h`
- **3x3 grid home screen** — `src/activities/home/RecentBooksActivity.cpp/.h`
- **Lucide icon system** — `src/components/icons/` (book / chart / cover / folder /
  hotspot / library / recent / settings2 / transfer / wifi etc.)
- **HelloRuru rabbit logo** — `src/images/RabbitLarge.h`
- **Per-book progress tracking** — `progressPercent` field in `RecentBook` struct
- **GfxRenderer enhancements** — perspective bitmap (`drawPerspectiveBitmap`),
  rendering helpers used by LyraFlow

---

## Trademark notice / 商標聲明

The following are **trademarks of HelloRuru / Kaoru Tsai** and are
**NOT covered by the AGPL-3.0 license**:

下列為 **HelloRuru / 蔡依庭的商標**，**不在 AGPL-3.0 授權範圍內**：

- The name **HelloRuru** and the related branding.
- The name **Ruru-Reader** and **Ruru-Reader-Super**.
- The **HelloRuru rabbit** logo and visual marks
  (e.g. `RabbitLarge.h`, `Logo120.png`, branded boot/sleep screens).
- The **pink (#D4A5A5) / brown (#6D5954) brand palette** when applied as
  a HelloRuru-style brand identity.

Forks may freely modify and redistribute the AGPL-3.0 code, including
the rabbit-logo asset file, but must **not present a fork as an official
HelloRuru product** or use HelloRuru branding to imply endorsement.

Forks 可以自由修改與再散布所有 AGPL-3.0 程式碼（包括兔子 Logo 的 .h 資產
檔案），但**不得將 fork 呈現為 HelloRuru 的官方產品**，也不得使用
HelloRuru 品牌暗示官方背書。

If you build a derivative, please rename / re-brand the project.

如果你要做衍生作品，請更名／改換品牌。

---

## Acknowledgments / 致謝

This project stands on the shoulders of, in chronological order:

| Upstream | Author | Contribution |
| :--- | :--- | :--- |
| [crosspoint-reader](https://github.com/daveallie/crosspoint-reader) | Dave Allie | Original MIT-licensed reader (re-licensed downstream) |
| CrossInk | uxjulia | Chinese-friendly intermediate fork |
| CrossInk-Carousel | chintanvajariya | Lyra / Flow / 3Covers UI theme system |
| [crosspoint-chinesetype](https://github.com/icannotttt/crosspoint-chinesetype) | icannotttt | Chinese localisation, OPDS, JianGuo cloud, KOReader sync, BLE skeleton |
| **[crosspoint-chinesetype (TW)](https://github.com/SamLaio/crosspoint-chinesetype)** | **SamLaio** | **Direct base for this project — Traditional Chinese vertical layout, second-generation font glyphs, AGPL-licensed full distribution** |

Special thanks to [@SamLaio](https://github.com/SamLaio) — without
your AGPL release of the Traditional Chinese fork, this project would
not exist. The vertical-writing layout, secondary font glyphs, and
overall TW-first architecture are entirely your work, kept intact in
this fork.

Fonts:

- **noto-sans-cjk** (Adobe / Google) — SIL Open Font License 1.1
- Reference: [jf-openhuninn](https://justfont.com/huninn/) by justfont (CC BY 4.0)
  used in the sibling project `ruru-reader-tw`.

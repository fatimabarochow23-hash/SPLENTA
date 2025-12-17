# SPLENTA UI Batch 09: Rebranding to "SubForge"

用户已确认性能问题的根源在于高频重绘下的复杂字体渲染。
目前的策略：
1. **Header 清理**：删除旧的 "SPLENTA" 标题。
2. **Footer 重塑**：在右下角实现 "SubForge" 品牌标识，使用 **静态绘制** 策略确保零性能损耗。

---

## 🗑️ 任务 1：Header 清理 (Remove Legacy Logo)

**目标**：移除顶部的 "SPLENTA" 文字，腾出空间给预设管理。

**执行**：
- 打开 `PluginEditor.h` / `.cpp`。
- 删除 `logoLabel` 及其相关的所有初始化和布局代码。
- 调整 `presetNameLabel` 或 `presetComboBox` 的位置，使其靠左对齐（占据原本 Logo 的位置），让 Header 布局更平衡。

---

## 🎨 任务 2：右下角品牌重塑 (Footer Rebranding)

**目标**：在界面右下角绘制新的 "SubForge" 标识（参考 Web 版截图）。

**设计规范**：
- **位置**：Footer 区域的最右侧（原本 "AUDIO TOOLS" 的位置）。
- **图标**：一个六边形 (Hexagon)。
- **主标题**："SUBFORGE" (JetBrains Mono, Bold, ~20px)。
- **副标题**："GENERATIVE LF ENHANCER" (JetBrains Mono, ~10px, 位于主标题下方)。

**配色逻辑 (关键)**：
- **图标**：描边颜色 = `currentPalette.accent`。
- **SUB**：颜色 = `Colours::white` (或极浅的灰)。
- **FORGE**：颜色 = `currentPalette.accent` (随主题变色)。
- **副标题**：颜色 = `Colours::white.withAlpha(0.3f)`。

**✅ 性能安全实现指南 (必读)**：
为了避免重蹈覆辙（卡顿），**严禁**为这个 Logo 创建单独的 `Component` 或 `Label` 并尝试实时更新它。
**请直接在 `PluginEditor::paint()` 方法的末尾绘制它。**

**原因**：
`PluginEditor::paint()` 只在窗口大小改变或主题切换时调用一次。它**不会**每秒调用 60 次（只要你不手动调 `repaint`）。因此，在这里进行复杂的 `drawText` 或 `setColour` 是绝对安全的，CPU 负载为 0。

**代码逻辑示例**：
```cpp
// 在 PluginEditor::paint() 底部
// 1. 定义区域
auto footerArea = getLocalBounds().removeFromBottom(40).reduced(10);
auto logoArea = footerArea.removeFromRight(200);

// 2. 绘制六边形图标 (左侧)
Path hexagon;
// ... 构建六边形路径 ...
g.setColour(currentPalette.accent);
g.strokePath(hexagon, PathStrokeType(2.0f));

// 3. 绘制文字 (分段绘制以实现变色)
g.setFont(getJetBrainsMono().withHeight(20.0f).withStyle(Font::bold));

// 绘制 "SUB"
g.setColour(Colours::white);
g.drawText("SUB", textRect, Justification::left, false);

// 计算 "SUB" 的宽度并偏移，绘制 "FORGE"
int subWidth = g.getCurrentFont().getStringWidth("SUB");
auto forgeRect = textRect.withTrimmedLeft(subWidth);
g.setColour(currentPalette.accent); // ✅ 这里变色是安全的，因为只画一次
g.drawText("FORGE", forgeRect, Justification::left, false);

// 4. 绘制副标题
g.setFont(getJetBrainsMono().withHeight(10.0f));
g.setColour(Colours::white.withAlpha(0.3f));
g.drawText("GENERATIVE LF ENHANCER", subTitleRect, Justification::right, false);
```

---

## 检查清单
- [ ] Header 左上角的 "SPLENTA" 彻底删除了吗？
- [ ] 右下角现在显示 "SUBFORGE" 了吗？
- [ ] "FORGE" 的颜色会随主题改变（变蓝、变绿）吗？
- [ ] **关键验证**：界面是否依然保持 60FPS 流畅？（确认没有导致循环重绘）

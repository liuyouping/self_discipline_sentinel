# web_file — 赛博自律哨兵 Web 前端

## 目录内容

| 文件 | 说明 |
| --- | --- |
| [sleep.html] | 移动端单页面 Web 界面，纯 HTML/CSS/JS，无构建依赖 |

## 与固件的关系

> **原说明保留**：在智能终端的屏幕上产生二维码，二维码内嵌这个网址：
> ```
> https://iot.wit-iot.com/web/sleep.html
> ```
> 用户通过手机跳转此网址，可以查看更加详细的设备界面。

固件侧 [app.c → doBasicDemo()] 使用 `qrcodegen` 库将上述 URL 实时编码为二维码，再通过 `qrcode_to_rgb565()` 居中缩放至 ST7789 LCD（240×320）显示。

## 页面概览

- **标题**：赛博自律哨兵
- **主题**：赛博朋克暗色系（`#0b0e1a` 深色背景 + 径向渐变光晕 + 毛玻璃卡片）
- **适配**：移动端优先（`max-width: 420px` + `viewport-fit=cover` 支持全面屏）
- **单页多视图**：通过 `.page.active` 控制页面切换（Header、卡片列表、连接控制栏）
- **视觉特性**：
  - `.control-bar` — 连接控制区（设备选择 + 蓝牙/WiFi 按钮）
  - `.header` — 渐变标题文字 + 时间胶囊徽章
  - `.card` — 毛玻璃信息卡片（`backdrop-filter: blur(12px)`）

## 技术栈

纯原生前端：HTML5 + CSS3 + Vanilla JavaScript，**零构建**，任何现代浏览器可直接打开。

```bash
# 本地预览（任选其一）
python3 -m http.server 8080
# 或
npx serve .
# 然后访问 http://localhost:8080/sleep.html
```

## 部署

生产环境托管于 `https://iot.wit-iot.com/web/sleep.html`，固件中的二维码文本与该地址强耦合。

更新页面后请：
1. 将新 `sleep.html` 上传至服务器对应路径
2. 固件侧二维码地址如未改变，**无需重新烧录**；若 URL 变更则需修改 [app.c] 的 `text` 常量后重编译

## 后续可扩展

- [ ] 与 WS63 固件通过 WebBLE 或 MQTT-WS 建立数据通道，把姿态角、睡眠数据推到页面
- [ ] 增加历史曲线（Chart.js / ECharts）展示 `Pitch / Roll / Yaw` 时序
- [ ] 增加赛博朋克风格的动画过渡与粒子特效

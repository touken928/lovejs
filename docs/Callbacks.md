# 回调驱动

LoveJS采用回调驱动的架构，类似于Love2D。你只需要在`main.js`中导出回调函数，引擎会在适当的时机自动调用它们。

## 基本结构

```javascript
// main.js
import { setWindow, clear, present } from 'graphics';

// 初始化
export function load() {
    setWindow("我的游戏", 800, 600);
}

// 更新
export function update(dt) {
    // dt是距离上一帧的时间（秒）
}

// 绘制
export function draw() {
    clear(0, 0, 0, 1);
    present();
}
```

## 回调函数列表

### load()

游戏启动时调用一次，用于初始化资源。

```javascript
export function load() {
    setWindow("游戏标题", 800, 600);
    // 加载纹理、初始化游戏状态等
}
```

### update(dt)

每帧调用，用于更新游戏逻辑。

| 参数 | 类型 | 说明 |
|-----|------|------|
| dt | number | 距离上一帧的时间（秒） |

```javascript
let x = 0;

export function update(dt) {
    // 每秒移动100像素
    x += 100 * dt;
}
```

### draw()

每帧调用，用于绘制画面。在`update`之后调用。

```javascript
export function draw() {
    clear(0.1, 0.1, 0.2, 1);
    
    setColor(1, 1, 1, 1);
    circle(x, 300, 20, true);
    
    present();
}
```

### keypressed(key)

键盘按下时调用。

| 参数 | 类型 | 说明 |
|-----|------|------|
| key | string | 按键名称（小写） |

```javascript
let keys = {};

export function keypressed(key) {
    keys[key] = true;
    
    if (key === 'escape') {
        // 退出逻辑
    }
}
```

### keyreleased(key)

键盘释放时调用。

| 参数 | 类型 | 说明 |
|-----|------|------|
| key | string | 按键名称（小写） |

```javascript
export function keyreleased(key) {
    keys[key] = false;
}
```

### mousepressed(x, y, button)

鼠标按下时调用。

| 参数 | 类型 | 说明 |
|-----|------|------|
| x | number | 鼠标X坐标 |
| y | number | 鼠标Y坐标 |
| button | number | 按钮编号（1=左键, 2=中键, 3=右键） |

```javascript
export function mousepressed(x, y, button) {
    if (button === 1) {
        // 左键点击
        player.x = x;
        player.y = y;
    }
}
```

### mousereleased(x, y, button)

鼠标释放时调用。

| 参数 | 类型 | 说明 |
|-----|------|------|
| x | number | 鼠标X坐标 |
| y | number | 鼠标Y坐标 |
| button | number | 按钮编号 |

```javascript
export function mousereleased(x, y, button) {
    // 鼠标释放处理
}
```

### wheelmoved(x, y)

鼠标滚轮滚动时调用。

| 参数 | 类型 | 说明 |
|-----|------|------|
| x | number | 水平滚动量 |
| y | number | 垂直滚动量（正=向上，负=向下） |

```javascript
let zoom = 1;

export function wheelmoved(x, y) {
    zoom += y * 0.1;
    zoom = Math.max(0.1, Math.min(3, zoom));
}
```

## 完整示例

```javascript
import { setWindow, clear, present, setColor, circle, rectangle } from 'graphics';

// 游戏状态
let player = { x: 400, y: 300, speed: 200 };
let keys = {};

export function load() {
    setWindow("简单游戏", 800, 600);
}

export function update(dt) {
    // 键盘移动
    if (keys['w']) player.y -= player.speed * dt;
    if (keys['s']) player.y += player.speed * dt;
    if (keys['a']) player.x -= player.speed * dt;
    if (keys['d']) player.x += player.speed * dt;
    
    // 边界检测
    player.x = Math.max(20, Math.min(780, player.x));
    player.y = Math.max(20, Math.min(580, player.y));
}

export function draw() {
    // 清屏
    clear(0.1, 0.1, 0.2, 1);
    
    // 绘制玩家
    setColor(0, 1, 0, 1);
    circle(player.x, player.y, 20, true);
    
    // 呈现
    present();
}

export function keypressed(key) {
    keys[key] = true;
}

export function keyreleased(key) {
    keys[key] = false;
}

export function mousepressed(x, y, button) {
    if (button === 1) {
        player.x = x;
        player.y = y;
    }
}

export function mousereleased(x, y, button) {}

export function wheelmoved(x, y) {}
```

## 游戏循环

引擎内部的游戏循环大致如下：

```
1. 调用 load()
2. 循环:
   a. 处理事件 -> 调用 keypressed/keyreleased/mousepressed/mousereleased/wheelmoved
   b. 调用 update(dt)
   c. 调用 draw()
   d. 等待下一帧 (~60 FPS)
3. 退出
```

## 注意事项

1. 所有回调函数都是可选的，未导出的回调不会被调用
2. `draw()`中必须调用`present()`才能显示画面
3. `dt`的单位是秒，使用它来实现帧率无关的移动
4. 按键名称是小写的（如`'w'`、`'space'`、`'escape'`）

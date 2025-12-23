# Graphics 模块

Graphics模块提供2D图形渲染功能，包括窗口管理、基本图形绘制、颜色设置、变换和纹理支持。

## 导入

```javascript
// 导入所需函数
import { setWindow, clear, present, setColor, line, rectangle, circle, print } from 'graphics';

// 或导入全部
import * as graphics from 'graphics';
```

## 窗口管理

### setWindow(title, width, height)

创建游戏窗口。

| 参数 | 类型 | 说明 |
|-----|------|------|
| title | string | 窗口标题 |
| width | number | 窗口宽度（像素） |
| height | number | 窗口高度（像素） |

```javascript
setWindow("我的游戏", 800, 600);
```

### getWindowSize()

获取窗口尺寸。

返回: `[width, height]`

```javascript
let [width, height] = getWindowSize();
```

## 渲染控制

### clear(r, g, b, a)

清除屏幕并填充指定颜色。

| 参数 | 类型 | 说明 |
|-----|------|------|
| r | number | 红色分量 (0-1) |
| g | number | 绿色分量 (0-1) |
| b | number | 蓝色分量 (0-1) |
| a | number | 透明度 (0-1) |

```javascript
clear(0.1, 0.1, 0.2, 1.0); // 深蓝色背景
```

### present()

将渲染内容呈现到屏幕。每帧绘制完成后必须调用。

```javascript
export function draw() {
    clear(0, 0, 0, 1);
    // 绘制内容...
    present(); // 必须调用
}
```

### setColor(r, g, b, a)

设置后续绘制操作的颜色。

| 参数 | 类型 | 说明 |
|-----|------|------|
| r | number | 红色分量 (0-1) |
| g | number | 绿色分量 (0-1) |
| b | number | 蓝色分量 (0-1) |
| a | number | 透明度 (0-1) |

```javascript
setColor(1, 0, 0, 1);    // 红色
setColor(0, 1, 0, 0.5);  // 半透明绿色
```

## 基本图形

### point(x, y)

绘制一个点。

```javascript
setColor(1, 1, 1, 1);
point(100, 100);
```

### line(x1, y1, x2, y2)

绘制一条线段。

| 参数 | 类型 | 说明 |
|-----|------|------|
| x1, y1 | number | 起点坐标 |
| x2, y2 | number | 终点坐标 |

```javascript
setColor(1, 1, 1, 1);
line(0, 0, 800, 600); // 对角线
```

### rectangle(x, y, width, height, filled)

绘制矩形。

| 参数 | 类型 | 说明 |
|-----|------|------|
| x, y | number | 左上角坐标 |
| width | number | 宽度 |
| height | number | 高度 |
| filled | boolean | true=填充, false=边框 |

```javascript
setColor(1, 0, 0, 1);
rectangle(100, 100, 200, 150, true);  // 填充矩形

setColor(0, 1, 0, 1);
rectangle(100, 100, 200, 150, false); // 边框矩形
```

### circle(x, y, radius, filled)

绘制圆形。

| 参数 | 类型 | 说明 |
|-----|------|------|
| x, y | number | 圆心坐标 |
| radius | number | 半径 |
| filled | boolean | true=填充, false=边框 |

```javascript
setColor(0, 0, 1, 1);
circle(400, 300, 50, true);  // 填充圆

setColor(1, 1, 0, 1);
circle(400, 300, 50, false); // 边框圆
```

## 文本渲染

### print(text, x, y)

绘制文本。

| 参数 | 类型 | 说明 |
|-----|------|------|
| text | string | 要绘制的文本 |
| x, y | number | 文本位置 |

```javascript
setColor(1, 1, 1, 1);
print("Hello World!", 100, 100);
print("Score: 1000", 100, 130);
```

## 变换

变换函数用于实现旋转、缩放、平移等效果。使用`push()`和`pop()`来保存和恢复变换状态。

### push()

保存当前变换状态到栈中。

### pop()

从栈中恢复之前保存的变换状态。

### translate(x, y)

平移坐标系。

| 参数 | 类型 | 说明 |
|-----|------|------|
| x | number | X方向平移量 |
| y | number | Y方向平移量 |

### rotate(angle)

旋转坐标系。

| 参数 | 类型 | 说明 |
|-----|------|------|
| angle | number | 旋转角度（弧度） |

### scale(x, y)

缩放坐标系。

| 参数 | 类型 | 说明 |
|-----|------|------|
| x | number | X方向缩放比例 |
| y | number | Y方向缩放比例 |

### 变换示例

```javascript
// 绘制旋转的矩形
push();                      // 保存状态
translate(400, 300);         // 移动到中心
rotate(Math.PI / 4);         // 旋转45度
setColor(1, 0, 0, 1);
rectangle(-50, -50, 100, 100, true); // 以中心绘制
pop();                       // 恢复状态
```

## 纹理

### loadTexture(path)

加载纹理图片。

| 参数 | 类型 | 说明 |
|-----|------|------|
| path | string | 图片文件路径 |

返回: 纹理ID（字符串），加载失败返回空字符串

```javascript
let texture = loadTexture("player.png");
```

### drawTexture(textureId, x, y, rotation, scaleX, scaleY)

绘制纹理。

| 参数 | 类型 | 说明 |
|-----|------|------|
| textureId | string | 纹理ID |
| x, y | number | 绘制位置 |
| rotation | number | 旋转角度（弧度） |
| scaleX | number | X缩放比例 |
| scaleY | number | Y缩放比例 |

```javascript
let texture = loadTexture("player.png");

export function draw() {
    clear(0, 0, 0, 1);
    drawTexture(texture, 100, 100, 0, 1, 1);
    present();
}
```

## 预定义颜色

模块提供了一些预定义的颜色常量：

| 常量 | 值 |
|-----|-----|
| WHITE | [1, 1, 1, 1] |
| BLACK | [0, 0, 0, 1] |
| RED | [1, 0, 0, 1] |
| GREEN | [0, 1, 0, 1] |
| BLUE | [0, 0, 1, 1] |
| YELLOW | [1, 1, 0, 1] |
| CYAN | [0, 1, 1, 1] |
| MAGENTA | [1, 0, 1, 1] |

```javascript
import { setColor, WHITE, RED } from 'graphics';

setColor(...WHITE);  // 使用展开运算符
setColor(...RED);
```

## 完整示例

```javascript
import { 
    setWindow, clear, present, 
    setColor, line, rectangle, circle,
    push, pop, translate, rotate
} from 'graphics';

let time = 0;

export function load() {
    setWindow("Graphics Demo", 800, 600);
}

export function update(dt) {
    time += dt;
}

export function draw() {
    // 清屏
    clear(0.1, 0.1, 0.2, 1);
    
    // 绘制网格
    setColor(0.3, 0.3, 0.3, 1);
    for (let x = 0; x <= 800; x += 50) {
        line(x, 0, x, 600);
    }
    for (let y = 0; y <= 600; y += 50) {
        line(0, y, 800, y);
    }
    
    // 绘制旋转的矩形
    push();
    translate(200, 200);
    rotate(time);
    setColor(1, 0, 0, 1);
    rectangle(-40, -40, 80, 80, true);
    pop();
    
    // 绘制脉动的圆
    let radius = 30 + Math.sin(time * 3) * 10;
    setColor(0, 1, 0, 1);
    circle(400, 300, radius, true);
    
    // 绘制边框圆
    setColor(0, 0, 1, 1);
    circle(600, 200, 50, false);
    
    present();
}

export function keypressed(key) {}
export function keyreleased(key) {}
export function mousepressed(x, y, button) {}
export function mousereleased(x, y, button) {}
export function wheelmoved(x, y) {}
```

## 坐标系

- 原点(0, 0)在窗口左上角
- X轴向右为正
- Y轴向下为正
- 角度使用弧度制（`Math.PI`为180度）

```
(0,0) ────────────► X
  │
  │
  │
  ▼
  Y
```
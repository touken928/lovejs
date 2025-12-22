// Love2D风格的游戏 - 导出回调函数
import { setWindow, clear, present, setColor, line, rectangle, circle, push, pop, translate, rotate } from 'graphics';

// 游戏状态
let player = { x: 400, y: 300, size: 20 };
let rotation = 0;
let time = 0;
let keys = {};

// 初始化回调
export function load() {
    setWindow("LoveJS Demo", 800, 600);
}

// 更新回调
export function update(dt) {
    time += dt;
    rotation += dt * 2;
    
    // 键盘控制玩家移动
    if (keys['w']) player.y -= 200 * dt;
    if (keys['s']) player.y += 200 * dt;
    if (keys['a']) player.x -= 200 * dt;
    if (keys['d']) player.x += 200 * dt;
    
    // 限制玩家在屏幕内
    player.x = Math.max(player.size, Math.min(800 - player.size, player.x));
    player.y = Math.max(player.size, Math.min(600 - player.size, player.y));
}

// 绘制回调
export function draw() {
    clear(0.1, 0.1, 0.2, 1.0);
    
    // 绘制网格
    setColor(0.3, 0.3, 0.3, 1.0);
    for (let x = 0; x <= 800; x += 50) line(x, 0, x, 600);
    for (let y = 0; y <= 600; y += 50) line(0, y, 800, y);
    
    // 绘制中心十字线
    setColor(1.0, 1.0, 1.0, 1.0);
    line(0, 300, 800, 300);
    line(400, 0, 400, 600);
    
    // 绘制旋转的矩形
    push();
    translate(200, 150);
    rotate(rotation);
    setColor(1.0, 0.0, 0.0, 1.0);
    rectangle(-25, -25, 50, 50, true);
    pop();
    
    // 绘制玩家
    setColor(0.0, 1.0, 0.0, 1.0);
    circle(player.x, player.y, player.size, true);
    
    // 绘制装饰
    setColor(1.0, 1.0, 0.0, 0.8);
    for (let i = 0; i < 5; i++) {
        let x = 100 + i * 120;
        let y = 500 + Math.sin(time * 3 + i) * 20;
        circle(x, y, 15, false);
    }
    
    present();
}

// 键盘按下回调
export function keypressed(key) {
    keys[key] = true;
}

// 键盘释放回调
export function keyreleased(key) {
    keys[key] = false;
}

// 鼠标按下回调
export function mousepressed(x, y, button) {
    player.x = x;
    player.y = y;
}

// 鼠标释放回调
export function mousereleased(x, y, button) {
}

// 鼠标滚轮回调
export function wheelmoved(x, y) {
    player.size += y * 2;
    player.size = Math.max(5, Math.min(50, player.size));
}
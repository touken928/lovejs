// 基本图形绘制示例 - 导出回调函数
import { setWindow, clear, present, setColor, point, line, rectangle, circle, push, pop, translate, rotate } from 'graphics';

let time = 0;

export function load() {
    setWindow("基本图形示例", 800, 600);
}

export function update(dt) {
    time += dt;
}

export function draw() {
    clear(0.05, 0.05, 0.1, 1.0);
    
    // 绘制标题区域
    setColor(1, 1, 1, 1);
    rectangle(10, 10, 780, 50, false);
    
    // 绘制各种图形
    let startX = 100;
    let startY = 100;
    let spacing = 150;
    
    // 1. 点
    setColor(1, 0, 0, 1);
    for (let i = 0; i < 20; i++) {
        for (let j = 0; j < 20; j++) {
            point(startX + i * 2, startY + j * 2);
        }
    }
    
    // 2. 线条
    setColor(0, 1, 0, 1);
    for (let i = 0; i < 10; i++) {
        let angle = (time + i * 0.5) * 2;
        let x1 = startX + spacing + Math.cos(angle) * 30;
        let y1 = startY + 20 + Math.sin(angle) * 30;
        let x2 = startX + spacing + Math.cos(angle + Math.PI) * 30;
        let y2 = startY + 20 + Math.sin(angle + Math.PI) * 30;
        line(x1, y1, x2, y2);
    }
    
    // 3. 矩形
    setColor(0, 0, 1, 1);
    rectangle(startX + spacing * 2, startY, 40, 40, false);
    setColor(0, 0, 1, 0.5);
    rectangle(startX + spacing * 2 + 10, startY + 10, 40, 40, true);
    
    // 4. 圆形
    setColor(1, 1, 0, 1);
    circle(startX + spacing * 3 + 20, startY + 20, 20, false);
    setColor(1, 1, 0, 0.5);
    circle(startX + spacing * 3 + 20, startY + 20, 15, true);
    
    // 第二行 - 动画效果
    startY += 100;
    
    // 旋转矩形
    push();
    translate(startX + 20, startY + 20);
    rotate(time * 2);
    setColor(1, 0.5, 0, 1);
    rectangle(-15, -15, 30, 30, true);
    pop();
    
    // 脉动圆形
    let radius = 20 + Math.sin(time * 4) * 10;
    setColor(1, 0, 1, 0.8);
    circle(startX + spacing + 20, startY + 20, radius, true);
    
    // 彩虹圆环
    for (let i = 0; i < 8; i++) {
        let hue = (time + i * 0.3) % (Math.PI * 2);
        let r = Math.sin(hue) * 0.5 + 0.5;
        let g = Math.sin(hue + Math.PI * 2 / 3) * 0.5 + 0.5;
        let b = Math.sin(hue + Math.PI * 4 / 3) * 0.5 + 0.5;
        setColor(r, g, b, 1);
        circle(startX + spacing * 2 + 20, startY + 20, 25 - i * 3, false);
    }
    
    // 波浪线
    setColor(0, 1, 1, 1);
    let prevX = startX + spacing * 3;
    let prevY = startY + 20 + Math.sin(time * 3) * 10;
    
    for (let i = 1; i <= 40; i++) {
        let x = startX + spacing * 3 + i;
        let y = startY + 20 + Math.sin(time * 3 + i * 0.2) * 10;
        line(prevX, prevY, x, y);
        prevX = x;
        prevY = y;
    }
    
    present();
}

export function keypressed(key) {}
export function keyreleased(key) {}
export function mousepressed(x, y, button) {}
export function mousereleased(x, y, button) {}
export function wheelmoved(x, y) {}
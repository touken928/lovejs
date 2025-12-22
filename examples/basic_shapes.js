// 基本图形绘制示例 - 导出回调函数
import * as graphics from 'graphics';

let time = 0;

export function load() {
    graphics.setWindow("基本图形示例", 800, 600);
}

export function update(dt) {
    time += dt;
}

export function draw() {
    graphics.clear(0.05, 0.05, 0.1, 1.0);
    
    // 绘制标题区域
    graphics.setColor(1, 1, 1, 1);
    graphics.rectangle(10, 10, 780, 50, false);
    
    // 绘制各种图形
    let startX = 100;
    let startY = 100;
    let spacing = 150;
    
    // 1. 点
    graphics.setColor(1, 0, 0, 1);
    for (let i = 0; i < 20; i++) {
        for (let j = 0; j < 20; j++) {
            graphics.point(startX + i * 2, startY + j * 2);
        }
    }
    
    // 2. 线条
    graphics.setColor(0, 1, 0, 1);
    for (let i = 0; i < 10; i++) {
        let angle = (time + i * 0.5) * 2;
        let x1 = startX + spacing + Math.cos(angle) * 30;
        let y1 = startY + 20 + Math.sin(angle) * 30;
        let x2 = startX + spacing + Math.cos(angle + Math.PI) * 30;
        let y2 = startY + 20 + Math.sin(angle + Math.PI) * 30;
        graphics.line(x1, y1, x2, y2);
    }
    
    // 3. 矩形
    graphics.setColor(0, 0, 1, 1);
    graphics.rectangle(startX + spacing * 2, startY, 40, 40, false);
    graphics.setColor(0, 0, 1, 0.5);
    graphics.rectangle(startX + spacing * 2 + 10, startY + 10, 40, 40, true);
    
    // 4. 圆形
    graphics.setColor(1, 1, 0, 1);
    graphics.circle(startX + spacing * 3 + 20, startY + 20, 20, false);
    graphics.setColor(1, 1, 0, 0.5);
    graphics.circle(startX + spacing * 3 + 20, startY + 20, 15, true);
    
    // 第二行 - 动画效果
    startY += 100;
    
    // 旋转矩形
    graphics.push();
    graphics.translate(startX + 20, startY + 20);
    graphics.rotate(time * 2);
    graphics.setColor(1, 0.5, 0, 1);
    graphics.rectangle(-15, -15, 30, 30, true);
    graphics.pop();
    
    // 脉动圆形
    let radius = 20 + Math.sin(time * 4) * 10;
    graphics.setColor(1, 0, 1, 0.8);
    graphics.circle(startX + spacing + 20, startY + 20, radius, true);
    
    // 彩虹圆环
    for (let i = 0; i < 8; i++) {
        let hue = (time + i * 0.3) % (Math.PI * 2);
        let r = Math.sin(hue) * 0.5 + 0.5;
        let g = Math.sin(hue + Math.PI * 2 / 3) * 0.5 + 0.5;
        let b = Math.sin(hue + Math.PI * 4 / 3) * 0.5 + 0.5;
        graphics.setColor(r, g, b, 1);
        graphics.circle(startX + spacing * 2 + 20, startY + 20, 25 - i * 3, false);
    }
    
    // 波浪线
    graphics.setColor(0, 1, 1, 1);
    let prevX = startX + spacing * 3;
    let prevY = startY + 20 + Math.sin(time * 3) * 10;
    
    for (let i = 1; i <= 40; i++) {
        let x = startX + spacing * 3 + i;
        let y = startY + 20 + Math.sin(time * 3 + i * 0.2) * 10;
        graphics.line(prevX, prevY, x, y);
        prevX = x;
        prevY = y;
    }
    
    graphics.present();
}

export function keypressed(key) {}
export function keyreleased(key) {}
export function mousepressed(x, y, button) {}
export function mousereleased(x, y, button) {}
export function wheelmoved(x, y) {}
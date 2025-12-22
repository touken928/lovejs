// 测试文本渲染
import * as graphics from 'graphics';

export function load() {
    graphics.setWindow("Text Test", 800, 600);
}

export function update(dt) {
}

export function draw() {
    graphics.clear(0.1, 0.1, 0.2, 1.0);
    
    // 绘制文本
    graphics.setColor(1.0, 1.0, 1.0, 1.0);
    graphics.print("HELLO WORLD!", 100, 100);
    graphics.print("LoveJS Engine", 100, 120);
    graphics.print("0123456789", 100, 140);
    graphics.print("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 100, 160);
    
    // 绘制一些图形作为对比
    graphics.setColor(0.0, 1.0, 0.0, 1.0);
    graphics.circle(400, 300, 50, true);
    
    graphics.setColor(1.0, 1.0, 0.0, 1.0);
    graphics.print("Score: 1000", 100, 200);
    
    graphics.present();
}

export function keypressed(key) {}
export function keyreleased(key) {}
export function mousepressed(x, y, button) {}
export function mousereleased(x, y, button) {}
export function wheelmoved(x, y) {}

// 最简单的 LoveJS 示例
import { setWindow, clear, present, setColor, circle, print } from 'graphics';

export function load() {
    setWindow("Hello LoveJS", 800, 600);
}

export function update(dt) {}

export function draw() {
    clear(0.2, 0.2, 0.3, 1);
    
    // 文字居中显示 (14个字符 * 7像素 + 13个间距 = 111像素宽)
    setColor(1, 1, 1, 1);
    print("Hello, LoveJS!", 345, 280);
    
    setColor(0, 0.8, 0.4, 1);
    circle(400, 350, 30, true);
    
    present();
}

export function keypressed(key) {}
export function keyreleased(key) {}
export function mousepressed(x, y, button) {}
export function mousereleased(x, y, button) {}
export function wheelmoved(x, y) {}

// 测试 import 其他 JS 模块
import { setWindow, clear, present, setColor, print } from 'graphics';
import { add, multiply, PI } from './utils.js';

let result1 = 0;
let result2 = 0;

export function load() {
    setWindow("Import Test", 800, 600);
    result1 = add(10, 20);
    result2 = multiply(5, 6);
}

export function update(dt) {}

export function draw() {
    clear(0.1, 0.1, 0.2, 1);
    
    setColor(1, 1, 1, 1);
    print("Testing JS module import", 280, 200);
    
    setColor(0, 1, 0, 1);
    print("add(10, 20) = " + result1, 300, 260);
    print("multiply(5, 6) = " + result2, 300, 290);
    print("PI = " + PI, 300, 320);
    
    present();
}

export function keypressed(key) {}
export function keyreleased(key) {}
export function mousepressed(x, y, button) {}
export function mousereleased(x, y, button) {}
export function wheelmoved(x, y) {}

// 粒子效果示例 - 展示动态图形和数组操作
import { setWindow, clear, present, setColor, circle, print } from 'graphics';

const MAX_PARTICLES = 200;
let particles = [];
let mouseX = 400, mouseY = 300;

function spawnParticle(x, y) {
    if (particles.length >= MAX_PARTICLES) return;
    
    const angle = Math.random() * Math.PI * 2;
    const speed = 50 + Math.random() * 100;
    
    particles.push({
        x, y,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed,
        life: 1.0,
        size: 3 + Math.random() * 5,
        hue: Math.random()
    });
}

export function load() {
    setWindow("Particles", 800, 600);
}

export function update(dt) {
    // 持续在鼠标位置生成粒子
    for (let i = 0; i < 3; i++) {
        spawnParticle(mouseX, mouseY);
    }
    
    // 更新粒子
    for (let p of particles) {
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += 100 * dt; // 重力
        p.life -= dt * 0.8;
    }
    
    // 移除死亡粒子
    particles = particles.filter(p => p.life > 0);
}

export function draw() {
    clear(0.05, 0.05, 0.1, 1);
    
    for (let p of particles) {
        const r = Math.sin(p.hue * Math.PI * 2) * 0.5 + 0.5;
        const g = Math.sin(p.hue * Math.PI * 2 + 2) * 0.5 + 0.5;
        const b = Math.sin(p.hue * Math.PI * 2 + 4) * 0.5 + 0.5;
        setColor(r, g, b, p.life);
        circle(p.x, p.y, p.size * p.life, true);
    }
    
    setColor(1, 1, 1, 0.8);
    print(`Particles: ${particles.length}`, 10, 20);
    
    present();
}

export function keypressed(key) {}
export function keyreleased(key) {}
export function mousepressed(x, y, button) { mouseX = x; mouseY = y; }
export function mousereleased(x, y, button) {}
export function wheelmoved(x, y) {}

import {
    setWindow, clear, present, setColor,
    rectangle, circle, print
} from 'graphics';

// ============================================
// 贪吃蛇游戏
// ============================================

// 游戏设置
const GRID_SIZE = 20;
const CANVAS_WIDTH = 800;
const CANVAS_HEIGHT = 600;
const GRID_WIDTH = Math.floor(CANVAS_WIDTH / GRID_SIZE);
const GRID_HEIGHT = Math.floor(CANVAS_HEIGHT / GRID_SIZE);

// 游戏状态
let snake = [];
let direction = { x: 1, y: 0 };
let nextDirection = { x: 1, y: 0 };
let food = { x: 10, y: 10 };
let score = 0;
let gameSpeed = 0.15; // 移动间隔（秒）
let timeSinceLastMove = 0;
let gameOver = false;
let keys = {};

// 方向常量
const DIRECTIONS = {
    UP: { x: 0, y: -1 },
    DOWN: { x: 0, y: 1 },
    LEFT: { x: -1, y: 0 },
    RIGHT: { x: 1, y: 0 }
};

function initGame() {
    // 初始化蛇（从中心开始）
    const centerX = Math.floor(GRID_WIDTH / 2);
    const centerY = Math.floor(GRID_HEIGHT / 2);
    snake = [
        { x: centerX, y: centerY },
        { x: centerX - 1, y: centerY },
        { x: centerX - 2, y: centerY }
    ];

    direction = { x: 1, y: 0 };
    nextDirection = { x: 1, y: 0 };
    score = 0;
    gameSpeed = 0.15;
    timeSinceLastMove = 0;
    gameOver = false;

    // 生成第一个食物
    generateFood();
}

function generateFood() {
    do {
        food.x = Math.floor(Math.random() * GRID_WIDTH);
        food.y = Math.floor(Math.random() * GRID_HEIGHT);
    } while (snake.some(segment => segment.x === food.x && segment.y === food.y));
}

function updateGame(dt) {
    if (gameOver) return;

    timeSinceLastMove += dt;

    // 控制移动速度
    if (timeSinceLastMove >= gameSpeed) {
        timeSinceLastMove = 0;

        // 应用新方向
        direction = { ...nextDirection };

        // 计算新的头部位置
        const head = { ...snake[0] };
        head.x += direction.x;
        head.y += direction.y;

        // 检查边界碰撞
        if (head.x < 0 || head.x >= GRID_WIDTH ||
            head.y < 0 || head.y >= GRID_HEIGHT) {
            gameOver = true;
            return;
        }

        // 检查自身碰撞
        if (snake.some(segment => segment.x === head.x && segment.y === head.y)) {
            gameOver = true;
            return;
        }

        // 添加新头部
        snake.unshift(head);

        // 检查是否吃到食物
        if (head.x === food.x && head.y === food.y) {
            score += 10;
            generateFood();

            // 随着分数增加，速度加快
            gameSpeed = Math.max(0.05, 0.15 - score * 0.002);
        } else {
            // 移除尾部
            snake.pop();
        }
    }
}

function render() {
    // 清屏
    clear(0.1, 0.1, 0.15, 1);

    // 绘制背景网格（可选）
    setColor(0.15, 0.15, 0.2, 1);
    for (let x = 0; x < GRID_WIDTH; x++) {
        for (let y = 0; y < GRID_HEIGHT; y++) {
            if ((x + y) % 2 === 0) {
                rectangle(x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE, true);
            }
        }
    }

    // 绘制蛇
    for (let i = 0; i < snake.length; i++) {
        const segment = snake[i];
        const brightness = 1 - (i / snake.length) * 0.5;

        // 头部更亮
        if (i === 0) {
            setColor(0.2, 1, 0.4, 1); // 亮绿色
        } else {
            setColor(0.2 * brightness, 0.8 * brightness, 0.3 * brightness, 1);
        }

        rectangle(
            segment.x * GRID_SIZE,
            segment.y * GRID_SIZE,
            GRID_SIZE,
            GRID_SIZE,
            true
        );

        // 给蛇添加边框效果
        setColor(0.1, 0.5, 0.2, 1);
        rectangle(
            segment.x * GRID_SIZE + 2,
            segment.y * GRID_SIZE + 2,
            GRID_SIZE - 4,
            GRID_SIZE - 4,
            false
        );
    }

    // 绘制食物
    setColor(1, 0.3, 0.3, 1);
    circle(
        food.x * GRID_SIZE + GRID_SIZE / 2,
        food.y * GRID_SIZE + GRID_SIZE / 2,
        GRID_SIZE / 2 - 2,
        true
    );

    // 食物光晕效果
    setColor(1, 0.5, 0.5, 0.5);
    circle(
        food.x * GRID_SIZE + GRID_SIZE / 2,
        food.y * GRID_SIZE + GRID_SIZE / 2,
        GRID_SIZE / 2 + 2,
        false
    );

    // 绘制分数
    setColor(1, 1, 1, 1);
    print(`SCORE: ${score}`, 20, 30);
    print(`LENGTH: ${snake.length}`, 20, 55);

    // 游戏结束画面
    if (gameOver) {
        // 半透明遮罩
        setColor(0, 0, 0, 0.7);
        rectangle(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, true);

        // 游戏结束文字
        setColor(1, 0.3, 0.3, 1);
        print("GAME OVER!", CANVAS_WIDTH / 2 - 80, CANVAS_HEIGHT / 2 - 40);

        setColor(1, 1, 1, 1);
        print(`FINAL SCORE: ${score}`, CANVAS_WIDTH / 2 - 100, CANVAS_HEIGHT / 2 + 10);
        print("PRESS R TO RESTART", CANVAS_WIDTH / 2 - 120, CANVAS_HEIGHT / 2 + 50);
    }

    // 控制说明
    if (!gameOver && score === 0) {
        setColor(0.8, 0.8, 0.8, 1);
        print("USE WASD OR ARROW KEYS", CANVAS_WIDTH - 250, CANVAS_HEIGHT - 50);
    }

    present();
}

// ============================================
// 事件处理
// ============================================
export function keypressed(key) {
    keys[key] = true;

    // 方向控制（防止反向移动）
    if (key === 'w' || key === 'arrowup') {
        if (direction.y !== 1) nextDirection = DIRECTIONS.UP;
    } else if (key === 's' || key === 'arrowdown') {
        if (direction.y !== -1) nextDirection = DIRECTIONS.DOWN;
    } else if (key === 'a' || key === 'arrowleft') {
        if (direction.x !== 1) nextDirection = DIRECTIONS.LEFT;
    } else if (key === 'd' || key === 'arrowright') {
        if (direction.x !== -1) nextDirection = DIRECTIONS.RIGHT;
    }

    // 重新开始
    if (key === 'r' && gameOver) {
        initGame();
    }
}

export function keyreleased(key) {
    keys[key] = false;
}

// ============================================
// 主循环
// ============================================
export function load() {
    setWindow("Snake Game", CANVAS_WIDTH, CANVAS_HEIGHT);
    initGame();
}

export function update(dt) {
    updateGame(dt);
}

export function draw() {
    render();
}

export function mousepressed(x, y, button) {}
export function mousereleased(x, y, button) {}
export function wheelmoved(x, y) {}

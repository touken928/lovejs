import {
    setWindow, clear, present, setColor,
    rectangle, print
} from 'graphics';

// ============================================
// 俄罗斯方块游戏
// ============================================

// 游戏设置
const BLOCK_SIZE = 30;
const BOARD_WIDTH = 10;
const BOARD_HEIGHT = 20;
const BOARD_X = 200;  // 游戏区域左上角X
const BOARD_Y = 30;   // 游戏区域左上角Y

// 方块形状定义 (每个方块有4种旋转状态)
const SHAPES = {
    I: [
        [[1,1,1,1]],
        [[1],[1],[1],[1]]
    ],
    O: [
        [[1,1],[1,1]]
    ],
    T: [
        [[0,1,0],[1,1,1]],
        [[1,0],[1,1],[1,0]],
        [[1,1,1],[0,1,0]],
        [[0,1],[1,1],[0,1]]
    ],
    S: [
        [[0,1,1],[1,1,0]],
        [[1,0],[1,1],[0,1]]
    ],
    Z: [
        [[1,1,0],[0,1,1]],
        [[0,1],[1,1],[1,0]]
    ],
    J: [
        [[1,0,0],[1,1,1]],
        [[1,1],[1,0],[1,0]],
        [[1,1,1],[0,0,1]],
        [[0,1],[0,1],[1,1]]
    ],
    L: [
        [[0,0,1],[1,1,1]],
        [[1,0],[1,0],[1,1]],
        [[1,1,1],[1,0,0]],
        [[1,1],[0,1],[0,1]]
    ]
};

// 方块颜色
const COLORS = {
    I: [0, 1, 1, 1],      // 青色
    O: [1, 1, 0, 1],      // 黄色
    T: [0.6, 0, 0.8, 1],  // 紫色
    S: [0, 1, 0, 1],      // 绿色
    Z: [1, 0, 0, 1],      // 红色
    J: [0, 0, 1, 1],      // 蓝色
    L: [1, 0.5, 0, 1]     // 橙色
};

const SHAPE_NAMES = ['I', 'O', 'T', 'S', 'Z', 'J', 'L'];

// 游戏状态
let board = [];
let currentPiece = null;
let nextPiece = null;
let score = 0;
let level = 1;
let lines = 0;
let gameOver = false;
let paused = false;
let dropTimer = 0;
let dropInterval = 1.0;  // 下落间隔（秒）

function initGame() {
    // 初始化游戏板
    board = [];
    for (let y = 0; y < BOARD_HEIGHT; y++) {
        board.push(new Array(BOARD_WIDTH).fill(null));
    }
    
    score = 0;
    level = 1;
    lines = 0;
    gameOver = false;
    paused = false;
    dropTimer = 0;
    dropInterval = 1.0;
    
    nextPiece = createPiece();
    spawnPiece();
}

function createPiece() {
    const name = SHAPE_NAMES[Math.floor(Math.random() * SHAPE_NAMES.length)];
    return {
        name: name,
        shapes: SHAPES[name],
        rotation: 0,
        x: 0,
        y: 0,
        color: COLORS[name]
    };
}

function getShape(piece) {
    return piece.shapes[piece.rotation % piece.shapes.length];
}

function spawnPiece() {
    currentPiece = nextPiece;
    nextPiece = createPiece();
    
    const shape = getShape(currentPiece);
    currentPiece.x = Math.floor((BOARD_WIDTH - shape[0].length) / 2);
    currentPiece.y = 0;
    
    // 检查游戏结束
    if (!isValidPosition(currentPiece, currentPiece.x, currentPiece.y)) {
        gameOver = true;
    }
}

function isValidPosition(piece, newX, newY, newRotation) {
    const rot = newRotation !== undefined ? newRotation : piece.rotation;
    const shape = piece.shapes[rot % piece.shapes.length];
    
    for (let y = 0; y < shape.length; y++) {
        for (let x = 0; x < shape[y].length; x++) {
            if (shape[y][x]) {
                const boardX = newX + x;
                const boardY = newY + y;
                
                // 边界检查
                if (boardX < 0 || boardX >= BOARD_WIDTH || boardY >= BOARD_HEIGHT) {
                    return false;
                }
                
                // 碰撞检查
                if (boardY >= 0 && board[boardY][boardX] !== null) {
                    return false;
                }
            }
        }
    }
    return true;
}


function lockPiece() {
    const shape = getShape(currentPiece);
    
    for (let y = 0; y < shape.length; y++) {
        for (let x = 0; x < shape[y].length; x++) {
            if (shape[y][x]) {
                const boardY = currentPiece.y + y;
                const boardX = currentPiece.x + x;
                if (boardY >= 0) {
                    board[boardY][boardX] = currentPiece.color;
                }
            }
        }
    }
    
    clearLines();
    spawnPiece();
}

function clearLines() {
    let clearedLines = 0;
    
    for (let y = BOARD_HEIGHT - 1; y >= 0; y--) {
        if (board[y].every(cell => cell !== null)) {
            board.splice(y, 1);
            board.unshift(new Array(BOARD_WIDTH).fill(null));
            clearedLines++;
            y++; // 重新检查当前行
        }
    }
    
    if (clearedLines > 0) {
        // 计分：1行=100, 2行=300, 3行=500, 4行=800
        const points = [0, 100, 300, 500, 800];
        score += points[clearedLines] * level;
        lines += clearedLines;
        
        // 每10行升一级
        level = Math.floor(lines / 10) + 1;
        dropInterval = Math.max(0.1, 1.0 - (level - 1) * 0.1);
    }
}

function movePiece(dx, dy) {
    if (isValidPosition(currentPiece, currentPiece.x + dx, currentPiece.y + dy)) {
        currentPiece.x += dx;
        currentPiece.y += dy;
        return true;
    }
    return false;
}

function rotatePiece() {
    const newRotation = (currentPiece.rotation + 1) % currentPiece.shapes.length;
    
    // 尝试原位旋转
    if (isValidPosition(currentPiece, currentPiece.x, currentPiece.y, newRotation)) {
        currentPiece.rotation = newRotation;
        return;
    }
    
    // 墙踢：尝试左右移动后旋转
    const kicks = [-1, 1, -2, 2];
    for (const kick of kicks) {
        if (isValidPosition(currentPiece, currentPiece.x + kick, currentPiece.y, newRotation)) {
            currentPiece.x += kick;
            currentPiece.rotation = newRotation;
            return;
        }
    }
}

function hardDrop() {
    while (movePiece(0, 1)) {
        score += 2;
    }
    lockPiece();
}

function updateGame(dt) {
    if (gameOver || paused) return;
    
    dropTimer += dt;
    if (dropTimer >= dropInterval) {
        dropTimer = 0;
        if (!movePiece(0, 1)) {
            lockPiece();
        }
    }
}

function drawBlock(x, y, color, ghost = false) {
    const px = BOARD_X + x * BLOCK_SIZE;
    const py = BOARD_Y + y * BLOCK_SIZE;
    
    if (ghost) {
        setColor(color[0], color[1], color[2], 0.3);
        rectangle(px + 1, py + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2, true);
    } else {
        // 主体
        setColor(color[0], color[1], color[2], color[3]);
        rectangle(px + 1, py + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2, true);
        
        // 高光
        setColor(1, 1, 1, 0.3);
        rectangle(px + 1, py + 1, BLOCK_SIZE - 2, 4, true);
        rectangle(px + 1, py + 1, 4, BLOCK_SIZE - 2, true);
        
        // 阴影
        setColor(0, 0, 0, 0.3);
        rectangle(px + BLOCK_SIZE - 5, py + 1, 4, BLOCK_SIZE - 2, true);
        rectangle(px + 1, py + BLOCK_SIZE - 5, BLOCK_SIZE - 2, 4, true);
    }
}

function getGhostY() {
    let ghostY = currentPiece.y;
    while (isValidPosition(currentPiece, currentPiece.x, ghostY + 1)) {
        ghostY++;
    }
    return ghostY;
}


function render() {
    clear(0.1, 0.1, 0.15, 1);
    
    // 绘制游戏区域边框
    setColor(0.3, 0.3, 0.4, 1);
    rectangle(BOARD_X - 2, BOARD_Y - 2, BOARD_WIDTH * BLOCK_SIZE + 4, BOARD_HEIGHT * BLOCK_SIZE + 4, false);
    
    // 绘制游戏区域背景
    setColor(0.05, 0.05, 0.1, 1);
    rectangle(BOARD_X, BOARD_Y, BOARD_WIDTH * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE, true);
    
    // 绘制网格
    setColor(0.15, 0.15, 0.2, 1);
    for (let x = 0; x <= BOARD_WIDTH; x++) {
        rectangle(BOARD_X + x * BLOCK_SIZE, BOARD_Y, 1, BOARD_HEIGHT * BLOCK_SIZE, true);
    }
    for (let y = 0; y <= BOARD_HEIGHT; y++) {
        rectangle(BOARD_X, BOARD_Y + y * BLOCK_SIZE, BOARD_WIDTH * BLOCK_SIZE, 1, true);
    }
    
    // 绘制已固定的方块
    for (let y = 0; y < BOARD_HEIGHT; y++) {
        for (let x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] !== null) {
                drawBlock(x, y, board[y][x]);
            }
        }
    }
    
    // 绘制幽灵方块（预览落点）
    if (currentPiece && !gameOver) {
        const ghostY = getGhostY();
        const shape = getShape(currentPiece);
        for (let y = 0; y < shape.length; y++) {
            for (let x = 0; x < shape[y].length; x++) {
                if (shape[y][x]) {
                    drawBlock(currentPiece.x + x, ghostY + y, currentPiece.color, true);
                }
            }
        }
    }
    
    // 绘制当前方块
    if (currentPiece && !gameOver) {
        const shape = getShape(currentPiece);
        for (let y = 0; y < shape.length; y++) {
            for (let x = 0; x < shape[y].length; x++) {
                if (shape[y][x] && currentPiece.y + y >= 0) {
                    drawBlock(currentPiece.x + x, currentPiece.y + y, currentPiece.color);
                }
            }
        }
    }
    
    // 绘制侧边信息
    const infoX = BOARD_X + BOARD_WIDTH * BLOCK_SIZE + 40;
    
    // 下一个方块预览
    setColor(1, 1, 1, 1);
    print("NEXT", infoX, 50);
    
    setColor(0.2, 0.2, 0.3, 1);
    rectangle(infoX, 70, 120, 80, true);
    setColor(0.3, 0.3, 0.4, 1);
    rectangle(infoX, 70, 120, 80, false);
    
    if (nextPiece) {
        const shape = nextPiece.shapes[0];
        const offsetX = (120 - shape[0].length * BLOCK_SIZE) / 2;
        const offsetY = (80 - shape.length * BLOCK_SIZE) / 2;
        for (let y = 0; y < shape.length; y++) {
            for (let x = 0; x < shape[y].length; x++) {
                if (shape[y][x]) {
                    const px = infoX + offsetX + x * BLOCK_SIZE;
                    const py = 70 + offsetY + y * BLOCK_SIZE;
                    setColor(nextPiece.color[0], nextPiece.color[1], nextPiece.color[2], 1);
                    rectangle(px + 1, py + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2, true);
                }
            }
        }
    }
    
    // 分数信息
    setColor(1, 1, 1, 1);
    print("SCORE", infoX, 180);
    setColor(0, 1, 1, 1);
    print(`${score}`, infoX, 205);
    
    setColor(1, 1, 1, 1);
    print("LEVEL", infoX, 250);
    setColor(0, 1, 0, 1);
    print(`${level}`, infoX, 275);
    
    setColor(1, 1, 1, 1);
    print("LINES", infoX, 320);
    setColor(1, 1, 0, 1);
    print(`${lines}`, infoX, 345);
    
    // 控制说明
    setColor(0.6, 0.6, 0.6, 1);
    print("CONTROLS:", infoX, 420);
    print("A/D - Move", infoX, 445);
    print("W - Rotate", infoX, 470);
    print("S - Soft Drop", infoX, 495);
    print("SPACE - Hard Drop", infoX, 520);
    print("P - Pause", infoX, 545);
    
    // 游戏结束
    if (gameOver) {
        setColor(0, 0, 0, 0.7);
        rectangle(BOARD_X, BOARD_Y, BOARD_WIDTH * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE, true);
        
        setColor(1, 0.3, 0.3, 1);
        print("GAME OVER!", BOARD_X + 60, BOARD_Y + 250);
        
        setColor(1, 1, 1, 1);
        print(`SCORE: ${score}`, BOARD_X + 80, BOARD_Y + 290);
        print("PRESS R TO RESTART", BOARD_X + 40, BOARD_Y + 330);
    }
    
    // 暂停
    if (paused && !gameOver) {
        setColor(0, 0, 0, 0.7);
        rectangle(BOARD_X, BOARD_Y, BOARD_WIDTH * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE, true);
        
        setColor(1, 1, 0, 1);
        print("PAUSED", BOARD_X + 100, BOARD_Y + 280);
    }
    
    present();
}

// ============================================
// 事件处理
// ============================================
export function keypressed(key) {
    if (gameOver) {
        if (key === 'r') initGame();
        return;
    }
    
    if (key === 'p') {
        paused = !paused;
        return;
    }
    
    if (paused) return;
    
    if (key === 'a' || key === 'arrowleft') {
        movePiece(-1, 0);
    } else if (key === 'd' || key === 'arrowright') {
        movePiece(1, 0);
    } else if (key === 's' || key === 'arrowdown') {
        if (movePiece(0, 1)) score += 1;
    } else if (key === 'w' || key === 'arrowup') {
        rotatePiece();
    } else if (key === 'space') {
        hardDrop();
    }
}

export function keyreleased(key) {}

// ============================================
// 主循环
// ============================================
export function load() {
    setWindow("Tetris", 700, 650);
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

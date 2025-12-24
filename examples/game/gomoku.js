import {
    setWindow, clear, present, setColor,
    rectangle, circle, line, print
} from 'graphics';

// ============================================
// 五子棋游戏（双人对战）
// ============================================

// 游戏设置
const BOARD_SIZE = 15;       // 15x15 棋盘
const CELL_SIZE = 40;        // 每格大小
const BOARD_PADDING = 50;    // 棋盘边距
const STONE_RADIUS = 16;     // 棋子半径

// 计算窗口大小
const WINDOW_WIDTH = BOARD_SIZE * CELL_SIZE + BOARD_PADDING * 2 + 200;
const WINDOW_HEIGHT = BOARD_SIZE * CELL_SIZE + BOARD_PADDING * 2;

// 游戏状态
let board = [];              // 0=空, 1=黑, 2=白
let currentPlayer = 1;       // 1=黑, 2=白
let gameOver = false;
let winner = 0;
let lastMove = null;         // 最后落子位置
let hoverPos = null;         // 鼠标悬停位置
let moveHistory = [];        // 落子历史

// 初始化游戏
function initGame() {
    board = [];
    for (let y = 0; y < BOARD_SIZE; y++) {
        board.push(new Array(BOARD_SIZE).fill(0));
    }
    currentPlayer = 1;
    gameOver = false;
    winner = 0;
    lastMove = null;
    moveHistory = [];
}

// 屏幕坐标转棋盘坐标
function screenToBoard(sx, sy) {
    const x = Math.round((sx - BOARD_PADDING) / CELL_SIZE);
    const y = Math.round((sy - BOARD_PADDING) / CELL_SIZE);
    if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
        return { x, y };
    }
    return null;
}

// 棋盘坐标转屏幕坐标
function boardToScreen(bx, by) {
    return {
        x: BOARD_PADDING + bx * CELL_SIZE,
        y: BOARD_PADDING + by * CELL_SIZE
    };
}

// 落子
function placeStone(x, y) {
    if (gameOver || board[y][x] !== 0) return false;
    
    board[y][x] = currentPlayer;
    lastMove = { x, y };
    moveHistory.push({ x, y, player: currentPlayer });
    
    // 检查胜利
    if (checkWin(x, y, currentPlayer)) {
        gameOver = true;
        winner = currentPlayer;
    } else {
        currentPlayer = currentPlayer === 1 ? 2 : 1;
    }
    
    return true;
}

// 检查胜利（四个方向）
function checkWin(x, y, player) {
    const directions = [
        [1, 0],   // 水平
        [0, 1],   // 垂直
        [1, 1],   // 对角线
        [1, -1]   // 反对角线
    ];
    
    for (const [dx, dy] of directions) {
        let count = 1;
        
        // 正方向
        for (let i = 1; i < 5; i++) {
            const nx = x + dx * i;
            const ny = y + dy * i;
            if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
            if (board[ny][nx] !== player) break;
            count++;
        }
        
        // 反方向
        for (let i = 1; i < 5; i++) {
            const nx = x - dx * i;
            const ny = y - dy * i;
            if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
            if (board[ny][nx] !== player) break;
            count++;
        }
        
        if (count >= 5) return true;
    }
    
    return false;
}

// 悔棋
function undoMove() {
    if (moveHistory.length === 0 || gameOver) return;
    
    const last = moveHistory.pop();
    board[last.y][last.x] = 0;
    currentPlayer = last.player;
    lastMove = moveHistory.length > 0 ? moveHistory[moveHistory.length - 1] : null;
}

// 绘制棋盘
function drawBoard() {
    // 棋盘背景
    setColor(0.87, 0.72, 0.53, 1);  // 木色
    rectangle(BOARD_PADDING - CELL_SIZE/2, BOARD_PADDING - CELL_SIZE/2, 
              BOARD_SIZE * CELL_SIZE, BOARD_SIZE * CELL_SIZE, true);
    
    // 绘制网格线
    setColor(0.2, 0.15, 0.1, 1);
    for (let i = 0; i < BOARD_SIZE; i++) {
        // 横线
        const y = BOARD_PADDING + i * CELL_SIZE;
        line(BOARD_PADDING, y, BOARD_PADDING + (BOARD_SIZE - 1) * CELL_SIZE, y);
        // 竖线
        const x = BOARD_PADDING + i * CELL_SIZE;
        line(x, BOARD_PADDING, x, BOARD_PADDING + (BOARD_SIZE - 1) * CELL_SIZE);
    }
    
    // 绘制星位（天元和四个角星）
    const starPoints = [
        [3, 3], [3, 11], [11, 3], [11, 11],  // 四角星
        [7, 7]  // 天元
    ];
    setColor(0.2, 0.15, 0.1, 1);
    for (const [sx, sy] of starPoints) {
        const pos = boardToScreen(sx, sy);
        circle(pos.x, pos.y, 4, true);
    }
}

// 绘制棋子
function drawStones() {
    for (let y = 0; y < BOARD_SIZE; y++) {
        for (let x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x] !== 0) {
                const pos = boardToScreen(x, y);
                
                // 棋子阴影
                setColor(0, 0, 0, 0.3);
                circle(pos.x + 2, pos.y + 2, STONE_RADIUS, true);
                
                // 棋子本体
                if (board[y][x] === 1) {
                    // 黑子
                    setColor(0.1, 0.1, 0.1, 1);
                    circle(pos.x, pos.y, STONE_RADIUS, true);
                    // 高光
                    setColor(0.4, 0.4, 0.4, 0.5);
                    circle(pos.x - 4, pos.y - 4, 5, true);
                } else {
                    // 白子
                    setColor(0.95, 0.95, 0.95, 1);
                    circle(pos.x, pos.y, STONE_RADIUS, true);
                    // 边框
                    setColor(0.7, 0.7, 0.7, 1);
                    circle(pos.x, pos.y, STONE_RADIUS, false);
                    // 高光
                    setColor(1, 1, 1, 0.8);
                    circle(pos.x - 4, pos.y - 4, 5, true);
                }
                
                // 标记最后落子
                if (lastMove && lastMove.x === x && lastMove.y === y) {
                    setColor(1, 0, 0, 1);
                    circle(pos.x, pos.y, 5, true);
                }
            }
        }
    }
}

// 绘制悬停预览
function drawHover() {
    if (!hoverPos || gameOver || board[hoverPos.y][hoverPos.x] !== 0) return;
    
    const pos = boardToScreen(hoverPos.x, hoverPos.y);
    
    if (currentPlayer === 1) {
        setColor(0.1, 0.1, 0.1, 0.5);
    } else {
        setColor(0.9, 0.9, 0.9, 0.5);
    }
    circle(pos.x, pos.y, STONE_RADIUS, true);
}

// 绘制信息面板
function drawInfo() {
    const infoX = BOARD_PADDING + BOARD_SIZE * CELL_SIZE + 30;
    
    // 标题
    setColor(1, 1, 1, 1);
    print("GOMOKU", infoX + 15, 50);
    
    // 当前玩家
    setColor(0.8, 0.8, 0.8, 1);
    print("Current:", infoX, 120);
    
    if (!gameOver) {
        if (currentPlayer === 1) {
            setColor(0.1, 0.1, 0.1, 1);
            circle(infoX + 80, 145, 15, true);
            setColor(1, 1, 1, 1);
            print("BLACK", infoX + 30, 170);
        } else {
            setColor(0.95, 0.95, 0.95, 1);
            circle(infoX + 80, 145, 15, true);
            setColor(0.7, 0.7, 0.7, 1);
            circle(infoX + 80, 145, 15, false);
            setColor(1, 1, 1, 1);
            print("WHITE", infoX + 30, 170);
        }
    }
    
    // 落子数
    setColor(0.8, 0.8, 0.8, 1);
    print("Moves:", infoX + 5, 220);
    setColor(0, 1, 1, 1);
    print(`${moveHistory.length}`, infoX + 80, 220);
    
    // 控制说明
    setColor(0.6, 0.6, 0.6, 1);
    print("CONTROLS:", infoX, 300);
    print("Click-Place", infoX, 330);
    print("U - Undo", infoX + 5, 355);
    print("R - Restart", infoX, 380);
    
    // 胜利信息
    if (gameOver) {
        setColor(0, 0, 0, 0.5);
        rectangle(0, WINDOW_HEIGHT / 2 - 60, WINDOW_WIDTH, 120, true);
        
        if (winner === 1) {
            setColor(1, 0.8, 0, 1);
            print("BLACK WINS!", WINDOW_WIDTH / 2 - 70, WINDOW_HEIGHT / 2 - 20);
        } else {
            setColor(1, 0.8, 0, 1);
            print("WHITE WINS!", WINDOW_WIDTH / 2 - 70, WINDOW_HEIGHT / 2 - 20);
        }
        
        setColor(1, 1, 1, 1);
        print("Press R", WINDOW_WIDTH / 2 - 40, WINDOW_HEIGHT / 2 + 20);
    }
}

function render() {
    clear(0.15, 0.15, 0.2, 1);
    
    drawBoard();
    drawStones();
    drawHover();
    drawInfo();
    
    present();
}

// ============================================
// 事件处理
// ============================================
export function keypressed(key) {
    if (key === 'r') {
        initGame();
    } else if (key === 'u') {
        undoMove();
    }
}

export function keyreleased(key) {}

export function mousepressed(x, y, button) {
    if (button === 1 && !gameOver) {
        const pos = screenToBoard(x, y);
        if (pos) {
            placeStone(pos.x, pos.y);
        }
    }
}

export function mousereleased(x, y, button) {}

export function wheelmoved(x, y) {}

// 鼠标移动更新悬停位置（通过update检测）
let mouseX = 0, mouseY = 0;

// ============================================
// 主循环
// ============================================
export function load() {
    setWindow("Gomoku - Five in a Row", WINDOW_WIDTH, WINDOW_HEIGHT);
    initGame();
}

export function update(dt) {
    // 更新悬停位置（简化处理，实际需要鼠标移动事件）
}

export function draw() {
    render();
}

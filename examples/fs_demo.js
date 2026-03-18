import * as graphics from 'graphics';
import * as fs from 'fs';

let message = "loading...";

export async function load() {
    graphics.setWindow("FS Demo", 800, 600);

    try {
        // 读取 README.md 的前 80 个字符，然后写入一个临时文件
        const text = await fs.readFile("README.md");
        await fs.writeFile("tmp_fs_demo.txt", text.slice(0, 80));
        message = "FS OK: read README.md and wrote tmp_fs_demo.txt";
    } catch (err) {
        message = "FS Error: " + err.message;
    }
}

export function update(dt) {
    // nothing
}

export function draw() {
    graphics.clear(0.05, 0.05, 0.08, 1.0);
    graphics.setColor(1, 1, 1, 1);
    graphics.print(message, 20, 20);
    graphics.present();
}


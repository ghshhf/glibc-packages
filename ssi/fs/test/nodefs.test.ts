/**
 * SSI-FS: NODEFS Backend Tests
 *
 * Tests the Node.js native file system backend.
 * Requires a real filesystem — skipped in browser environments.
 */

import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { NodeFsBackend } from '../src/nodefs';
import * as fs from 'node:fs';
import * as path from 'node:path';
import * as os from 'node:os';

import {
  O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC, O_APPEND,
  SEEK_SET, SEEK_CUR, SEEK_END,
  SsiErrorCode,
} from '../src/backends';

// =========================================================================
// Test Helpers
// =========================================================================

function tmpDir(): string {
  return fs.mkdtempSync(path.join(os.tmpdir(), 'ssi-fs-nodefs-'));
}

function cleanDir(dir: string): void {
  try { fs.rmSync(dir, { recursive: true, force: true }); } catch { /* ignore */ }
}

// =========================================================================
// Tests
// =========================================================================

describe('NodeFsBackend', () => {
  let backend: NodeFsBackend;
  let testRoot: string;

  beforeEach(() => {
    testRoot = tmpDir();
    backend = new NodeFsBackend(testRoot);
    backend.init({ root: testRoot });
  });

  afterEach(() => {
    backend.destroy();
    cleanDir(testRoot);
  });

  // ── Lifecycle ──

  it('should init with a root directory', () => {
    expect(fs.existsSync(testRoot)).toBe(true);
  });

  // ── File Operations ──

  it('should open a new file with O_CREAT | O_RDWR', () => {
    const err = backend.open('/test.txt', O_CREAT | O_RDWR);
    expect(err).toBe(SsiErrorCode.OK);
    expect(fs.existsSync(path.join(testRoot, 'test.txt'))).toBe(true);
  });

  it('should fail to open non-existent file without O_CREAT', () => {
    const err = backend.open('/nonexistent.txt', O_RDONLY);
    expect(err).toBe(SsiErrorCode.NOT_FOUND);
  });

  it('should write and read back data', () => {
    const err = backend.open('/hello.txt', O_CREAT | O_RDWR);
    expect(err).toBe(SsiErrorCode.OK);

    const data = new TextEncoder().encode('Hello, SkyNet!').buffer;
    const writeErr = backend.write(3, data);
    expect(writeErr).toBe(SsiErrorCode.OK);

    // Seek to beginning
    const seekErr = backend.seek(3, 0, SEEK_SET);
    expect(seekErr).toBe(SsiErrorCode.OK);

    // Read back
    const result = backend.read(3, 1024);
    expect(result.error).toBe(SsiErrorCode.OK);
    expect(result.data).not.toBeNull();
    const text = new TextDecoder().decode(result.data!);
    expect(text).toBe('Hello, SkyNet!');
  });

  it('should handle read beyond file size gracefully', () => {
    backend.open('/small.txt', O_CREAT | O_RDWR);
    const data = new TextEncoder().encode('Hi').buffer;
    backend.write(3, data);
    backend.seek(3, 0, SEEK_SET);

    const result = backend.read(3, 9999);
    expect(result.error).toBe(SsiErrorCode.OK);
    expect(result.data).not.toBeNull();
    expect(new TextDecoder().decode(result.data!)).toBe('Hi');
  });

  it('should seek correctly (SET / CUR / END)', () => {
    backend.open('/seek.txt', O_CREAT | O_RDWR);
    const data = new TextEncoder().encode('0123456789').buffer;
    backend.write(3, data);

    // SEEK_SET
    backend.seek(3, 3, SEEK_SET);
    const r1 = backend.read(3, 3);
    expect(new TextDecoder().decode(r1.data!)).toBe('345');

    // SEEK_CUR (was 6, +2 = 8, only 2 bytes left)
    backend.seek(3, 2, SEEK_CUR);
    const r2 = backend.read(3, 3);
    expect(new TextDecoder().decode(r2.data!)).toBe('89');

    // SEEK_END (0 from end = empty read)
    backend.seek(3, 0, SEEK_END);
    const r3 = backend.read(3, 3);
    expect(new TextDecoder().decode(r3.data!)).toBe('');
  });

  it('should close a file descriptor', () => {
    backend.open('/close.txt', O_CREAT | O_RDWR);
    expect(backend.close(3)).toBe(SsiErrorCode.OK);
    expect(backend.close(3)).toBe(SsiErrorCode.NOT_FOUND); // already closed
  });

  it('should reject write on read-only fd', () => {
    backend.open('/ro.txt', O_CREAT | O_RDWR);
    backend.close(3);

    // NOTE: O_RDONLY fails if file doesn't exist without O_CREAT.
    // So we open with O_RDONLY on existing file.
    const roFd = () => {
      const err = backend.open('/ro.txt', O_RDONLY);
      return err === SsiErrorCode.OK ? 3 : -1;
    };

    // Re-open as read-only
    // The file was opened with fd 3, closed, so next open reuses fd 3
    backend.open('/ro.txt', O_RDONLY);
    const writeErr = backend.write(3, new ArrayBuffer(10));
    expect(writeErr).toBe(SsiErrorCode.PERMISSION);
  });

  // ── Directory Operations ──

  it('should create a directory', () => {
    expect(backend.mkdir('/mydir', 0o755)).toBe(SsiErrorCode.OK);
    const st = fs.statSync(path.join(testRoot, 'mydir'));
    expect(st.isDirectory()).toBe(true);
  });

  it('should reject duplicate mkdir', () => {
    backend.mkdir('/dup', 0o755);
    expect(backend.mkdir('/dup', 0o755)).toBe(SsiErrorCode.BUSY);
  });

  it('should list directory contents', () => {
    backend.mkdir('/listdir', 0o755);
    backend.open('/listdir/a.txt', O_CREAT | O_RDWR);
    backend.close(3);
    backend.open('/listdir/b.txt', O_CREAT | O_RDWR);
    backend.close(3);

    const result = backend.readdir('/listdir');
    expect(result.error).toBe(SsiErrorCode.OK);
    const names = result.entries.map(e => e.name).sort();
    expect(names).toEqual(['a.txt', 'b.txt']);
  });

  it('should stat a file', () => {
    backend.open('/stat.txt', O_CREAT | O_RDWR);
    const data = new TextEncoder().encode('12345').buffer;
    backend.write(3, data);
    backend.close(3);

    const result = backend.stat('/stat.txt');
    expect(result.error).toBe(SsiErrorCode.OK);
    expect(result.stat).not.toBeNull();
    expect(result.stat!.size).toBe(5);
    expect(result.stat!.isFile).toBe(true);
    expect(result.stat!.isDirectory).toBe(false);
  });

  it('should return NOT_FOUND for nonexistent stat', () => {
    const result = backend.stat('/no-such-file');
    expect(result.error).toBe(SsiErrorCode.NOT_FOUND);
    expect(result.stat).toBeNull();
  });

  it('should unlink a file', () => {
    backend.open('/delete.txt', O_CREAT | O_RDWR);
    backend.close(3);
    expect(fs.existsSync(path.join(testRoot, 'delete.txt'))).toBe(true);

    expect(backend.unlink('/delete.txt')).toBe(SsiErrorCode.OK);
    expect(fs.existsSync(path.join(testRoot, 'delete.txt'))).toBe(false);
  });

  it('should rename a file', () => {
    backend.open('/old.txt', O_CREAT | O_RDWR);
    backend.close(3);

    console.log('[DEBUG] testRoot:', testRoot);
    console.log('[DEBUG] old.txt exists:', fs.existsSync(path.join(testRoot, 'old.txt')));
    console.log('[DEBUG] rename result:', backend.rename('/old.txt', '/new.txt'));
    expect(backend.rename('/old.txt', '/new.txt')).toBe(SsiErrorCode.OK);
    expect(fs.existsSync(path.join(testRoot, 'old.txt'))).toBe(false);
    expect(fs.existsSync(path.join(testRoot, 'new.txt'))).toBe(true);
  });
});

// =========================================================================
// 安全沙箱测试
// =========================================================================

describe('NodeFsBackend Security', () => {
  it('should reject path traversal attempts', () => {
    const testRoot = tmpDir();
    const backend = new NodeFsBackend(testRoot);
    backend.init({ root: testRoot });

    // 尝试逃逸到 /etc
    const result = backend.stat('/../../../etc/passwd');
    expect(result.error).toBe(SsiErrorCode.NOT_FOUND);


    backend.destroy();
    cleanDir(testRoot);
  });

  it('should be isolated to its root directory', () => {
    const testRoot = tmpDir();
    const backend = new NodeFsBackend(testRoot);
    backend.init({ root: testRoot });

    // 在根目录外创建文件
    const outsideFile = path.join(os.tmpdir(), 'ssi-fs-outside-test');
    fs.writeFileSync(outsideFile, 'you should not see this');

    // 通过 NODEFS 应无法访问
    const result = backend.stat('/../ssi-fs-outside-test');
    expect(result.error).toBe(SsiErrorCode.NOT_FOUND);

    // 清理
    backend.destroy();
    cleanDir(testRoot);
    try { fs.unlinkSync(outsideFile); } catch { /* ignore */ }
  });
});
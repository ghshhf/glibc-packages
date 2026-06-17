/**
 * SSI-FS: File System Tests
 *
 * Tests for the virtual file system implementation.
 * Covers: MEMFS backend, mount/unmount, file operations, directory ops.
 */

import { describe, it, expect, beforeEach } from 'vitest';
import { FileSystem } from '../src/index';
import {
  MemFsBackend,
  O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC, O_APPEND,
  SEEK_SET, SEEK_CUR, SEEK_END,
  SsiErrorCode,
} from '../src/index';

// =========================================================================
// FileSystem Class Tests
// =========================================================================

describe('FileSystem (SSI-FS)', () => {
  let fs: FileSystem;

  beforeEach(async () => {
    fs = new FileSystem();
    await fs.init({});
    await fs.start();
  });

  afterEach(async () => {
    await fs.stop();
    fs.destroy();
  });

  // ── File Operations ──

  it('should open a file with O_CREAT', () => {
    const err = fs.open('/test.txt', O_CREAT | O_RDWR);
    expect(err).toBe(SsiErrorCode.OK);
  });

  it('should fail to open non-existent file without O_CREAT', () => {
    const err = fs.open('/nonexistent.txt', O_RDONLY);
    expect(err).toBe(SsiErrorCode.NOT_FOUND);
  });

  it('should write and read back data', () => {
    const err = fs.open('/hello.txt', O_CREAT | O_RDWR);
    expect(err).toBe(SsiErrorCode.OK);

    const data = new TextEncoder().encode('Hello, SkyNet!').buffer;
    const writeErr = fs.write(3, data);
    expect(writeErr).toBe(SsiErrorCode.OK);

    // Seek to beginning
    const seekErr = fs.seek(3, 0, SEEK_SET);
    expect(seekErr).toBe(SsiErrorCode.OK);

    // Read back
    const result = fs.read(3, 1024);
    expect(result.error).toBe(SsiErrorCode.OK);
    expect(result.data).not.toBeNull();
    const text = new TextDecoder().decode(result.data!);
    expect(text).toBe('Hello, SkyNet!');
  });

  it('should seek correctly (SET/CUR/END)', () => {
    fs.open('/seek.txt', O_CREAT | O_RDWR);
    const data = new TextEncoder().encode('0123456789').buffer;
    fs.write(3, data);

    // SEEK_SET
    fs.seek(3, 3, SEEK_SET);
    const r1 = fs.read(3, 3);
    expect(new TextDecoder().decode(r1.data!)).toBe('345');

    // SEEK_CUR (position was 6, now 8, only 2 bytes left)
    fs.seek(3, 2, SEEK_CUR);
    const r2 = fs.read(3, 3);
    expect(new TextDecoder().decode(r2.data!)).toBe('89');

    // SEEK_END (position 10, offset -3 = 7, reads 3 bytes)
    fs.seek(3, -3, SEEK_END);
    const r3 = fs.read(3, 3);
    expect(new TextDecoder().decode(r3.data!)).toBe('789');
  });

  it('should tell current position', () => {
    fs.open('/tell.txt', O_CREAT | O_RDWR);
    const data = new TextEncoder().encode('Hello').buffer;
    fs.write(3, data);

    const result = fs.tell(3);
    expect(result.error).toBe(SsiErrorCode.OK);
    expect(result.position).toBe(5);
  });

  it('should close a file descriptor', () => {
    fs.open('/close.txt', O_CREAT | O_RDWR);
    const closeErr = fs.close(3);
    expect(closeErr).toBe(SsiErrorCode.OK);

    // Should fail to read from closed fd
    const readResult = fs.read(3, 10);
    expect(readResult.error).toBe(SsiErrorCode.NOT_FOUND);
  });

  // ── Directory Operations ──

  it('should create directories', () => {
    const err = fs.mkdir('/mydir');
    expect(err).toBe(SsiErrorCode.OK);

    // Nested
    const err2 = fs.mkdir('/mydir/subdir');
    expect(err2).toBe(SsiErrorCode.OK);
  });

  it('should read directory entries', () => {
    fs.mkdir('/mydir');
    fs.open('/mydir/file1.txt', O_CREAT | O_RDWR);
    fs.open('/mydir/file2.txt', O_CREAT | O_RDWR);

    const result = fs.readdir('/mydir');
    expect(result.error).toBe(SsiErrorCode.OK);
    expect(result.entries.length).toBe(4); // ., .., file1.txt, file2.txt
    expect(result.entries.some(e => e.name === 'file1.txt')).toBe(true);
    expect(result.entries.some(e => e.name === 'file2.txt')).toBe(true);
  });

  it('should stat files and directories', () => {
    fs.mkdir('/mydir');
    fs.open('/file.txt', O_CREAT | O_RDWR);
    fs.write(3, new TextEncoder().encode('data').buffer);

    const dirStat = fs.stat('/mydir');
    expect(dirStat.error).toBe(SsiErrorCode.OK);
    expect(dirStat.stat!.isDirectory).toBe(true);
    expect(dirStat.stat!.isFile).toBe(false);

    const fileStat = fs.stat('/file.txt');
    expect(fileStat.error).toBe(SsiErrorCode.OK);
    expect(fileStat.stat!.isDirectory).toBe(false);
    expect(fileStat.stat!.isFile).toBe(true);
    expect(fileStat.stat!.size).toBe(4);
  });

  it('should unlink a file', () => {
    fs.open('/delete.txt', O_CREAT | O_RDWR);
    fs.close(3);
    const unlinkErr = fs.unlink('/delete.txt');
    expect(unlinkErr).toBe(SsiErrorCode.OK);

    const statResult = fs.stat('/delete.txt');
    expect(statResult.error).toBe(SsiErrorCode.NOT_FOUND);
  });

  it('should rename files', () => {
    fs.open('/old.txt', O_CREAT | O_RDWR);
    fs.write(3, new TextEncoder().encode('rename test').buffer);
    fs.close(3);

    const renameErr = fs.rename('/old.txt', '/new.txt');
    expect(renameErr).toBe(SsiErrorCode.OK);

    expect(fs.stat('/old.txt').error).toBe(SsiErrorCode.NOT_FOUND);
    expect(fs.stat('/new.txt').error).toBe(SsiErrorCode.OK);
  });

  // ── Mount Operations ──

  it('should mount and unmount a backend', () => {
    const memfs = new MemFsBackend(1024);
    const mountErr = fs.mount('/mnt', memfs);
    expect(mountErr).toBe(SsiErrorCode.OK);

    // Write to mounted backend
    fs.open('/mnt/remote.txt', O_CREAT | O_RDWR);
    fs.write(3, new TextEncoder().encode('remote').buffer);
    fs.close(3);

    // Can stat via mount path
    const statResult = fs.stat('/mnt/remote.txt');
    expect(statResult.error).toBe(SsiErrorCode.OK);
    expect(statResult.stat!.size).toBe(6);

    // Unmount
    const unmountErr = fs.unmount('/mnt');
    expect(unmountErr).toBe(SsiErrorCode.OK);

    // Mounted files gone
    expect(fs.stat('/mnt/remote.txt').error).toBe(SsiErrorCode.NOT_FOUND);
  });

  it('should route paths to the correct backend', () => {
    const memfsA = new MemFsBackend(1024);
    const memfsB = new MemFsBackend(1024);

    fs.mount('/a', memfsA);
    fs.mount('/b', memfsB);

    // Write to /a/hello.txt
    fs.open('/a/hello.txt', O_CREAT | O_RDWR);
    fs.write(3, new TextEncoder().encode('from A').buffer);
    fs.close(3);

    // Write to /b/hello.txt
    fs.open('/b/hello.txt', O_CREAT | O_RDWR);
    fs.write(3, new TextEncoder().encode('from B').buffer);
    fs.close(3);

    // Verify isolation
    const r1 = fs.stat('/a/hello.txt');
    expect(r1.error).toBe(SsiErrorCode.OK);

    const r2 = fs.stat('/b/hello.txt');
    expect(r2.error).toBe(SsiErrorCode.OK);
  });

  // ── createMemfs ──

  it('should create a named in-memory filesystem', () => {
    const err = fs.createMemfs('scratch', 512);
    expect(err).toBe(SsiErrorCode.OK);

    fs.open('/memfs/scratch/tmp.dat', O_CREAT | O_RDWR);
    fs.write(3, new TextEncoder().encode('scratch data').buffer);
    fs.close(3);

    const stat = fs.stat('/memfs/scratch/tmp.dat');
    expect(stat.error).toBe(SsiErrorCode.OK);
    expect(stat.stat!.size).toBe(12);
  });

  // ── OPFS/IDBFS (not supported in non-browser) ──

  it('should return NOT_SUPPORTED for OPFS', () => {
    expect(fs.mountOpfs('/opfs')).toBe(SsiErrorCode.NOT_SUPPORTED);
  });

  it('should return NOT_SUPPORTED for IDBFS', () => {
    expect(fs.mountIdbfs('/idbfs', 'test')).toBe(SsiErrorCode.NOT_SUPPORTED);
  });
});

// =========================================================================
// MEMFS Backend Unit Tests
// =========================================================================

describe('MemFsBackend', () => {
  let memfs: MemFsBackend;

  beforeEach(() => {
    memfs = new MemFsBackend(1024 * 1024);
    memfs.init({});
  });

  afterEach(() => {
    memfs.destroy();
  });

  it('should create files with O_CREAT', () => {
    const err = memfs.open('/test.txt', O_CREAT | O_RDWR);
    expect(err).toBe(SsiErrorCode.OK);
  });

  it('should read/write data', () => {
    memfs.open('/data.bin', O_CREAT | O_RDWR);
    const data = new Uint8Array([1, 2, 3, 4, 5]).buffer;
    memfs.write(3, data);
    memfs.seek(3, 0, SEEK_SET);
    const result = memfs.read(3, 5);
    expect(new Uint8Array(result.data!)).toEqual(new Uint8Array([1, 2, 3, 4, 5]));
  });

  it('should handle large files', () => {
    memfs.open('/large.bin', O_CREAT | O_RDWR);
    const chunk = new Uint8Array(64 * 1024); // 64KB
    for (let i = 0; i < 16; i++) { // 1MB total
      memfs.write(3, chunk.buffer);
    }
    const stat = memfs.stat('/large.bin');
    expect(stat.stat!.size).toBe(1024 * 1024);
  });

  it('should create and list directories', () => {
    memfs.mkdir('/a', 0o755);
    memfs.mkdir('/a/b', 0o755);
    memfs.open('/a/b/file.txt', O_CREAT | O_RDWR);

    const entries = memfs.readdir('/a/b');
    expect(entries.entries.length).toBe(3); // ., .., file.txt
    expect(entries.entries.some(e => e.name === 'file.txt')).toBe(true);
  });

  it('should fail stat for non-existent path', () => {
    const result = memfs.stat('/does_not_exist');
    expect(result.error).toBe(SsiErrorCode.NOT_FOUND);
  });

  it('should unlink files', () => {
    memfs.open('/del.txt', O_CREAT | O_RDWR);
    memfs.close(3);
    memfs.unlink('/del.txt');
    expect(memfs.stat('/del.txt').error).toBe(SsiErrorCode.NOT_FOUND);
  });

  it('should rename files', () => {
    memfs.open('/old.txt', O_CREAT | O_RDWR);
    memfs.write(3, new TextEncoder().encode('data').buffer);
    memfs.close(3);
    memfs.rename('/old.txt', '/new.txt');
    expect(memfs.stat('/old.txt').error).toBe(SsiErrorCode.NOT_FOUND);
    expect(memfs.stat('/new.txt').stat!.size).toBe(4);
  });

  it('should handle seek and tell', () => {
    memfs.open('/seek.bin', O_CREAT | O_RDWR);
    const data = new TextEncoder().encode('Hello World').buffer;
    memfs.write(3, data);

    const tell1 = memfs.tell(3);
    expect(tell1.position).toBe(11);

    memfs.seek(3, 0, SEEK_SET);
    const read1 = memfs.read(3, 5);
    expect(new TextDecoder().decode(read1.data!)).toBe('Hello');

    const tell2 = memfs.tell(3);
    expect(tell2.position).toBe(5);
  });
});

// Type declarations for File System Access API extensions
// that are missing from the standard TypeScript lib definitions.

interface FileSystemDirectoryHandle {
  /**
   * Returns an async iterator of [name, handle] pairs for all entries
   * in this directory. This is part of the File System Access API spec
   * but may be missing from older TypeScript lib definitions.
   */
  entries(): AsyncIterableIterator<[string, FileSystemHandle]>;
}

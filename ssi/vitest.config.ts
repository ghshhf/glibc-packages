import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    globals: true,
    environment: 'node',
    include: ['core/test/**/*.test.ts', 'bus/test/**/*.test.ts', 'ui/test/**/*.test.ts', 'db/test/**/*.test.ts', 'sec/test/**/*.test.ts', 'ai/test/**/*.test.ts', 'hal/test/**/*.test.ts', 'fs/test/**/*.test.ts', 'browser/test/**/*.test.ts', 'net/test/**/*.test.ts', 'packager/test/**/*.test.ts'],
    exclude: ['node_modules'],
  },
});

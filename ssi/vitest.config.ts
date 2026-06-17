import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    include: ['core/test/**/*.test.ts', 'bus/test/**/*.test.ts', 'ui/test/**/*.test.ts', 'db/test/**/*.test.ts', 'sec/test/**/*.test.ts', 'ai/test/**/*.test.ts', 'hal/test/**/*.test.ts'],
    exclude: ['node_modules'],
  },
});

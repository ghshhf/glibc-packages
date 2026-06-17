import { SwbnPackager } from './packager/src/index';
import { resolve } from 'path';

async function main() {
  const componentDir = resolve('./demo-component');
  const outputPath = resolve('../dist/demo-browser.swbn');
  
  await SwbnPackager.build(componentDir, outputPath);
  SwbnPackager.info(outputPath);
}

main().catch(console.error);

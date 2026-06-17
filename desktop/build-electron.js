#!/usr/bin/env node
/**
 * SkyNet Desktop Build Script
 * 
 * Uses electron-packager to build Windows .exe portable/installer
 * without the winCodeSign dependency that electron-builder requires.
 */

const packager = require('electron-packager');
const path = require('path');
const fs = require('fs');
const { execSync } = require('child_process');

const ROOT = __dirname;
const DIST = path.resolve(ROOT, '..', 'dist');
const APP_NAME = 'SkyNet-SSI-Runtime';
const VERSION = '1.0.0';

async function build() {
  console.log('[SkyNet Build] Starting Windows build...');
  console.log(`[SkyNet Build] Output: ${DIST}`);

  // Ensure dist directory exists
  if (!fs.existsSync(DIST)) {
    fs.mkdirSync(DIST, { recursive: true });
  }

  // Step 1: Package the app with electron-packager
  console.log('[SkyNet Build] Step 1: Packaging app...');
  
  const appPaths = await packager({
    dir: ROOT,
    name: APP_NAME,
    platform: 'win32',
    arch: 'x64',
    out: DIST,
    icon: path.join(ROOT, 'public', 'icon.png'),
    appVersion: VERSION,
    executableName: 'SkyNet',
    win32metadata: {
      CompanyName: 'SkyNet',
      FileDescription: 'SkyNet SSI Runtime Dashboard',
      OriginalFilename: 'SkyNet.exe',
      ProductName: 'SkyNet SSI Runtime',
      InternalName: 'SkyNet'
    },
    asar: true,
    overwrite: true,
    prune: true,
    ignore: [
      /node_modules\/electron/,
      /node_modules\/electron-builder/,
      /build-electron\.js/,
      /\.gitignore/,
      /package-lock\.json/
    ]
  });

  console.log('[SkyNet Build] App packaged successfully:');
  appPaths.forEach(p => console.log(`  - ${p}`));

  const appDir = appPaths[0];
  const appExe = path.join(appDir, `${APP_NAME}.exe`);

  if (!fs.existsSync(appExe)) {
    // Try alternative path
    const altExe = path.join(appDir, 'SkyNet.exe');
    if (fs.existsSync(altExe)) {
      console.log(`[SkyNet Build] Found executable: ${altExe}`);
    }
  }

  // Step 2: Create a ZIP distribution
  console.log('[SkyNet Build] Step 2: Creating ZIP archive...');
  const zipName = `${APP_NAME}-${VERSION}-win32-x64.zip`;
  const zipPath = path.join(DIST, zipName);

  // Use PowerShell to create the zip
  try {
    execSync(
      `powershell -Command "Compress-Archive -Path '${appDir}\\*' -DestinationPath '${zipPath}' -Force"`,
      { stdio: 'pipe', timeout: 120000 }
    );
    const zipSize = fs.statSync(zipPath).size;
    console.log(`[SkyNet Build] ZIP created: ${zipPath} (${(zipSize / 1024 / 1024).toFixed(1)} MB)`);
  } catch (zipErr) {
    console.error('[SkyNet Build] ZIP creation failed:', zipErr.message);
    console.log('[SkyNet Build] The unpacked app is still available at:', appDir);
  }

  // Step 3: Create NSIS installer if makensis is available
  console.log('[SkyNet Build] Step 3: Checking for NSIS installer...');
  try {
    execSync('makensis /VERSION', { stdio: 'pipe' });
    console.log('[SkyNet Build] NSIS found, creating installer...');
    // Future: create NSIS installer script
  } catch (e) {
    console.log('[SkyNet Build] NSIS not available. Installer skipped.');
    console.log('[SkyNet Build] The ZIP distribution can be used directly.');
  }

  console.log('[SkyNet Build] Build complete!');
  console.log(`[SkyNet Build] Output directory: ${DIST}`);
}

build().catch(err => {
  console.error('[SkyNet Build] Build failed:', err);
  process.exit(1);
});

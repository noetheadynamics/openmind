// OpenMind – bump version in package.json, src-tauri/Cargo.toml, src-tauri/tauri.conf.json
const fs = require('fs');
const path = require('path');

const newVersion = process.argv[2];
if (!/^\d+\.\d+\.\d+$/.test(newVersion || '')) {
    console.error('Usage: node scripts/bump_version.js <x.y.z>');
    process.exit(1);
}

const root = path.join(__dirname, '..');

// package.json
const pkgPath = path.join(root, 'package.json');
const pkg = JSON.parse(fs.readFileSync(pkgPath, 'utf8'));
pkg.version = newVersion;
fs.writeFileSync(pkgPath, JSON.stringify(pkg, null, 2) + '\n');
console.log('  package.json -> ' + newVersion);

// src-tauri/tauri.conf.json
const confPath = path.join(root, 'src-tauri', 'tauri.conf.json');
const conf = JSON.parse(fs.readFileSync(confPath, 'utf8'));
conf.version = newVersion;
fs.writeFileSync(confPath, JSON.stringify(conf, null, 2) + '\n');
console.log('  tauri.conf.json -> ' + newVersion);

// src-tauri/Cargo.toml
const cargoPath = path.join(root, 'src-tauri', 'Cargo.toml');
let cargo = fs.readFileSync(cargoPath, 'utf8');
cargo = cargo.replace(/^version = ".*"$/m, `version = "${newVersion}"`);
fs.writeFileSync(cargoPath, cargo);
console.log('  Cargo.toml -> ' + newVersion);

console.log('Version bumped to ' + newVersion);

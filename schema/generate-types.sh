#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
EDITOR_DIR="$SCRIPT_DIR/../editor-tx"

cd "$EDITOR_DIR"

node -e "
const { compileFromFile } = require('json-schema-to-typescript');
const fs = require('fs');
const path = require('path');

async function main() {
  const schemaPath = path.resolve('$SCRIPT_DIR', 't16-config.schema.json');
  const ts = await compileFromFile(schemaPath, {
    bannerComment: '// DO NOT EDIT — generated from schema/t16-config.schema.json',
    additionalProperties: false,
  });
  const outPath = path.resolve('$EDITOR_DIR', 'src/types/config.ts');
  fs.writeFileSync(outPath, ts);
  console.log('Generated:', outPath);
}
main().catch(e => { console.error(e); process.exit(1); });
"

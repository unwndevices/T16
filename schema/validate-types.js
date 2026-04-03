#!/usr/bin/env node
const fs = require('fs')
const path = require('path')

const schemaPath = path.join(__dirname, 't16-config.schema.json')
const typesPath = path.join(__dirname, '..', 'editor-tx', 'src', 'types', 'config.ts')

// Validate both files exist
if (!fs.existsSync(schemaPath)) {
    console.error('ERROR: Schema file not found:', schemaPath)
    process.exit(1)
}
if (!fs.existsSync(typesPath)) {
    console.error('ERROR: Types file not found:', typesPath)
    process.exit(1)
}

const schema = JSON.parse(fs.readFileSync(schemaPath, 'utf8'))
const types = fs.readFileSync(typesPath, 'utf8')

// Check that all required schema properties appear in the TypeScript types
const errors = []

// Check top-level properties
const topProps = Object.keys(schema.properties || {})
for (const prop of topProps) {
    if (!types.includes(prop)) {
        errors.push(`Missing top-level property in types: ${prop}`)
    }
}

// Check global properties
const globalProps = Object.keys(schema.properties?.global?.properties || {})
for (const prop of globalProps) {
    if (!types.includes(prop)) {
        errors.push(`Missing global property in types: ${prop}`)
    }
}

// Check bank properties
const bankProps = Object.keys(schema.properties?.banks?.items?.properties || {})
for (const prop of bankProps) {
    if (!types.includes(prop)) {
        errors.push(`Missing bank property in types: ${prop}`)
    }
}

if (errors.length > 0) {
    console.error('VALIDATION FAILED:')
    errors.forEach(e => console.error('  -', e))
    process.exit(1)
}

console.log(`PASS: All ${topProps.length + globalProps.length + bankProps.length} schema properties found in TypeScript types`)

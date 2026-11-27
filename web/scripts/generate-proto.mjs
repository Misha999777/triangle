import { spawnSync } from 'node:child_process'
import { mkdirSync, rmSync } from 'node:fs'
import { dirname, join } from 'node:path'
import { fileURLToPath } from 'node:url'

const root = join(dirname(fileURLToPath(import.meta.url)), '..')
const protoDir = join(root, '..', 'proto')
const outDir = join(root, 'src', 'gen')
const plugin = join(root, 'node_modules', '.bin', process.platform === 'win32' ? 'protoc-gen-es.cmd' : 'protoc-gen-es')

rmSync(outDir, { recursive: true, force: true })
mkdirSync(outDir, { recursive: true })

const result = spawnSync('protoc', [
  `--plugin=protoc-gen-es=${plugin}`,
  `--es_out=${outDir}`,
  '--es_opt=target=js+dts',
  `-I${protoDir}`,
  join(protoDir, 'triangle.proto'),
], { stdio: 'inherit' })

if (result.error?.code === 'ENOENT') {
  console.error('protoc not found')
  process.exit(1)
}
process.exit(result.status ?? 1)

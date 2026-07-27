"use strict";

// Read the Prisma panel view's JavaScript as one buffer, whatever it is split
// across. The Node counterpart of tools/prisma_view_source.py.
//
// The view used to hold a single inline <script>, and both Node harnesses
// sliced that block straight out of index.html. Once the script moved into
// scripts/*.js those slices found nothing -- loudly, in their case. The quieter
// hazard is the same one the Python loader exists for: a harness that searches
// a buffer the code has left will happily report that the code is absent.
//
// Load order comes from the <script src> tags themselves, which is the order
// the browser executes them in. A list maintained here could drift from what
// actually loads.

const fs = require("fs");
const path = require("path");

const VIEW_DIR = path.resolve(
  __dirname,
  "../Data/PrismaUI/views/CalamityAffixes"
);

// Deliberately narrow: only local relative references are inlined. An absolute
// or protocol URL is not part of the view's own source.
const SCRIPT_SRC_RE = /<script\b[^>]*\bsrc\s*=\s*["']([^"':]+?)["'][^>]*>\s*<\/script\s*>/gi;
const INLINE_SCRIPT_RE = /<script(?![^>]*\bsrc\b)[^>]*>([\s\S]*?)<\/script\s*>/gi;

function readIndex(viewDir) {
  return fs.readFileSync(path.join(viewDir, "index.html"), "utf8");
}

/** Referenced script paths, in document order -- which is execution order. */
function scriptPaths(viewDir = VIEW_DIR) {
  const source = readIndex(viewDir);
  const paths = [];
  for (const match of source.matchAll(SCRIPT_SRC_RE)) {
    paths.push(path.join(viewDir, match[1]));
  }
  return paths;
}

/** Every line of JS the view runs, in execution order. */
function loadScripts(viewDir = VIEW_DIR) {
  const source = readIndex(viewDir);
  const parts = [];
  for (const match of source.matchAll(INLINE_SCRIPT_RE)) {
    parts.push(match[1]);
  }
  for (const scriptPath of scriptPaths(viewDir)) {
    parts.push(fs.readFileSync(scriptPath, "utf8"));
  }
  return parts.filter((part) => part.length > 0).join("\n");
}

/** One named script file, for harnesses that exercise a single unit. */
function loadScript(relative, viewDir = VIEW_DIR) {
  return fs.readFileSync(path.join(viewDir, relative), "utf8");
}

module.exports = { VIEW_DIR, scriptPaths, loadScripts, loadScript };

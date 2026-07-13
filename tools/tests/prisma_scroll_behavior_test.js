"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");

const viewPath = path.resolve(
  __dirname,
  "../../Data/PrismaUI/views/CalamityAffixes/index.html"
);
const source = fs.readFileSync(viewPath, "utf8");
const start = source.indexOf("(function initFramePacedScroll() {");
const endMarker = "      })();";
const end = source.indexOf(endMarker, start);
assert(start >= 0 && end > start, "scroll controller not found");
const controller = source.slice(start, end + endMarker.length);

const handlers = new Map();
const rafQueue = new Map();
let nextRafId = 1;

class FakeElement {
  constructor(marked = true) {
    this.isConnected = true;
    this.clientHeight = 200;
    this.scrollHeight = 2000;
    this.parentElement = document.body;
    this.overflowY = "auto";
    this._scrollTop = 0;
    this.marked = marked;
  }

  get scrollTop() {
    return this._scrollTop;
  }

  set scrollTop(value) {
    const max = Math.max(0, this.scrollHeight - this.clientHeight);
    this._scrollTop = Math.round(
      Math.max(0, Math.min(Number(value) || 0, max))
    );
  }

  getAttribute(name) {
    if (name === "data-wheel-scroll-mode" && this.marked) {
      return "smooth";
    }
    return null;
  }
}

global.Element = FakeElement;
global.document = {
  body: {},
  addEventListener(type, callback) {
    handlers.set(type, callback);
  }
};
global.getComputedStyle = (element) => ({
  overflowY: element.overflowY
});
global.clamp = (value, minimum, maximum) =>
  Math.min(maximum, Math.max(minimum, value));
global.requestAnimationFrame = (callback) => {
  const id = nextRafId;
  nextRafId += 1;
  rafQueue.set(id, callback);
  return id;
};
global.cancelAnimationFrame = (id) => {
  rafQueue.delete(id);
};

eval(controller);

function runFrame(timestamp) {
  const entry = rafQueue.entries().next().value;
  assert(entry, "expected a queued animation frame");
  const [id, callback] = entry;
  rafQueue.delete(id);
  callback(timestamp);
}

function drainFrames(maxFrames = 60) {
  let frame = 0;
  while (rafQueue.size && frame < maxFrames) {
    frame += 1;
    runFrame(frame * 16.67);
  }
  assert.strictEqual(rafQueue.size, 0, "animation did not terminate");
  return frame;
}

function sendWheel(
  element,
  { deltaY, deltaMode = 0, wheelDelta = 0 }
) {
  let prevented = false;
  handlers.get("wheel")({
    target: element,
    deltaY,
    deltaMode,
    wheelDelta,
    preventDefault() {
      prevented = true;
    }
  });
  return prevented;
}

const discreteMouse = new FakeElement();
assert(
  sendWheel(discreteMouse, { deltaY: 3, wheelDelta: -120 })
);
assert(drainFrames() < 30);
assert.strictEqual(discreteMouse.scrollTop, 64);

const discreteUp = new FakeElement();
discreteUp.scrollTop = 500;
assert(
  sendWheel(discreteUp, { deltaY: -3, wheelDelta: 120 })
);
drainFrames();
assert.strictEqual(discreteUp.scrollTop, 436);

const legacyDown = new FakeElement();
legacyDown.scrollTop = 500;
assert(
  sendWheel(legacyDown, { deltaY: 0, wheelDelta: -120 })
);
drainFrames();
assert.strictEqual(legacyDown.scrollTop, 564);

const legacyUp = new FakeElement();
legacyUp.scrollTop = 500;
assert(
  sendWheel(legacyUp, { deltaY: 0, wheelDelta: 120 })
);
drainFrames();
assert.strictEqual(legacyUp.scrollTop, 436);

const multiNotch = new FakeElement();
multiNotch.scrollTop = 500;
assert(
  sendWheel(multiNotch, { deltaY: 0, wheelDelta: -360 })
);
drainFrames();
assert.strictEqual(multiNotch.scrollTop, 660);

const lineMode = new FakeElement();
assert(sendWheel(lineMode, { deltaY: 3, deltaMode: 1 }));
drainFrames();
assert.strictEqual(lineMode.scrollTop, 84);

const pageMode = new FakeElement();
assert(sendWheel(pageMode, { deltaY: 1, deltaMode: 2 }));
drainFrames();
assert.strictEqual(pageMode.scrollTop, 160);

const trackpad = new FakeElement();
assert(sendWheel(trackpad, { deltaY: 3, wheelDelta: -3 }));
drainFrames();
assert.strictEqual(trackpad.scrollTop, 3);

const ownWrite = new FakeElement();
assert(sendWheel(ownWrite, { deltaY: 100, wheelDelta: -120 }));
runFrame(16.67);
assert(rafQueue.size > 0);
handlers.get("scroll")({ target: ownWrite });
assert(
  rafQueue.size > 0,
  "own RAF write was mistaken for native scroll"
);
drainFrames();

const delayedThumb = new FakeElement();
assert(
  sendWheel(delayedThumb, { deltaY: 100, wheelDelta: -120 })
);
runFrame(16.67);
handlers.get("scroll")({ target: delayedThumb });
assert(rafQueue.size > 0);
delayedThumb.scrollTop = 400;
runFrame(33.34);
assert.strictEqual(
  rafQueue.size,
  0,
  "next RAF did not detect delayed native thumb scroll"
);
assert.strictEqual(
  delayedThumb.scrollTop,
  400,
  "next RAF overwrote delayed native thumb position"
);

const thumb = new FakeElement();
assert(sendWheel(thumb, { deltaY: 100, wheelDelta: -120 }));
runFrame(16.67);
assert(rafQueue.size > 0);
thumb.scrollTop = 400;
handlers.get("scroll")({ target: thumb });
assert.strictEqual(
  rafQueue.size,
  0,
  "native thumb scroll did not cancel RAF"
);
assert.strictEqual(thumb.scrollTop, 400);

const pointer = new FakeElement();
assert(sendWheel(pointer, { deltaY: 100, wheelDelta: -120 }));
assert(rafQueue.size > 0);
handlers.get("pointerdown")({ target: pointer });
assert.strictEqual(
  rafQueue.size,
  0,
  "pointerdown did not cancel RAF"
);

const mouse = new FakeElement();
assert(sendWheel(mouse, { deltaY: 100, wheelDelta: -120 }));
assert(rafQueue.size > 0);
handlers.get("mousedown")({ target: mouse });
assert.strictEqual(
  rafQueue.size,
  0,
  "mousedown did not cancel RAF"
);

const unmarked = new FakeElement(false);
assert.strictEqual(
  sendWheel(unmarked, { deltaY: 100, wheelDelta: -120 }),
  false
);
assert.strictEqual(
  rafQueue.size,
  0,
  "unmarked scroller was intercepted"
);

console.log("Prisma scroll behavior: OK");

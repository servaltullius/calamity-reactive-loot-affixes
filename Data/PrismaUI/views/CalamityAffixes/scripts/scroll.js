      // Frame-paced wheel scrolling for PrismaUI's CEF view.
      (function initFramePacedScroll() {
        const SCROLL_LINE_HEIGHT_PX = 28;
        const LEGACY_WHEEL_NOTCH_DELTA = 120;
        const MOUSE_WHEEL_NOTCH_PX = 64;
        const MAX_WHEEL_DELTA_PX = 160;
        const SMOOTH_SCROLL_TIME_CONSTANT_MS = 48;
        const SCROLL_STOP_DISTANCE_PX = 0.75;
        const SCROLL_MIN_EFFECTIVE_STEP_PX = 0.5;
        const SCROLL_WRITE_TOLERANCE_PX = 1;
        const state = new WeakMap();
        const scrollParentCache = new WeakMap();
        const reducedMotionQuery = typeof window.matchMedia === "function"
          ? window.matchMedia("(prefers-reduced-motion: reduce)")
          : null;

        function shouldReduceMotion() {
          return Boolean(reducedMotionQuery?.matches);
        }

        function getScrollState(element) {
          if (!state.has(element)) {
            state.set(element, {
              targetScrollTop: element.scrollTop,
              raf: 0,
              lastTimestamp: 0,
              lastProgrammaticScrollTop: null
            });
          }
          return state.get(element);
        }

        function cancelScrollAnimation(element) {
          const scrollState = state.get(element);
          if (!scrollState) return;

          if (scrollState.raf) {
            cancelAnimationFrame(scrollState.raf);
          }
          scrollState.targetScrollTop = element.scrollTop;
          scrollState.raf = 0;
          scrollState.lastTimestamp = 0;
          scrollState.lastProgrammaticScrollTop = null;
        }

        function isScrollable(element) {
          return Boolean(
            element &&
            element.isConnected &&
            element.clientHeight > 0 &&
            element.scrollHeight > element.clientHeight
          );
        }

        function isControlledScroller(element) {
          return (
            element?.getAttribute("data-wheel-scroll-mode") === "smooth"
          );
        }

        function findScrollParent(target) {
          if (!(target instanceof Element)) return null;

          const cached = scrollParentCache.get(target);
          if (isControlledScroller(cached) && isScrollable(cached)) {
            return cached;
          }

          let element = target;
          while (element && element !== document.body) {
            const overflowY = getComputedStyle(element).overflowY;
            if (
              isControlledScroller(element) &&
              (overflowY === "auto" || overflowY === "scroll") &&
              isScrollable(element)
            ) {
              scrollParentCache.set(target, element);
              return element;
            }
            element = element.parentElement;
          }
          return null;
        }

        function normalizeWheelDeltaPixels(event, scroller) {
          let deltaPixels = Number(event.deltaY) || 0;
          if (event.deltaMode === 1) {
            deltaPixels *= SCROLL_LINE_HEIGHT_PX;
          } else if (event.deltaMode === 2) {
            deltaPixels *= Math.max(
              scroller.clientHeight * 0.9,
              SCROLL_LINE_HEIGHT_PX * 8
            );
          } else {
            const legacyWheelDelta = Number(event.wheelDelta);
            if (
              Number.isFinite(legacyWheelDelta) &&
              Math.abs(legacyWheelDelta) >= LEGACY_WHEEL_NOTCH_DELTA
            ) {
              const notchCount = Math.max(
                1,
                Math.round(
                  Math.abs(legacyWheelDelta) / LEGACY_WHEEL_NOTCH_DELTA
                )
              );
              const notchPixels =
                Math.sign(deltaPixels || -legacyWheelDelta) *
                MOUSE_WHEEL_NOTCH_PX *
                notchCount;
              if (Math.abs(deltaPixels) < Math.abs(notchPixels)) {
                deltaPixels = notchPixels;
              }
            }
          }
          return clamp(
            deltaPixels,
            -MAX_WHEEL_DELTA_PX,
            MAX_WHEEL_DELTA_PX
          );
        }

        function animateScroll(element, timestamp) {
          const scrollState = getScrollState(element);
          if (!isScrollable(element)) {
            scrollState.targetScrollTop = element?.scrollTop || 0;
            scrollState.raf = 0;
            scrollState.lastTimestamp = 0;
            scrollState.lastProgrammaticScrollTop = null;
            return;
          }

          const expectedScrollTop =
            scrollState.lastProgrammaticScrollTop;
          if (
            expectedScrollTop !== null &&
            Math.abs(element.scrollTop - expectedScrollTop) >
              SCROLL_WRITE_TOLERANCE_PX
          ) {
            cancelScrollAnimation(element);
            return;
          }

          const maxScrollTop = Math.max(
            0,
            element.scrollHeight - element.clientHeight
          );
          scrollState.targetScrollTop = clamp(
            scrollState.targetScrollTop,
            0,
            maxScrollTop
          );
          const deltaTime = scrollState.lastTimestamp
            ? Math.min(40, Math.max(1, timestamp - scrollState.lastTimestamp))
            : 16.67;
          scrollState.lastTimestamp = timestamp;

          const current = element.scrollTop;
          const remaining = scrollState.targetScrollTop - current;
          if (Math.abs(remaining) <= SCROLL_STOP_DISTANCE_PX) {
            scrollState.lastProgrammaticScrollTop =
              scrollState.targetScrollTop;
            element.scrollTop = scrollState.targetScrollTop;
            scrollState.raf = 0;
            scrollState.lastTimestamp = 0;
            return;
          }

          const blend = 1 - Math.exp(
            -deltaTime / SMOOTH_SCROLL_TIME_CONSTANT_MS
          );
          const nextScrollTop = current + remaining * blend;
          if (
            Math.abs(nextScrollTop - current) <
            SCROLL_MIN_EFFECTIVE_STEP_PX
          ) {
            scrollState.lastProgrammaticScrollTop =
              scrollState.targetScrollTop;
            element.scrollTop = scrollState.targetScrollTop;
            scrollState.raf = 0;
            scrollState.lastTimestamp = 0;
            return;
          }

          scrollState.lastProgrammaticScrollTop = nextScrollTop;
          element.scrollTop = nextScrollTop;
          scrollState.raf = requestAnimationFrame((nextTimestamp) => {
            animateScroll(element, nextTimestamp);
          });
        }

        function cancelScrollForPointer(event) {
          const scroller = findScrollParent(event.target);
          if (scroller) {
            cancelScrollAnimation(scroller);
          }
        }

        function syncExternalScroll(event) {
          const scroller = event.target;
          if (!(scroller instanceof Element) || !state.has(scroller)) {
            return;
          }

          const scrollState = state.get(scroller);
          const actualScrollTop = scroller.scrollTop;
          if (
            scrollState.lastProgrammaticScrollTop !== null &&
            Math.abs(
              actualScrollTop - scrollState.lastProgrammaticScrollTop
            ) <= SCROLL_WRITE_TOLERANCE_PX
          ) {
            return;
          }

          cancelScrollAnimation(scroller);
        }

        document.addEventListener("pointerdown", cancelScrollForPointer, true);
        document.addEventListener("mousedown", cancelScrollForPointer, true);
        document.addEventListener("scroll", syncExternalScroll, true);

        document.addEventListener("wheel", (event) => {
          const scroller = findScrollParent(event.target);
          if (!scroller) return;

          const deltaPixels = normalizeWheelDeltaPixels(event, scroller);
          if (deltaPixels === 0) return;

          const scrollState = getScrollState(scroller);
          if (!scrollState.raf) {
            scrollState.targetScrollTop = scroller.scrollTop;
            scrollState.lastProgrammaticScrollTop = scroller.scrollTop;
          }
          const maxScrollTop = Math.max(
            0,
            scroller.scrollHeight - scroller.clientHeight
          );
          const previousTarget = scrollState.targetScrollTop;
          const nextTarget = clamp(
            scrollState.targetScrollTop + deltaPixels,
            0,
            maxScrollTop
          );
          const targetUnchanged =
            Math.abs(nextTarget - previousTarget) <= SCROLL_STOP_DISTANCE_PX;
          const currentAtTarget =
            Math.abs(scroller.scrollTop - previousTarget) <= SCROLL_STOP_DISTANCE_PX;
          if (targetUnchanged && currentAtTarget) {
            return;
          }

          event.preventDefault();
          scrollState.targetScrollTop = nextTarget;

          if (shouldReduceMotion()) {
            if (scrollState.raf) {
              cancelAnimationFrame(scrollState.raf);
            }
            scrollState.raf = 0;
            scrollState.lastTimestamp = 0;
            scrollState.lastProgrammaticScrollTop = nextTarget;
            scroller.scrollTop = nextTarget;
            return;
          }

          if (!scrollState.raf) {
            scrollState.lastTimestamp = 0;
            scrollState.raf = requestAnimationFrame((timestamp) => {
              animateScroll(scroller, timestamp);
            });
          }
        }, { passive: false, capture: true });
      })();

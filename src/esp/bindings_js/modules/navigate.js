// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/*global Module */

/**
 * NavigateTask class
 */
class NavigateTask {
  // PUBLIC methods.

  /**
   * Create navigate task.
   * @param {SimEnv} sim - simulator
   * @param {TopDownMap} topdown - TopDown Map
   * @param {Object} components - dictionary with status and canvas elements
   */
  constructor(sim, components) {
    this.sim = sim;
    this.components = components;
    this.topdown = components.topdown;
    this.semanticsEnabled = false;
    this.radarEnabled = false;

    if (this.components.semantic) {
      this.semanticsEnabled = true;
      this.semanticCtx = components.semantic.getContext("2d");
      this.semanticShape = this.sim.getObservationSpace("semantic").shape;
      this.semanticImageData = this.semanticCtx.createImageData(
        this.semanticShape.get(1),
        this.semanticShape.get(0)
      );
      this.semanticObservation = new Module.Observation();
      this.semanticObjects = this.sim.sim.getSemanticScene().objects;
      components.canvas.onmousedown = e => {
        this.handleMouseDown(e);
      };
    }

    if (this.components.radar) {
      this.radarEnabled = true;
      this.radarCtx = components.radar.getContext("2d");
    }

    this.actions = [
      { name: "moveForward", key: "w" },
      { name: "turnLeft", key: "a" },
      { name: "turnRight", key: "d" },
      { name: "lookUp", key: "ArrowUp" },
      { name: "lookDown", key: "ArrowDown" },
      { name: "done", key: " " }
    ];
  }

  handleMouseDown(event) {
    const height = this.semanticShape.get(0);
    const width = this.semanticShape.get(1);
    const offsetY = height - 1 - event.offsetY; /* flip-Y */
    const offsetX = event.offsetX;
    const objectId = new Uint32Array(
      this.semanticData.buffer,
      this.semanticData.byteOffset,
      this.semanticData.length / 4
    )[width * offsetY + offsetX];
    this.setStatus(this.semanticObjects.get(objectId).category.getName(""));
  }

  /**
   * Initialize the task. Should be called once.
   */
  init() {
    this.bindKeys();
  }

  /**
   * Reset the task.
   */
  reset() {
    this.sim.reset();
    this.setStatus("Ready");
    this.render();
  }

  // PRIVATE methods.

  setStatus(text) {
    this.components.status.innerHTML = text;
  }

  renderImage() {
    this.sim.displayObservation("rgb");
    this.renderRadar();
  }

  renderSemanticImage() {
    if (!this.semanticsEnabled || this.semanticObjects.size() == 0) {
      return;
    }

    this.sim.getObservation("semantic", this.semanticObservation);
    this.semanticData = this.semanticObservation.getData();
    const objectIds = new Uint32Array(
      this.semanticData.buffer,
      this.semanticData.byteOffset,
      this.semanticData.length / 4
    );

    // TOOD(msb) implement a better colorization scheme
    for (let i = 0; i < objectIds.length; i++) {
      const objectId = objectIds[i];
      if (objectId & 1) {
        this.semanticImageData.data[i * 4] = 255;
      } else {
        this.semanticImageData.data[i * 4] = 0;
      }
      if (objectId & 2) {
        this.semanticImageData.data[i * 4 + 1] = 255;
      } else {
        this.semanticImageData.data[i * 4 + 1] = 0;
      }
      if (objectId & 4) {
        this.semanticImageData.data[i * 4 + 2] = 255;
      } else {
        this.semanticImageData.data[i * 4 + 2] = 0;
      }
      this.semanticImageData.data[i * 4 + 3] = 255;
    }

    this.semanticCtx.putImageData(this.semanticImageData, 0, 0);
  }

  renderTopDown(options) {
    if (options.renderTopDown && this.topdown !== null) {
      this.topdown.moveTo(this.sim.getAgentState().position, 500);
    }
  }

  renderRadar() {
    if (!this.radarEnabled) {
      return;
    }
    const width = 100,
      height = 100;
    let radius = width / 2;
    let centerX = width / 2;
    let centerY = height / 2;
    let ctx = this.radarCtx;
    ctx.clearRect(0, 0, width, height);
    ctx.globalAlpha = 0.5;
    // draw circle
    ctx.fillStyle = "darkslategray";
    ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
    ctx.fill();
    // draw sector
    ctx.fillStyle = "darkgray";
    ctx.beginPath();
    // TODO(msb) Currently 90 degress but should really use fov.
    ctx.arc(centerX, centerY, radius, (-Math.PI * 3) / 4, -Math.PI / 4);
    ctx.lineTo(centerX, centerY);
    ctx.closePath();
    ctx.fill();
    // draw target
    ctx.globalAlpha = 1.0;
    ctx.beginPath();
    let magnitude, angle;
    [magnitude, angle] = this.sim.distanceToGoal();
    let normalized = magnitude / (magnitude + 1);
    let targetX = centerX + Math.sin(angle) * radius * normalized;
    let targetY = centerY - Math.cos(angle) * radius * normalized;
    ctx.fillStyle = "maroon";
    ctx.arc(targetX, targetY, 3, 0, 2 * Math.PI);
    ctx.fill();
  }

  render(options = { renderTopDown: true }) {
    this.renderImage();
    this.renderSemanticImage();
    this.renderTopDown(options);
  }

  handleAction(action) {
    this.sim.step(action);
    this.setStatus(action);
    this.render();
  }

  bindKeys() {
    document.addEventListener("keydown", event => {
      for (let a of this.actions) {
        if (event.key === a.key) {
          this.handleAction(a.name);
          event.preventDefault();
          break;
        }
      }
    });
  }
}

export default NavigateTask;

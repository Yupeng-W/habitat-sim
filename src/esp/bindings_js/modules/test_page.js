// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/*global Module */

import { defaultScene, defaultPhysicsConfigFilepath } from "./defaults";

// Our CI will try to load test_page.html in a browser, including instancing
// this class. See load_test_page.test.js. When adding tests here, you
// should use extensive logging to enable test troubleshooting directly from
// CI logs.
class TestPage {
  constructor() {}

  expect(condition, conditionAsStr) {
    if (!condition) {
      throw "expect failed: " + conditionAsStr;
    }
  }

  preRun(preloadFunc) {
    console.log("TestPage.preRun");

    this.scene = preloadFunc(defaultScene);
    this.physicsConfigFile = preloadFunc(defaultPhysicsConfigFilepath);

    const fileNoExtension = defaultScene.substr(
      0,
      defaultScene.lastIndexOf(".")
    );
    preloadFunc(fileNoExtension + ".navmesh");

    preloadFunc("data/objects/example_objects/cheezit.glb");
    this.objHandle = preloadFunc(
      "data/objects/example_objects/cheezit.object_config.json"
    );

    preloadFunc("data/objects/example_objects/banana.glb");
    this.bananaHandle = preloadFunc(
      "data/objects/example_objects/banana.object_config.json"
    );

    console.log("TestPage.preRun finished");
  }

  onRuntimeInitialized() {
    console.log("TestPage.onRuntimeInitialized");
    let config = new Module.SimulatorConfiguration();
    config.scene_id = this.scene;
    config.enablePhysics = true;
    config.physicsConfigFile = this.physicsConfigFile;

    console.log("new Module.Simulator");
    let sim = new Module.Simulator(config);

    console.log("loadAllObjectConfigsFromPath");
    Module.loadAllObjectConfigsFromPath(sim, "/data/objects/example_objects");

    console.log("sim.addObjectByHandle");
    let objId = sim.addObjectByHandle(this.objHandle, null, "", 0);
    this.expect(objId != -1, "objId != -1");

    console.log("sim.setTranslation");
    let dropPos = new Module.Vector3(-3.4, 1.8, 15.3);
    sim.setTranslation(dropPos, objId, 0);

    console.log("sim.getTranslation");
    let trans = sim.getTranslation(objId, 0);
    this.expect(trans.x() === dropPos.x(), "trans.x() === dropPos.x()");

    console.log("sim.stepWorld");
    sim.stepWorld(0.1);

    // if Bullet build, expect object to drop due to gravity
    console.log("Module.isBuildWithBulletPhysics");
    if (Module.isBuildWithBulletPhysics()) {
      trans = sim.getTranslation(objId, 0);
      this.expect(trans.y() < dropPos.y(), "trans.y() < dropPos.y()");
    }

    console.log("Testing raycast");
    // add an object
    let banana1id = sim.addObjectByHandle(this.bananaHandle, null, "", 0);
    sim.setTranslation(new Module.Vector3(0, 0.2, 9), banana1id, 0);
    // point a ray at the object
    let origin = new Module.Vector3(2, 0.2, 9);
    let direction = new Module.Vector3(-1, 0, 0);
    // check that castRay returns the object
    this.expect(Module.castRay(sim, origin, direction, 10) == banana1id);
    // check that castRay doesn't find the object if maxDistance is too small
    this.expect(Module.castRay(sim, origin, direction, 1) == -1);
    // add an object behind the first one
    let banana2id = sim.addObjectByHandle(this.bananaHandle, null, "", 0);
    sim.setTranslation(new Module.Vector3(-2, 0.2, 9), banana2id, 0);
    // check that castRay returns closest object
    let origin2 = new Module.Vector3(-4, 0.2, 9);
    let direction2 = new Module.Vector3(1, 0, 0);
    this.expect(Module.castRay(sim, origin, direction, 10) == banana1id);
    this.expect(Module.castRay(sim, origin2, direction2, 10) == banana2id);

    console.log("The test page has loaded successfully.");
    window.didTestPageLoad = true;
  }
}

export default TestPage;

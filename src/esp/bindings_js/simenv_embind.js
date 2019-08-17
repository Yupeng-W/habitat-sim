class SimEnv {
  // Uses embind simulator to act
  constructor(config, agentId) {
    this.sim = new Module.Simulator(config);;
    this.defaultAgentId = agentId;
  }

  reset() {
    this.sim.reset();
  }

  step(action) {
    const agent = this.sim.getAgent(this.defaultAgentId);
    agent.act(action);
  }

  createSensorSpec(config) {
    const converted = new Module.SensorSpec();
    for (let key in config) {
      let value = config[key];
      converted[key] = value;
    }
    return converted;
  }

  createAgentConfig(config) {
    const converted = new Module.AgentConfiguration();
    for (let key in config) {
      let value = config[key];
      if (key === 'sensorSpecifications') {
	const sensorSpecs = new Module.VectorSensorSpec();
	for (let c of value) {
	  sensorSpecs.push_back(this.createSensorSpec(c));
	}
	value = sensorSpecs;
      }
      converted[key] = value;
    }
    return converted;
  }

  addAgent(config) {
    this.sim.addAgent(this.createAgentConfig(config))
  }

  getObservationSpace(sensorId) {
    return this.sim.getAgentObservationSpace(this.defaultAgentId, sensorId);
  }

  getObservation(sensorId, buffer) {
    const obs = new Module.Observation();
    this.sim.getAgentObservation(0, sensorId, obs);
    return obs
  }

}

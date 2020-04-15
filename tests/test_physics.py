import math
import os.path as osp
import random

import magnum as mn
import numpy as np
import pytest
import quaternion

import examples.settings
import habitat_sim.physics
from habitat_sim.utils.common import (
    quat_from_angle_axis,
    quat_from_magnum,
    quat_to_magnum,
)


@pytest.mark.skipif(
    not osp.exists("data/scene_datasets/habitat-test-scenes/skokloster-castle.glb")
    or not osp.exists("data/objects/"),
    reason="Requires the habitat-test-scenes and habitat test objects",
)
def test_kinematics(sim):
    cfg_settings = examples.settings.default_sim_settings.copy()

    cfg_settings[
        "scene"
    ] = "data/scene_datasets/habitat-test-scenes/skokloster-castle.glb"
    # enable the physics simulator: also clears available actions to no-op
    cfg_settings["enable_physics"] = True
    cfg_settings["depth_sensor"] = True

    # test loading the physical scene
    hab_cfg = examples.settings.make_cfg(cfg_settings)
    sim.reconfigure(hab_cfg)
    assert sim.get_physics_object_library_size() > 0

    # test adding an object to the world
    object_id = sim.add_object(0)
    assert len(sim.get_existing_object_ids()) > 0

    # test setting the motion type

    assert sim.set_object_motion_type(habitat_sim.physics.MotionType.STATIC, object_id)
    assert (
        sim.get_object_motion_type(object_id) == habitat_sim.physics.MotionType.STATIC
    )
    assert sim.set_object_motion_type(
        habitat_sim.physics.MotionType.KINEMATIC, object_id
    )
    assert (
        sim.get_object_motion_type(object_id)
        == habitat_sim.physics.MotionType.KINEMATIC
    )

    # test kinematics
    I = np.identity(4)

    # test get and set translation
    sim.set_translation(np.array([0, 1.0, 0]), object_id)
    assert np.allclose(sim.get_translation(object_id), np.array([0, 1.0, 0]))

    # test get and set transform
    sim.set_transformation(I, object_id)
    assert np.allclose(sim.get_transformation(object_id), I)

    # test get and set rotation
    Q = quat_from_angle_axis(np.pi, np.array([0, 1.0, 0]))
    expected = np.eye(4)
    expected[0:3, 0:3] = quaternion.as_rotation_matrix(Q)
    sim.set_rotation(quat_to_magnum(Q), object_id)
    assert np.allclose(sim.get_transformation(object_id), expected)
    assert np.allclose(quat_from_magnum(sim.get_rotation(object_id)), Q)

    # test object removal
    old_object_id = sim.remove_object(object_id)
    assert len(sim.get_existing_object_ids()) == 0

    object_id = sim.add_object(0)

    prev_time = 0.0
    for _ in range(2):
        # do some kinematics here (todo: translating or rotating instead of absolute)
        sim.set_translation(np.random.rand(3), object_id)
        T = sim.get_transformation(object_id)

        # test getting observation
        obs = sim.step(random.choice(list(hab_cfg.agents[0].action_space.keys())))

        # check that time is increasing in the world
        assert sim.get_world_time() > prev_time
        prev_time = sim.get_world_time()

    sim.remove_object(object_id)

    # test attaching/dettaching an Agent to/from physics simulation
    agent_node = sim.agents[0].scene_node
    sim.add_object(0, agent_node)
    sim.set_translation(np.random.rand(3), object_id)
    assert np.allclose(agent_node.translation, sim.get_translation(object_id))
    sim.remove_object(object_id, False)  # don't delete the agent's node
    assert agent_node.translation


@pytest.mark.skipif(
    not osp.exists("data/scene_datasets/habitat-test-scenes/skokloster-castle.glb")
    or not osp.exists("data/objects/"),
    reason="Requires the habitat-test-scenes and habitat test objects",
)
def test_dynamics(sim):
    # This test assumes that default.phys_scene_config.json contains "physics simulator": "bullet".
    # TODO: enable dynamic override of this setting in simulation config structure

    cfg_settings = examples.settings.default_sim_settings.copy()

    cfg_settings[
        "scene"
    ] = "data/scene_datasets/habitat-test-scenes/skokloster-castle.glb"
    # enable the physics simulator: also clears available actions to no-op
    cfg_settings["enable_physics"] = True
    cfg_settings["depth_sensor"] = True

    # test loading the physical scene
    hab_cfg = examples.settings.make_cfg(cfg_settings)
    sim.reconfigure(hab_cfg)
    # make the simulation deterministic (C++ seed is set in reconfigure)
    np.random.seed(cfg_settings["seed"])
    assert sim.get_physics_object_library_size() > 0

    # test adding an object to the world
    object_id = sim.add_object(0)
    object2_id = sim.add_object(0)
    assert len(sim.get_existing_object_ids()) > 0

    # place the objects over the table in room
    sim.set_translation(np.array([-0.569043, 2.04804, 13.6156]), object_id)
    sim.set_translation(np.array([-0.569043, 2.04804, 12.6156]), object2_id)

    # get object MotionType and continue testing if MotionType::DYNAMIC (implies a physics implementation is active)
    if sim.get_object_motion_type(object_id) == habitat_sim.physics.MotionType.DYNAMIC:
        previous_object_states = [
            [sim.get_translation(object_id), sim.get_rotation(object_id)],
            [sim.get_translation(object2_id), sim.get_rotation(object2_id)],
        ]
        prev_time = sim.get_world_time()
        for _ in range(50):
            # force application at a location other than the origin should always cause angular and linear motion
            sim.apply_force(np.random.rand(3), np.random.rand(3), object2_id)

            # TODO: expose object properties (such as mass) to python
            # Counter the force of gravity on the object (it should not translate)
            obj_mass = 0.5
            sim.apply_force(np.array([0, obj_mass * 9.8, 0]), np.zeros(3), object_id)

            # apply torque to the "floating" object. It should rotate, but not translate
            sim.apply_torque(np.random.rand(3), object_id)

            # TODO: test other physics functions

            # test getting observation
            obs = sim.step(random.choice(list(hab_cfg.agents[0].action_space.keys())))

            # check that time is increasing in the world
            assert sim.get_world_time() > prev_time
            prev_time = sim.get_world_time()

            # check the object states
            # 1st object should rotate, but not translate
            assert np.allclose(
                previous_object_states[0][0], sim.get_translation(object_id)
            )
            assert previous_object_states[0][1] != sim.get_rotation(object_id)

            # 2nd object should rotate and translate
            assert not np.allclose(
                previous_object_states[1][0], sim.get_translation(object2_id)
            )
            assert previous_object_states[1][1] != sim.get_rotation(object2_id)

            previous_object_states = [
                [sim.get_translation(object_id), sim.get_rotation(object_id)],
                [sim.get_translation(object2_id), sim.get_rotation(object2_id)],
            ]

        # test setting DYNAMIC object to KINEMATIC
        assert sim.set_object_motion_type(
            habitat_sim.physics.MotionType.KINEMATIC, object2_id
        )
        assert (
            sim.get_object_motion_type(object2_id)
            == habitat_sim.physics.MotionType.KINEMATIC
        )

        obs = sim.step(random.choice(list(hab_cfg.agents[0].action_space.keys())))

        # 2nd object should no longer rotate or translate
        assert np.allclose(
            previous_object_states[1][0], sim.get_translation(object2_id)
        )
        assert previous_object_states[1][1] == sim.get_rotation(object2_id)

        sim.step_physics(0.1)

        # test velocity get/set
        test_lin_vel = np.array([1.0, 0.0, 0.0])
        test_ang_vel = np.array([0.0, 1.0, 0.0])

        # no velocity setting for KINEMATIC objects
        sim.set_linear_velocity(test_lin_vel, object2_id)
        assert sim.get_linear_velocity(object2_id) == np.array([0.0, 0.0, 0.0])

        sim.set_object_motion_type(habitat_sim.physics.MotionType.DYNAMIC, object2_id)
        sim.set_linear_velocity(test_lin_vel, object2_id)
        assert sim.get_linear_velocity(object2_id) == test_lin_vel
        sim.set_angular_velocity(test_ang_vel, object2_id)
        assert sim.get_angular_velocity(object2_id) == test_ang_vel

        # test modifying gravity
        new_object_start = np.array([100.0, 0, 0])
        sim.set_translation(new_object_start, object_id)
        new_grav = np.array([10.0, 0, 0])
        sim.set_gravity(new_grav)
        assert np.allclose(sim.get_gravity(), new_grav)
        assert np.allclose(sim.get_translation(object_id), new_object_start)
        sim.step_physics(0.1)
        assert sim.get_translation(object_id)[0] > new_object_start[0]


def test_velocity_control(sim):
    cfg_settings = examples.settings.default_sim_settings.copy()
    cfg_settings["scene"] = "NONE"
    cfg_settings["enable_physics"] = True
    hab_cfg = examples.settings.make_cfg(cfg_settings)
    sim.reconfigure(hab_cfg)
    sim.set_gravity(np.array([0, 0, 0.0]))

    template_path = osp.abspath("data/test_assets/objects/nested_box")
    template_ids = sim.load_object_configs(template_path)
    object_template = sim.get_object_template(template_ids[0])
    object_template.set_linear_damping(0.0)
    object_template.set_angular_damping(0.0)

    for iteration in range(2):
        sim.reset()
        object_id = sim.add_object(template_ids[0])
        vel_control = sim.get_object_velocity_control(object_id)

        if iteration == 0:
            if (
                sim.get_object_motion_type(object_id)
                != habitat_sim.physics.MotionType.DYNAMIC
            ):
                # Non-dynamic simulator in use. Skip 1st pass.
                sim.remove_object(object_id)
                continue
        elif iteration == 1:
            # test KINEMATIC
            sim.set_object_motion_type(
                habitat_sim.physics.MotionType.KINEMATIC, object_id
            )

        # test global velocities
        vel_control.linear_velocity = np.array([1.0, 0, 0])
        vel_control.angular_velocity = np.array([0, 1.0, 0])
        vel_control.controlling_lin_vel = True
        vel_control.controlling_ang_vel = True

        while sim.get_world_time() < 1.0:
            # NOTE: stepping close to default timestep to get near-constant velocity control of DYNAMIC bodies.
            sim.step_physics(0.00416)

        ground_truth_pos = sim.get_world_time() * vel_control.linear_velocity
        assert np.allclose(sim.get_translation(object_id), ground_truth_pos, atol=0.01)
        ground_truth_q = mn.Quaternion([[0, 0.480551, 0], 0.876967])
        angle_error = mn.math.angle(ground_truth_q, sim.get_rotation(object_id))
        assert angle_error < mn.Rad(0.005)

        sim.reset()

        # test local velocities (turn in a half circle)
        vel_control.lin_vel_is_local = True
        vel_control.ang_vel_is_local = True
        vel_control.linear_velocity = np.array([0, 0, -math.pi])
        vel_control.angular_velocity = np.array([math.pi * 2.0, 0, 0])

        sim.set_translation(np.array([0, 0, 0.0]), object_id)
        sim.set_rotation(mn.Quaternion(), object_id)

        while sim.get_world_time() < 0.5:
            # NOTE: stepping close to default timestep to get near-constant velocity control of DYNAMIC bodies.
            sim.step_physics(0.00416)

        # NOTE: explicit integration, so expect some error
        ground_truth_q = mn.Quaternion([[1.0, 0, 0], 0])
        assert np.allclose(
            sim.get_translation(object_id), np.array([0, 1.0, 0.0]), atol=0.03
        )
        angle_error = mn.math.angle(ground_truth_q, sim.get_rotation(object_id))
        assert angle_error < mn.Rad(0.05)

        sim.remove_object(object_id)

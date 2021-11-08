# Copyright (c) Facebook, Inc. and its affiliates.
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

import json
import os
import time
from enum import Enum
from typing import Any, Dict, List, Optional, Tuple

import magnum as mn
from fairmotion.core import motion
from fairmotion.data import amass
from fairmotion.ops import conversions

import habitat_sim.physics as phy
from habitat_sim.logging import LoggingContext, logger

#### Constants
ROOT = 0
METADATA_DEFAULT_WHEN_MISSING_FILE = {
    "urdf_path": "data/test_assets/urdf/amass_male.urdf",
    "amass_path": "data/fairmotion/amass_test_data/CMU/CMU/02/02_01_poses.npz",
    "bm_path": "data/fairmotion/amass_test_data/smplh/male/model.npz",
    "rotation": mn.Quaternion.rotation(mn.Deg(-90), mn.Vector3.x_axis())
    * mn.Quaternion.rotation(mn.Deg(90), mn.Vector3.z_axis()),
    "translation": mn.Vector3([2.5, 0.07, 0.7]),
}
METADATA_DIR = "data/fairmotion/"


class FairmotionInterface:
    def __init__(
        self,
        sim,
        urdf_path=None,
        amass_path=None,
        bm_path=None,
        metadata_file=None,
    ) -> None:

        LoggingContext.reinitialize_from_env()
        self.sim = sim
        self.art_obj_mgr = self.sim.get_articulated_object_manager()
        self.rgd_obj_mgr = self.sim.get_rigid_object_manager()
        self.model: Optional[phy.ManagedArticulatedObject] = None
        self.motion: Optional[motion.Motion] = None
        self.user_metadata = {}
        self.last_metadata_file: Optional[str] = None
        self.motion_stepper = 1
        self.rotation_offset: Optional[mn.Quaternion] = None
        self.translation_offset: Optional[mn.Vector3] = None

        self.key_frames = None
        self.key_frame_models = []
        self.preview_mode = Preview.OFF
        self.traj_ids: List[int] = []
        self.is_reversed = False
        self.setup_default_metadata()

        if metadata_file:
            try:
                self.fetch_metadata(metadata_file)
                self.set_data()
            except FileNotFoundError:
                logger.error(f"No file with path `{metadata_file}`, creating new file.")
                self.set_data(
                    urdf_path=urdf_path,
                    amass_path=amass_path,
                    bm_path=bm_path,
                )
                self.save_metadata(metadata_file)
                self.last_metadata_file = metadata_file

        else:
            # This sets the instance defaults with init(args)
            self.set_data(urdf_path=urdf_path, amass_path=amass_path, bm_path=bm_path)
            self.save_metadata("default")

        # positional offsets
        self.set_transform_offsets(
            rotate_offset=self.user_metadata["rotation"],
            translate_offset=self.user_metadata["translation"],
        )
        self.load_motion()

    def setup_default_metadata(self) -> None:
        """
        Called by init(), this method forces the default metadata into the metadata library.
        """
        # if folder doesn't exist, then create it.
        if not os.path.isdir(METADATA_DIR):
            os.makedirs(METADATA_DIR)

        # if default file isnt in metadata directory, create it from hardcoded metadata
        if not os.path.exists(METADATA_DIR + "default.json"):
            self.user_metadata = METADATA_DEFAULT_WHEN_MISSING_FILE
            self.save_metadata("default")

        # if default file exists, take it from there
        self.fetch_metadata("default")
        self.last_metadata_file: Optional[str] = None

    def set_data(
        self,
        urdf_path: Optional[str] = None,
        amass_path: Optional[str] = None,
        bm_path: Optional[str] = None,
        rotation: Optional[mn.Quaternion] = None,
        translation: Optional[mn.Vector3] = None,
    ) -> None:
        """
        A method that will take attributes of the model data sets as arguments, filling in
        empty args with the default set of data, and adds the data to the meta_data library.
        """
        data = self.user_metadata
        data["urdf_path"] = urdf_path or data["urdf_path"]
        data["amass_path"] = amass_path or data["amass_path"]
        data["bm_path"] = bm_path or data["bm_path"]
        data["rotation"] = rotation or data["rotation"]
        data["translation"] = translation or data["translation"]

    def metadata_parser(self, metadata_dict: Dict[str, Any], to_file: bool):
        """
        Convert metadata dict to and from a form that is json serializable
        """
        data = metadata_dict.copy()

        if to_file:
            for k, v in data.items():
                if isinstance(v, mn.Quaternion):
                    q_list = [list(v.vector)]
                    q_list.append(v.scalar)
                    data[k] = q_list
                elif isinstance(v, mn.Vector3):
                    data[k] = list(v)

        elif not to_file:
            for k, v in data.items():
                if k == "rotation":
                    data[k] = mn.Quaternion(v)
                elif k == "translation":
                    data[k] = mn.Vector3(v)
        return data

    def save_metadata(self, file: str):
        """
        Saves the current metadata to a json file in given file path
        """
        meta_access = [METADATA_DIR + file + ".json", METADATA_DIR + file, file]

        if file:
            # sanitizing file path
            for filename in meta_access:
                if os.path.exists(filename):
                    file = filename
                    break

                # we are no longer overwriting a file
                elif "/" not in file:
                    # file is not a file path, we need to aim it at our directory
                    if ".json" not in file:
                        # add file type
                        file = file + ".json"
                    file = METADATA_DIR + file
        else:
            # generate filename from timestamp
            file = METADATA_DIR + "user_" + time.strftime("%Y-%m-%d_%H-%M-%S")

        logger.info(f"Saving data to file: {file}")

        # updating user_metadata to reflect characters position
        if self.rotation_offset and self.translation_offset:
            data = self.user_metadata
            data["rotation"] = self.rotation_offset
            data["translation"] = self.translation_offset

        data = self.metadata_parser(self.user_metadata, to_file=True)

        with open(file, "w") as f:
            json.dump(data, f)
        logger.info(f"Saved: {file}")

    def fetch_metadata(self, file):
        """
        Fetch metadata from a json file in given file name and sets metadata
        """
        meta_access = [METADATA_DIR + file + ".json", METADATA_DIR + file, file]

        # sanitizing file path
        for filename in meta_access:
            if os.path.exists(filename):
                file = filename
                break

        with open(file, "r") as f:
            data = json.load(f)

        # set data to what was fetched
        self.user_metadata = self.metadata_parser(data, to_file=False)
        logger.info(f"Fetched: {file}")

        self.last_metadata_file = file

        # updating user_metadata to reflect characters position
        if self.rotation_offset and self.translation_offset:
            data = self.user_metadata
            self.rotation_offset = data["rotation"]
            self.translation_offset = data["translation"]

        self.hide_model()
        self.load_motion()

    def set_transform_offsets(
        self,
        rotate_offset: Optional[mn.Quaternion] = None,
        translate_offset: Optional[mn.Vector3] = None,
    ) -> None:
        """
        This method updates the offset of the model with the positional data passed to it.
        Use this for changing the location and orientation of the model.
        """
        self.rotation_offset = rotate_offset or self.rotation_offset
        self.translation_offset = translate_offset or self.translation_offset

        if self.traj_ids:
            # removes trajectory
            for t_id in self.traj_ids:
                self.sim.get_rigid_object_manager().remove_object_by_id(t_id)
                self.traj_ids = []
        self.next_pose(repeat=True)
        self.setup_key_frames()
        self.build_trajectory_vis()

    def load_motion(self) -> None:
        """
        Loads the motion currently set by metadata.
        """
        # loading text because the setup pauses here during motion load
        logger.info("Loading...")
        data = self.user_metadata
        self.motion = amass.load(file=data["amass_path"], bm_path=data["bm_path"])
        self.set_transform_offsets(
            rotate_offset=data["rotation"], translate_offset=data["translation"]
        )

    def load_model(self) -> None:
        """
        Loads the model currently set by metadata.
        """
        # loading text because the setup pauses here during motion load
        logger.info("Loading...")
        self.hide_model()

        # keeps the model up to date with current data target
        data = self.user_metadata

        # add an ArticulatedObject to the world with a fixed base
        self.model = self.art_obj_mgr.add_articulated_object_from_urdf(
            filepath=data["urdf_path"], fixed_base=True
        )
        assert self.model.is_alive

        # change motion_type to KINEMATIC
        self.model.motion_type = phy.MotionType.KINEMATIC

        self.model.translation = self.translation_offset
        self.next_pose(repeat=True)

    def hide_model(self) -> None:
        """
        Removes model from scene.
        """
        if self.model:
            self.art_obj_mgr.remove_object_by_handle(self.model.handle)
            self.model = None

    # currently the next_pose method is simply called twice in simulating a frame
    def next_pose(self, repeat=False) -> None:
        """
        Set the model state from the next frame in the motion trajectory. `repeat` is
        set to `True` when the user would like to repeat the last frame.
        """
        if not self.model or not self.motion:
            return

        # This function tracks is_reversed and changes the direction of
        # the motion accordingly.
        def sign(i):
            return -1 * i if self.is_reversed else i

        # repeat last frame: used mostly for position state change
        if repeat:
            self.motion_stepper = (
                self.motion_stepper - sign(1)
            ) % self.motion.num_frames()

        (
            new_pose,
            new_root_translate,
            new_root_rotation,
        ) = self.convert_CMUamass_single_pose(
            self.motion.poses[abs(self.motion_stepper)], self.model
        )
        self.model.joint_positions = new_pose
        self.model.rotation = new_root_rotation
        self.model.translation = new_root_translate

        # iterate the frame counter
        self.motion_stepper = (self.motion_stepper + sign(1)) % self.motion.num_frames()

    def convert_CMUamass_single_pose(
        self, pose, model
    ) -> Tuple[List[float], mn.Vector3, mn.Quaternion]:
        """
        This conversion is specific to the datasets from CMU
        """
        new_pose = []

        # Root joint
        root_T = pose.get_transform(ROOT, local=False)

        # adding offsets to root transformation
        # rotation
        root_rotation = self.rotation_offset * mn.Quaternion.from_matrix(
            mn.Matrix3x3(root_T[0:3, 0:3])
        )

        # translation
        root_translation = (
            self.translation_offset
            + self.rotation_offset.transform_vector(root_T[0:3, 3])
        )

        Q, _ = conversions.T2Qp(root_T)

        # Other joints
        for model_link_id in model.get_link_ids():
            joint_type = model.get_link_joint_type(model_link_id)
            joint_name = model.get_link_name(model_link_id)
            pose_joint_index = pose.skel.index_joint[joint_name]

            # When the target joint do not have dof, we simply ignore it
            if joint_type == phy.JointType.Fixed:
                continue

            # When there is no matching between the given pose and the simulated character,
            # the character just tries to hold its initial pose
            if pose_joint_index is None:
                raise KeyError(
                    "Error: pose data does not have a transform for that joint name"
                )
            elif joint_type not in [phy.JointType.Spherical]:
                raise NotImplementedError(
                    f"Error: {joint_type} is not a supported joint type"
                )
            else:
                T = pose.get_transform(pose_joint_index, local=True)
                if joint_type == phy.JointType.Spherical:
                    Q, _ = conversions.T2Qp(T)

            new_pose += list(Q)
        return new_pose, root_translation, root_rotation

    def setup_key_frames(self, num_key_frames: int = 10) -> None:
        """
        Prepares key frame data from the currently loaded motion data.
        """
        key_frames = [self.motion.poses[0]]

        total_dist = 0.0
        last_key_frame = self.motion.poses[0].get_transform(ROOT, local=False)[0:3, 3]

        # get total distance traveled by motion
        for pose in self.motion.poses:
            delta = pose.get_transform(ROOT, local=False)[0:3, 3] - last_key_frame
            total_dist += mn.Vector3(delta).length()
            last_key_frame = pose.get_transform(ROOT, local=False)[0:3, 3]

        # how much distance should occur between each key frame for the allotted frame count
        threshold = total_dist / num_key_frames
        last_key_frame = self.motion.poses[0].get_transform(ROOT, local=False)[0:3, 3]

        for pose in self.motion.poses:
            delta = pose.get_transform(ROOT, local=False)[0:3, 3] - last_key_frame
            if mn.Vector3(delta).length() >= threshold:
                key_frames.append(pose)
                last_key_frame = pose.get_transform(ROOT, local=False)[0:3, 3]

        self.key_frames = key_frames

    def toggle_key_frames(self) -> None:
        """
        Toggle the display of the key frames preview
        """
        if not self.motion:
            logger.error("Error: a motion is not loaded")
            return

        if (
            self.preview_mode in [Preview.KEYFRAMES, Preview.ALL]
            and not self.key_frame_models
        ):
            data = self.user_metadata

            for k in self.key_frames:

                self.key_frame_models.append(
                    self.art_obj_mgr.add_articulated_object_from_urdf(
                        filepath=data["urdf_path"], fixed_base=True
                    )
                )

                (
                    new_pose,
                    new_root_translate,
                    new_root_rotation,
                ) = self.convert_CMUamass_single_pose(k, self.key_frame_models[-1])

                self.key_frame_models[-1].motion_type = phy.MotionType.KINEMATIC
                self.key_frame_models[-1].joint_positions = new_pose
                self.key_frame_models[-1].rotation = new_root_rotation
                self.key_frame_models[-1].translation = new_root_translate

        elif self.key_frame_models:
            for m in self.key_frame_models:
                # removes key frames
                self.art_obj_mgr.remove_object_by_handle(m.handle)
            self.key_frame_models = []

    def build_trajectory_vis(self) -> None:
        """
        Build and display the trajectory visuals for the key frames
        """
        if not self.motion:
            logger.error("Error: a motion is not loaded")
            return

        points_to_preview = []

        if len(self.key_frames) >= 2:
            traj_radius = 0.02
            traj_offset = self.translation_offset + mn.Vector3(0, 0.1, 0)

            joint_names = [["rankle", "lankle"], "upperneck"]

            def define_preview_points(joint_names: List[str]) -> List[mn.Vector3]:
                """
                Pass in a list containing names of joints as strings and/or lists containing
                multiple names of joints, where the lists will result in an interpolated
                trajectory object being built from the mid point of the listed names and the
                single joint names will produce there own trajectory object. The total amount
                of trajectory objects that are produced is the length of the outer list.
                Example: joint_names = [["rankle", "lankle"], "upperneck"]


                Returns a list of lists of points to build the trajectory objects.
                """
                traj_objects = []
                joint_list = list(self.key_frames[0].skel.index_joint.keys())
                for joints in joint_names:
                    if not isinstance(joints, list):
                        joints = [joints]

                    # check if the joint names in this list are valid joints in the model
                    if not all(x in joint_list for x in joints):
                        logger.error(f"Error: Invalid joint names passed -> {joints}")
                        continue

                    midpoints = []
                    for i in self.key_frames:
                        mp = mn.Vector3.zero_init()
                        for j in joints:
                            mp += mn.Vector3(
                                i.get_transform(i.skel.index_joint[j], local=False)[
                                    0:3, 3
                                ]
                            )

                        mp /= len(joints)

                        midpoints.append(
                            traj_offset + self.rotation_offset.transform_vector(mp)
                        )
                    traj_objects.append(midpoints)
                return traj_objects

            points_to_preview = define_preview_points(joint_names)

        colors = [mn.Color3.red(), mn.Color3.yellow(), mn.Color3.green()]

        if self.preview_mode in [Preview.TRAJECTORY, Preview.ALL]:
            if not self.traj_ids:
                for i, p in enumerate(points_to_preview):
                    self.traj_ids.append(
                        self.sim.add_gradient_trajectory_object(
                            traj_vis_name=f"{joint_names[i]}{int(time.time() * 1000)}",
                            colors=colors,
                            points=p,
                            num_segments=3,
                            radius=traj_radius,
                            smooth=True,
                            num_interpolations=10,
                        )
                    )

        elif self.traj_ids:
            # removes trajectory
            for t_id in self.traj_ids:
                self.sim.get_rigid_object_manager().remove_object_by_id(t_id)
                self.traj_ids = []

    def cycle_model_previews(self) -> None:
        """
        Cycles through the Preview mode enum to display model/motion previews.
        """
        self.preview_mode = Preview((self.preview_mode.value + 1) % len(Preview))
        self.toggle_key_frames()
        self.build_trajectory_vis()

    def belongs_to(self, obj_id: int) -> bool:
        """
        Accepts an object id and returns True if the obj_id belongs to an object
        owned by this Fairmotion character.
        """
        # checking our model links
        if self.model and obj_id in self.model.link_object_ids:
            return True

        # checking our model
        if self.model and obj_id is self.model.object_id:
            return True

        # checking all key frame models
        if any(
            obj_id in ko_ids
            for ko_ids in (i.link_object_ids.keys() for i in self.key_frame_models)
        ):
            return True

        # checking all key frame models
        if any(obj_id == to for to in self.traj_ids):
            return True

        return False


class Preview(Enum):
    OFF = 0
    KEYFRAMES = 1
    TRAJECTORY = 2
    ALL = 3

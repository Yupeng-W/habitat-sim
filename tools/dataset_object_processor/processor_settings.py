# Copyright (c) Facebook, Inc. and its affiliates.
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

from habitat_sim.utils.settings import default_sim_settings
from habitat_sim.utils.settings import make_cfg as _make_cfg

# -------------------------------------------------------------------
# All, any, or none of these settings can be overridden in the
# dataset_processor_config.json files. In those configs, you only
# have to include the settings you want to override.
# -------------------------------------------------------------------
dataset_processor_config_fields = {
    # "scene": scene to use for testing dataset objects
    "scene": "data/test_assets/scenes/simple_room.glb",
    # "csv": if we are making a csv file of the results of any/all of our tests
    # "video": if we are making a video recording of any/all of our tests
    "outputs": {"csv": True, "video": True},
    # "csv": relative path from 'habitat_sim' that csv files are saved
    # "video": relative path from 'habitat_sim' that video recordings are saved
    # "output_file_prefix": prefix to put at the beginning of the filename.
    # Filename will be of the form:
    # <path to dir>/<filename_prefix>__date_<year>-<month>-<day>__time_<hour>:<min>:<sec>.
    # If filename_prefix is the empty string or None:
    # <path to dir>/date_<year>-<month>-<day>__time_<hour>:<min>:<sec>
    "output_paths": {
        "csv": "tools/dataset_object_processor/dataset_csv_files/",
        "video": "tools/dataset_object_processor/dataset_video_recordings/",
        "output_file_prefix": "",
    },
    # "start_obj_index": index of the dataset object to start processing. If this
    # or the next settings value is a string, process all objects.
    "start_obj_index": 0,
    # "num_objects": number of dataset objects to process in total. Process asset
    # indices 0 through num_objects-1 of the dataset. If this or the previous
    # settings value is a string, process all objects.
    "num_objects": "all",
    # if making a csv, this dictates the data you want to collect for it
    # "extensive_RAM_calculation": load and then remove the asset with the
    # RigidObjectManager many times, then take the average RAM usage.
    # "memory_data": collect RAM usage, size of meshes, and size of images of asset
    # "render_time_ratio": time asset takes to render vs duration of frame (dt)
    # "physics_data": snap down asset from 6 orthogonal rotations and see how long
    # it takes to fall asleep. Calculate difference in position, rotation, and the
    # ratio of how long it takes to simulate a frame versus frame duration (dt)
    "csv_data_to_collect": {
        "ram_calc_multiple_samples": True,
        "memory_data": True,
        "render_time_ratio": True,
        "physics_data": True,
    },
    # these refer to which metrics you will use to calculate how much RAM an object
    # uses when you load it using the RigidObjectManager. Refer to the
    # psutil.virtual_memory() function to see what each metric means, as "available"
    # and "free" seem like the same thing, but they aren't. I calculate the difference
    # in these metrics before and after loading the object, then take the average of them.
    # All possible metrics are:
    # "total", "available", "percent", "used", "free", "active", "inactive", "buffers",
    # "cached", "shared", and "slab", but only really "available", "used", "free",
    # "active", and "inactive" work with the way the script is currently implemented.
    "ram_metrics_to_use": ["available", "used", "free"],
    # These indicate the units and thresholds you will allow when collecting
    # memory data from each asset.
    # "units": units used to measure asset sizes. Possible values are "bytes", "KB",
    # "MB", and "GB"
    # "max_mesh_num": flag any asset with more than this number of meshes. E.g., many
    # have a render mesh and collision mesh.
    # "max_mesh_size": max size in "units", e.g. KB, that meshes are allowed to be.
    # It is total size of all meshes and images.
    # asset whose cumulative mesh sizes exceeds 'max_mesh_size KB', in this case,
    # "max_image_num": max number of cumulative textures (often total mipmaps) an asset
    # can have before flagging
    # "max_image_size": cumulative max memory size of all the textures and mip maps
    # the asset can have before flagging
    "memory_vars": {
        "units": "KB",
        "ram_calc_num_samples": 10,
        "max_mesh_num": 5,
        "max_mesh_size": 2400,
        "max_image_num": 32,
        "max_image_size": 12000,
    },
    # "fps": framerate used for physics simulation and video recording
    # "max_wait_time": max number of "time_units" script will wait for an object to become
    # asleep after snapping to ground.
    # "distance_units": Units used when measuring distances. Possible values are "cm"
    # (centimeters) and "m" (meters)
    # "angle_units" Units used when measuring angles.  Possible values are "deg" (degrees)
    # or "rad" (radians)
    "physics_vars": {
        "fps": 60,
        "time_units": "sec",
        "max_wait_time": 5,
        "distance_units": "m",
        "angle_units": "deg",
    },
    # "tasks": which tasks to record when making a video.
    #   "draw_bbox": draw the object's bbox as you rotate/display it in KINEMATIC movement mode
    #   "draw_collision_mesh": draw the object's collision mesh asset with lighting instead of
    #   its render asset as you rotate/display it in KINEMATIC mode
    #   "draw_collision_mesh_wireframe": same as "draw_collision_mesh", but instead of drawing
    #   the collision asset mesh with lighting, draw it as a wireframe.
    #   "record_physics": record the object during its physics tests descripted above
    # "physics_recording_pos": position of the agent when recording the physics tests
    # "physics_recording_rot": rotation of the agent when recording the physics tests
    # "revolution_dur": number of "time_units" it takes for the object to turn 360 degrees
    # when displaying it in KINEMATIC mode
    "video_vars": {
        "tasks": {
            "draw_bbox": True,
            "draw_collision_mesh": True,
            "draw_collision_mesh_wireframe": True,
            "record_physics": True,
        },
        "revolution_angle": 360.0,
        "revolution_dur": 4.0,
        "physics_recording_pos": [-2.0, 0.5, 4.00],
        "physics_recording_rot": {"angle": -20.0, "axis": [1, 0, 0]},
    },
    # "default_transforms": transforms that objects or agent are initialized as or often
    # reverted to.
    #   "default_agent_pos": initial position of agent
    #   "default_agent_rot": initial rotation of agent when recording objects in KINEMATIC
    #   mode.
    #   "default_obj_pos": initial position of rigid object when instantiated
    #   "default_obj_rot": initial rotation of rigid object when instantiated
    "default_transforms": {
        "default_agent_pos": [-2.0, 0.5, 4.00],
        "default_agent_rot": {"angle": 0.0, "axis": [1.0, 0.0, 0.0]},
        "default_obj_pos": [-2.0, 1.5, 2.00],
        "default_obj_rot": {"angle": 0.0, "axis": [1.0, 0.0, 0.0]},
    },
    # "headless": if simulator will be rendering to a window
    "headless": True,
    # "enable_physics": allow physics simulations with gravity
    "enable_physics": True,
    # "sensor_height": height of sensor above agent position
    "sensor_height": 1.0,
    # "silent": if program will log information to the console during normal program run
    "silent": True,
    # "debug_print": if program will log information to the console for debugging purposes
    "debug_print": False,
    # "bbox_rgb": color of bounding box if recording video of rigid objects' bboxes
    "bbox_rgb": [1.0, 0.8, 1.0],
}

# -------------------------------------------------------------------
# The following settings shouldn't be changed
# -------------------------------------------------------------------
dataset_processor_constants = {
    # Title of first column of csv file. object_config.json file name
    "object_name": [
        "object template file",
    ],
    # Column titles of CSV file when memory stats are requested
    "memory_data_headers": [
        "MEMORY --->",
        "RAM",
        "render asset file",
        "collision asset file",
        "mesh count",
        "index data size",
        "vertex data size",
        "total mesh data size",
        "image count",
        "image data size",
        "total asset size",
    ],
    # Column titles of CSV file when rendering stats are requested
    "render_time_headers": [
        "RENDERING --->",
        "Render Time Ratio",
    ],
    # Column titles of CSV file when physics stats are requested
    "physics_data_headers": [
        "PHYSICS --->",
        "wait times\nfor rotations",
        "angle: 0,\naxis (1, 0, 0)",
        "angle: 90,\naxis (1, 0, 0)",
        "angle: 180,\naxis (1, 0, 0)",
        "angle: 90,\naxis (-1, 0, 0)",
        "angle: 90,\naxis (0, 0, -1)",
        "angle: 90,\naxis (0, 0, 1)",
        "position\ndeltas",
        "pose 1",
        "pose 2",
        "pose 3",
        "pose 4",
        "pose 5",
        "pose 6",
        "rotation\ndeltas",
        "pose 1",
        "pose 2",
        "pose 3",
        "pose 4",
        "pose 5",
        "pose 6",
        "(sim time)/dt",
        "pose 1",
        "pose 2",
        "pose 3",
        "pose 4",
        "pose 5",
        "pose 6",
    ],
    # we must run physics tests on rigid objects in 6 different, orthogonal
    # orientations, each corresponding to a face of an imaginary cube
    # bounding the object. Each rotation is of the form:
    # (angle in degrees, (axis.x, axis.y, axis.z))
    "sim_test_rotations": [
        (0.0, (1, 0, 0)),
        (90.0, (1, 0, 0)),
        (180.0, (1, 0, 0)),
        (90.0, (-1, 0, 0)),
        (90.0, (0, 0, -1)),
        (90.0, (0, 0, 1)),
    ],
    # "mem_delta_order" is either -1 or 1. 1 means the delta is
    # calculated as (end_start - start_state), whereas -1 means
    # (start_state - end_state). E.g. Data "used" should be higher
    # after loading, so mem_delta_order == 1, but data free should
    # be higher before loading, so mem_delta_order == -1
    "mem_delta_order": {
        "available": -1,
        "percent": 1,
        "used": 1,
        "free": -1,
        "active": -1,
        "inactive": 1,
    },
}


default_sim_settings.update(dataset_processor_config_fields)
default_sim_settings.update(dataset_processor_constants)


def make_cfg(settings):
    return _make_cfg(settings)

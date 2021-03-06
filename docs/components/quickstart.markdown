## Getting Started
 
At its core, OSPRay Studio maintains a scenegraph of nodes representing the
current scene as a *directed acyclic graph*. This simplifies user interaction
with the scene and hides the maintenance of OSPRay state. The scenegraph
manages the four primary OSPRay components: the `FrameBuffer`, `Camera`,
`Renderer`, and `World`. OSPRay Studio currently offers three different modes
in which it can operate. You can choose between these modes when launching
OSPRay Studio at the command line.
 
### OSPRay Studio Modes

* **GUI Mode** - `./ospStudio [gui] [file1 [file2 ...]]`

This is the default mode in OSPRay Studio.  Objects can be loaded from command
line or via the UI. This mode provides full control over the scenegraph,
allowing manipulation of the camera, objects, lights, and more. This mode also
allows you to create scene files (.sg files), which can be used to load a full
scene in any mode.
 
* **Batch Mode** - `./ospStudio batch [options] [file1 [file2 ...]]`

This mode provides offline rendering for the objects (or scene file) provided.
Batch mode currently supports saving a single image or multiple frames for
animation scenes. For more information on available options, run `./ospStudio
batch --help`.
 
* **Timeseries Mode** - `./ospStudio timeseries [options]`

This mode can be used to import timestep volume datasets in raw binary or
OpenVDB formats. This mode is similar to GUI mode but provides extra support
and UI for loading and manipulating time-varying data.
 
### GUI Mode Features

OBJ and static glTF files can be loaded using the File -> Import option.
Animated glTF files can be loaded using File -> Import and animate. This will
automatically provide animation controls upon load. Animation with skinning is
supported in glTF format.

Demo scenes can be loaded in File -> Demo Scene. These are a good starting
point to try out various OSPRay Studio controls.  The Edit menu allows
immediate editing of renderer and frame buffer settings.  The View Menu
contains editors for more complex behaviors, including

* Keyframes for animated camera paths
* Camera snapshots (which can be saved and used across multiple sessions)
* Adding, removing, and manipulating lights in the scene
* Manipulating geometry and materials within the scene
* Animating time-varying data (e.g. animated glTF files or simulations)

The GUI mode allows you to save scene files (.sg files), which can be later
reloaded.  These are JSON-formatted files describing the current scene,
including loaded objects/data, lights, and camera state. Scene files are
human-readable and editable. Note that they do not include the actual data of
objects in the scene, but rather instruct OSPRay Studio to load and transform
the specified objects.  Material, light, and camera information can optionally
be saved separately from scene files.

The GUI mode can export screenshots of the current frame in EXR, HDR, PPM/PFM,
JPEG, and PNG formats. EXR supports the following framebuffer channels to be
exported as additional layers:

-   albedo
-   normal
-   depth
-   metadata (currently, geometric and instance IDs of scene objects)

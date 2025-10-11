#pragma once

#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/vec_float.h"
#include "DisplayManager.hpp"


namespace ozz {
namespace math {
struct Box;
}
namespace sample {
class ImGui;
namespace internal {

// Framework internal implementation of an OpenGL/glfw camera system that can be
// manipulated with the mouse and some shortcuts.
class Camera {
 public:
  // Initializes camera to its default framing.
  Camera(DisplayManager& display);

  // Destructor.
  ~Camera();

  // Updates camera framing: mouse manipulation, timed transitions...
  // Returns actions that the user applied to the camera during the frame.
  void Update(const math::Box& _box, float _delta_time, bool _first_frame);

  // Updates camera location, overriding user inputs.
  // Returns actions that the user applied to the camera during the frame.
  void Update(const math::Float4x4& _transform, const math::Box& _box,
              float _delta_time, bool _first_frame);

  // Resets camera center, angles and distance.
  void Reset(const math::Float3& _center, const math::Float2& _angles,
             float _distance);

  // Provides immediate mode gui display event.
  void OnGui(ImGui* _im_gui);

  // Binds 3d projection and view matrices to the current matrix.
  void Bind3D();

  // Binds 2d projection and view matrices to the current matrix.
  void Bind2D();

  // Resize notification, used to rebuild projection matrix.
  void Resize(int _width, int _height);

  int GetMousePos()
  {

  }

  int GetMouseWheel()
  {

  }

  // Get the current projection matrix.
  const math::Float4x4& projection() { return projection_; }

  // Get the current model-view matrix.
  const math::Float4x4& view() { return view_; }

  // Get the current model-view-projection matrix.
  const math::Float4x4& view_proj() { return view_proj_; }

  // Set to true to automatically frame the camera on the whole scene.
  void set_auto_framing(bool _auto) { auto_framing_ = _auto; }
  // Get auto framing state.
  bool auto_framing() const { return auto_framing_; }

 public:
  struct Controls {
    bool zooming;
    bool zooming_wheel;
    bool rotating;
    bool panning;
  };
  Controls UpdateControls(float _delta_time);

  // The current projection matrix.
  math::Float4x4 projection_;

  // The current projection matrix.
  math::Float4x4 projection_2d_;

  // The current model-view matrix.
  math::Float4x4 view_;

  // The current model-view-projection matrix.
  math::Float4x4 view_proj_;

  // The angles in degree of the camera rotation around x and y axes.
  math::Float2 angles_;

  // The center of the rotation.
  math::Float3 center_;

  // The view distance, from the center of rotation.
  float distance_;

  // The position of the mouse, the last time it has been seen.
  int mouse_last_x_;
  int mouse_last_y_;
  int mouse_last_wheel_;
  DisplayManager& display;
  // Set to true to automatically frame the camera on the whole scene.
  bool auto_framing_;
};
}  // namespace internal
}  // namespace sample
}  // namespace ozz
extern crate glfw;

use glfw::Context;

fn main() {
    let mut glfw = glfw::init(glfw::FAIL_ON_ERRORS).unwrap();

    let (mut window, _events) = glfw
        .create_window(300, 300, "Hello this is window", glfw::WindowMode::Windowed)
        .expect("Failed to create GLFW window.");

    window.set_key_polling(true);
    window.make_current();

    while !window.should_close() {
        glfw.poll_events();
    }
}

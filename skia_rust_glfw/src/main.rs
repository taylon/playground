extern crate glfw;
use skia_safe::{gpu, Budgeted, Color, EncodedImageFormat, ImageInfo, Paint, Rect, Surface};
use std::fs;
use std::io::Write;
use std::path::PathBuf;

use glfw::Context;

fn write_file(bytes: &[u8]) {
    let path = PathBuf::from(".");
    fs::create_dir_all(&path).expect("failed to create directory");

    let mut file_path = path.join("testing");
    file_path.set_extension("png");

    let mut file = fs::File::create(file_path).expect("failed to create file");
    file.write_all(bytes).expect("failed to write to file");
}

fn draw_rect(surface: &mut Surface) {
    let canvas = surface.canvas();
    canvas.scale((2.0, 2.0));
    canvas.save();
    canvas.translate((128.0, 128.0)).rotate(45.0, None);

    let rect = Rect::from_point_and_size((-90.5, -90.5), (181.0, 181.0));

    let mut paint = Paint::default();
    paint.set_color(Color::BLUE);

    canvas.draw_rect(rect, &paint);
    canvas.restore();
}

fn main() {
    let mut glfw = glfw::init(glfw::FAIL_ON_ERRORS).unwrap();
    let (mut window, _events) = glfw
        .create_window(300, 300, "RUST_SKIA_GLFW", glfw::WindowMode::Windowed)
        .expect("Failed to create GLFW window.");
    window.make_current();

    let mut context = gpu::Context::new_gl(None).unwrap();

    let image_info = ImageInfo::new_n32_premul((300, 300), None);
    let mut surface = Surface::new_render_target(
        &mut context,
        Budgeted::Yes,
        &image_info,
        None,
        gpu::SurfaceOrigin::BottomLeft,
        None,
        false,
    )
    .unwrap();

    draw_rect(&mut surface);

    context.flush_and_submit();
    window.swap_buffers();

    // let mut surface = Surface::new_raster_n32_premul((256, 256)).unwrap();
    let image = surface.image_snapshot();
    let data = image.encode_to_data(EncodedImageFormat::PNG).unwrap();
    write_file(data.as_bytes());

    // while !window.should_close() {
    //     glfw.poll_events();
    // }
}

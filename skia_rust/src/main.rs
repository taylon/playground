use skia_safe::{Canvas, EncodedImageFormat, Surface};
use skia_safe::{Color, Paint, Rect};
use std::fs;
use std::io::Write;
use std::path::PathBuf;

fn write_file(bytes: &[u8]) {
    let path = PathBuf::from(".");
    fs::create_dir_all(&path).expect("failed to create directory");

    let mut file_path = path.join("testing");
    file_path.set_extension("png");

    let mut file = fs::File::create(file_path).expect("failed to create file");
    file.write_all(bytes).expect("failed to write to file");
}

fn draw_rect(canvas: &mut Canvas) {
    canvas.save();
    canvas.translate((128.0, 128.0)).rotate(45.0, None);

    let rect = Rect::from_point_and_size((-90.5, -90.5), (181.0, 181.0));

    let mut paint = Paint::default();
    paint.set_color(Color::BLUE);

    canvas.draw_rect(rect, &paint);
    canvas.restore();
}

fn main() {
    let mut surface = Surface::new_raster_n32_premul((256, 256)).unwrap();

    let mut canvas = surface.canvas();
    canvas.scale((2.0, 2.0));

    draw_rect(&mut canvas);

    let image = surface.image_snapshot();
    let data = image.encode_to_data(EncodedImageFormat::PNG).unwrap();

    write_file(data.as_bytes());
}

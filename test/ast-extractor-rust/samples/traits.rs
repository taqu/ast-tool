pub trait Drawable {
    fn draw(&self);
    fn bounds(&self) -> (f32, f32);
}

trait Private {
    fn setup(&self);
}

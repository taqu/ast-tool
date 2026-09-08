mod geometry {
    pub struct Circle {
        pub radius: f32,
    }

    pub fn area(radius: f32) -> f32 {
        3.14 * radius * radius
    }
}

mod math {
    pub mod ops {
        pub fn multiply(a: i32, b: i32) -> i32 {
            a * b
        }
    }
}

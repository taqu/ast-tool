pub mod network {
    pub mod http {
        pub const DEFAULT_TIMEOUT: u32 = 30;
        pub static AGENT: &str = "rust-client";

        pub type Result<T> = std::result::Result<T, Error>;

        #[derive(Debug)]
        pub struct Error {
            pub code: u32,
            message: String,
        }

        pub enum Status {
            Ok,
            NotFound,
            ServerError(String),
        }

        pub trait Connectable {
            fn connect(&self) -> bool;
            fn disconnect(&self);
            fn timeout(&self) -> u32 { 30 }
        }

        pub struct Client {
            pub host: String,
            port: u16,
        }

        impl Client {
            pub fn new(host: &str, port: u16) -> Self {
                Client { host: host.to_string(), port }
            }

            pub fn get(&self, path: &str) -> Status {
                Status::Ok
            }

            fn build_url(&self, path: &str) -> String {
                format!("{}:{}{}", self.host, self.port, path)
            }
        }

        impl Connectable for Client {
            fn connect(&self) -> bool { true }
            fn disconnect(&self) {}
        }
    }
}

pub struct Point {
    pub x: f64,
    pub y: f64,
}

impl Point {
    pub fn new(x: f64, y: f64) -> Self { Point { x, y } }
    pub fn distance(&self, other: &Point) -> f64 {
        ((self.x - other.x).powi(2) + (self.y - other.y).powi(2)).sqrt()
    }
}

pub trait Shape {
    fn area(&self) -> f64;
    fn perimeter(&self) -> f64;
    fn name(&self) -> &str { "Shape" }
}

pub enum Color {
    Red,
    Green,
    Blue,
    Custom(u8, u8, u8),
}

pub const MAX_SIZE: usize = 1024;
pub static COUNTER: std::sync::atomic::AtomicUsize =
    std::sync::atomic::AtomicUsize::new(0);

pub fn add(a: i32, b: i32) -> i32 { a + b }
pub fn greet(name: &str) -> String { format!("Hello, {}!", name) }

pub type Callback = fn(i32) -> i32;
pub type Matrix = Vec<Vec<f64>>;

fn main() {
    let x = add(1, 2);
}

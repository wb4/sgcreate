use anyhow::{anyhow, Result};
use regex::Regex;

use std::collections::HashMap;
use std::env;
use std::ops;
use std::process;

const EYE_SEPARATION: Length = Length {
    meters: 6.2 * METERS_PER_CENTIMETER,
};

const METERS_PER_METER: f32 = 1.0;
const METERS_PER_CENTIMETER: f32 = 0.01;
const METERS_PER_MILLIMETER: f32 = 0.001;
const METERS_PER_INCH: f32 = 0.0254;

fn main() {
    let args = env::args().collect::<Vec<_>>();

    if args.len() != 6 {
        print_usage(&args[0]);
        process::exit(1);
    }

    let params = match args[1..]
        .iter()
        .map(|s| Length::try_from_str(&s))
        .collect::<Result<Vec<_>>>()
    {
        Ok(params) => params,
        Err(e) => {
            eprintln!("{e}");
            eprintln!();
            print_usage(&args[0]);
            process::exit(1);
        }
    };

    let face_distance = params[0];
    let image_width = params[1];
    let image_height = params[2];
    let min_separation = params[3];
    let max_separation = params[4];

    let fov = 2.0 * (image_width.min(image_height) / (2.0 * face_distance)).atan();
    let min_distance = distance_for_separation(min_separation, EYE_SEPARATION, face_distance);
    let max_distance = distance_for_separation(max_separation, EYE_SEPARATION, face_distance);

    println!("Camera FOV: {:.1}°", fov.to_degrees());
    println!("Min dstance: {:.2}cm", min_distance.to_centimeters());
    println!("Max dstance: {:.2}cm", max_distance.to_centimeters());
}

fn print_usage(app: &str) {
    eprintln!("Usage: {app} <face_distance> <image_width> <image_height> <min_separation> <max_separation>");
    eprintln!();
    eprintln!("All parameters are lengths and must include units.  Accepted units are meters,");
    eprintln!("centimeters, millimeters, and inches.  These can be abbreviated as m, cm, and");
    eprintln!("in, respectively.");
}

fn distance_for_separation(
    separation: Length,
    eye_separation: Length,
    face_distance: Length,
) -> Length {
    let separation = separation.to_meters();
    let eye_separation = eye_separation.to_meters();
    let face_distance = face_distance.to_meters();

    Length {
        meters: (eye_separation * face_distance) / (eye_separation - separation),
    }
}

#[derive(Copy, Clone)]
struct Length {
    meters: f32,
}

impl Length {
    fn from_meters(meters: f32) -> Self {
        Self { meters }
    }

    fn try_from_str(s: &str) -> Result<Self> {
        let unit_conversion_factors: HashMap<&str, f32> = HashMap::from([
            ("meters", METERS_PER_METER),
            ("meter", METERS_PER_METER),
            ("m", METERS_PER_METER),
            ("centimeters", METERS_PER_CENTIMETER),
            ("centimeter", METERS_PER_CENTIMETER),
            ("cm", METERS_PER_CENTIMETER),
            ("millimeters", METERS_PER_MILLIMETER),
            ("millimeter", METERS_PER_MILLIMETER),
            ("mm", METERS_PER_MILLIMETER),
            ("inches", METERS_PER_INCH),
            ("inch", METERS_PER_INCH),
            ("in", METERS_PER_INCH),
        ]);

        let re = Regex::new(r"^\s*(?<scalar>(\d+(\.\d*)?)|(\.\d+))\s*(?<units>[[:alpha:]]+)\s*$")
            .expect("You messed up the regular expression.  Fix it.");

        let caps = re.captures(s).ok_or(anyhow!("bad input format"))?;

        let scalar = &caps["scalar"]
            .parse::<f32>()
            .expect("scalar failed to parse as f32.  Fix your regular expression.");
        let units = &caps["units"];

        let conversion_factor = *unit_conversion_factors
            .get(units)
            .ok_or(anyhow!("invalid units"))?;

        Ok(Length::from_meters(scalar * conversion_factor))
    }

    fn min(self, other: Self) -> Self {
        Length::from_meters(self.to_meters().min(other.to_meters()))
    }

    fn to_meters(self) -> f32 {
        self.meters
    }

    fn to_centimeters(self) -> f32 {
        self.meters / METERS_PER_CENTIMETER
    }
}

impl ops::Mul<f32> for Length {
    type Output = Self;

    fn mul(self, rhs: f32) -> Self {
        Length::from_meters(self.to_meters() * rhs)
    }
}

impl ops::Mul<Length> for f32 {
    type Output = Length;

    fn mul(self, rhs: Length) -> Length {
        rhs * self
    }
}

impl ops::Div<Self> for Length {
    type Output = f32;

    fn div(self, rhs: Self) -> f32 {
        self.meters / rhs.meters
    }
}

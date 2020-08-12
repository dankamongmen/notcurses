extern crate bindgen;
extern crate pkg_config;

use std::env;
use std::path::PathBuf;

// largely taken from https://rust-lang.github.io/rust-bindgen/tutorial-3.html
fn main() {
    let plib = pkg_config::Config::new()
        .atleast_version("1.6.11")
        .probe("notcurses")
        .unwrap();

    // Tell cargo to invalidate the built crate whenever the wrapper changes
    println!("cargo:rerun-if-changed=wrapper.h");

    cc::Build::new()
        .file("stdout.c")
        .compile("stdout");

    // The bindgen::Builder is the main entry point to bindgen, and lets you
    // build up options for the resulting bindings.
    let mut builder = bindgen::Builder::default()
        .use_core()
        .ctypes_prefix("cty")
        .clang_arg("-D_XOPEN_SOURCE")
        // The input header we would like to generate
        // builder for.
        .header("wrapper.h")
        // generate comments, also from headers and not just doc comments (///)
        .generate_comments(true)
        .clang_arg("-fretain-comments-from-system-headers")
        .clang_arg("-fparse-all-comments")
        // Remove warnings about improper_ctypes
        .blacklist_function("strtold")
        .blacklist_function("wcstold")
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks));

    for d in plib.include_paths {
        builder = builder.clang_arg(format!("-I{}", d.to_string_lossy()));
    }

    // Finish the builder and generate the builder.
    let bindings = builder.generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}

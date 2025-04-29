fn main() {
    // Tell cargo to rebuild if any of these files change
    println!("cargo:rerun-if-changed=../C_Components/ast.h");
    println!("cargo:rerun-if-changed=../C_Components/parser.h");
    println!("cargo:rerun-if-changed=../C_Components/ast.c");
    println!("cargo:rerun-if-changed=../C_Components/parser.c");
    println!("cargo:rerun-if-changed=../C_Components/lexer.c");
    println!("cargo:rerun-if-changed=../C_Components/lexer.l");
    println!("cargo:rerun-if-changed=../liblogic.so");
    
    // Generate Rust bindings from C header
    let bindings = bindgen::Builder::default()
        .header("../C_Components/ast.h")
        .generate()
        .expect("Unable to generate bindings");

    bindings
        .write_to_file("src/ast_bindings.rs")
        .expect("Couldn't write bindings!");
        
    // Link to our shared library
    let current_dir = std::env::current_dir().unwrap();
    let project_root = current_dir.parent().unwrap();
    println!("cargo:rustc-link-search=native={}", project_root.display());
    println!("cargo:rustc-link-lib=dylib=logic");
    
    // Add linking for C runtime libraries
    println!("cargo:rustc-link-lib=dylib=c");
    println!("cargo:rustc-link-lib=dylib=m");
    println!("cargo:rustc-link-lib=dylib=stdc++");
}

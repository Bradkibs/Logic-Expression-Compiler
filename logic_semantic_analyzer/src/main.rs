use std::env;
use std::process;

fn print_usage() {
    println!("Usage: logic_runner <input_file> <output_file>");
    println!("  <input_file>  - Path to the input .lec file");
    println!("  <output_file> - Path to write the output");
}

fn main() {
    let args: Vec<String> = env::args().collect();
    
    if args.len() != 3 {
        println!("Error: Incorrect number of arguments");
        print_usage();
        process::exit(1);
    }

    let input_file = &args[1];
    let output_file = &args[2];

    println!("Processing logical expressions...");
    println!("Input file: {}", input_file);
    println!("Output file: {}", output_file);

    match logic_semantic_analyzer::process_lec_file(input_file, output_file) {
        Ok(_) => println!("Processing completed successfully"),
        Err(e) => {
            eprintln!("Error processing file: {}", e);
            process::exit(1);
        }
    }
}

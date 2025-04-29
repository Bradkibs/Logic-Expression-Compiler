pub mod ast;
mod ast_bindings;

use std::fs::{File, OpenOptions};
use std::io::{self, BufRead, BufReader, Write};
use std::error::Error;
use ast::evaluate_expression;

#[derive(Debug)]
pub enum LogicError {
    IoError(io::Error),
    EvaluationError(String),
}

impl From<io::Error> for LogicError {
    fn from(err: io::Error) -> Self {
        LogicError::IoError(err)
    }
}

impl std::fmt::Display for LogicError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            LogicError::IoError(e) => write!(f, "IO error: {}", e),
            LogicError::EvaluationError(msg) => write!(f, "Evaluation error: {}", msg),
        }
    }
}

impl Error for LogicError {}

pub fn process_lec_file(input_path: &str, output_path: &str) -> Result<(), LogicError> {
    let input_file = File::open(input_path)?;
    let reader = BufReader::new(input_file);
    let mut output_file = OpenOptions::new()
        .create(true)
        .write(true)
        .truncate(true)
        .open(output_path)?;

    for line in reader.lines() {
        let expression = line?;
        let expression = expression.trim();
        if expression.is_empty() {
            continue;
        }

        writeln!(output_file, "Expression: {}", expression)?;
        
        let steps = evaluate_expression(expression);
        for step in steps {
            writeln!(output_file, "  {}", step)?;
        }
        
        writeln!(output_file)?; // Add blank line between expressions
    }

    Ok(())
}

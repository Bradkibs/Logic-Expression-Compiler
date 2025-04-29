use std::ffi::{CStr, CString};
use crate::ast_bindings;

pub fn evaluate_expression(expression: &str) -> Vec<String> {
    let c_expr = match CString::new(expression) {
        Ok(expr) => expr,
        Err(_) => return vec!["Error: Expression contains invalid characters".to_string()],
    };
    
    unsafe {
        let eval_steps = ast_bindings::evaluate_expression(c_expr.as_ptr());
        if eval_steps.is_null() {
            return vec!["Error: Could not evaluate expression".to_string()];
        }
        
        let step_count = (*eval_steps).step_count as usize;
        let mut results = Vec::with_capacity(step_count);
        
        for i in 0..step_count {
            let step = *(*eval_steps).steps.offset(i as isize);
            if !step.is_null() {
                let desc = (*step).step_description;
                if !desc.is_null() {
                    if let Ok(step_str) = CStr::from_ptr(desc).to_str() {
                        results.push(step_str.to_string());
                    }
                }
            }
        }
        
        // Free the C resources
        ast_bindings::free_evaluation_steps(eval_steps);
        
        if results.is_empty() {
            vec!["No evaluation steps returned".to_string()]
        } else {
            results
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_simple_evaluation() {
        let steps = evaluate_expression("A AND B");
        assert!(!steps.is_empty());
    }
}

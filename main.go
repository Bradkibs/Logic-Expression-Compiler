package main

/*
#cgo LDFLAGS: -L. -llogic
#cgo CFLAGS: -I./C_Components
#include "./C_Components/ast.h"
#include <stdlib.h>

Node* apply_logical_laws(Node* node, EvaluationSteps* steps);
EvaluationSteps* init_evaluation_steps();
void add_evaluation_step(EvaluationSteps* steps, const char* description);
EvaluationSteps* evaluate_multiple_expressions(const char* expression);
int get_steps_count(EvaluationSteps* steps);
char* get_step_at(EvaluationSteps* steps, int index);
void free_evaluation_steps(EvaluationSteps* steps);
*/
import "C"

import (
	"bufio"
	"fmt"
	"os"
	"unsafe"
)

// Reads a .lec file and returns its content as a string
func ReadLECFile(filename string) (string, error) {
	file, err := os.Open(filename)
	if err != nil {
		return "", fmt.Errorf("could not open file: %v", err)
	}
	defer file.Close()

	var content string
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		content += scanner.Text() + "\n"
	}
	if err := scanner.Err(); err != nil {
		return "", fmt.Errorf("error reading file: %v", err)
	}

	return content, nil
}

// Evaluates the logical expression via the C backend and writes steps to a file
func EvaluateExpression(expression string) error {
	cExpr := C.CString(expression)
	defer C.free(unsafe.Pointer(cExpr))

	steps := C.evaluate_multiple_expressions(cExpr)

	if steps == nil {
		return fmt.Errorf("evaluation returned null")
	}
	defer C.free_evaluation_steps(steps)

	file, err := os.Create("output.txt")
	if err != nil {
		return err
	}
	defer file.Close()

	count := int(C.get_steps_count(steps))
	for i := 0; i < count; i++ {
		cStep := C.get_step_at(steps, C.int(i))
		step := C.GoString(cStep)
		_, err := file.WriteString(step + "\n")
		if err != nil {
			return err
		}
	}

	return nil
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage: ./logic <input>.lec")
		return
	}

	lecFile := os.Args[1]
	expression, err := ReadLECFile(lecFile)
	if err != nil {
		fmt.Println("Error reading .lec file:", err)
		return
	}

	err = EvaluateExpression(expression)
	if err != nil {
		fmt.Println("Error evaluating expression:", err)
	} else {
		fmt.Println("Expression evaluated and steps written to output.txt")
	}
}
package main

/*
#cgo LDFLAGS: -L. -llogic
#cgo CFLAGS: -I./C_Components
#include "ast.h"


Node* apply_logical_laws(Node* node, EvaluationSteps* steps);
EvaluationSteps* init_evaluation_steps();
void add_evaluation_step(EvaluationSteps* steps, const char* description);
EvaluationSteps* evaluate_expression(const char* expression);
*/
import "C"
import (
	"bufio"
	"fmt"
	"os"
	"unsafe"
)

// Helper function to convert C string to Go string
func cStringToGoString(cStr *C.char) string {
	return C.GoString(cStr)
}

// Function to call apply_logical_laws from C
func ApplyLogicalLaws(root *C.Node) (*C.Node, []string) {
	steps := C.init_evaluation_steps()
	result := C.apply_logical_laws(root, steps)

	// Getting steps from the EvaluationSteps structure
	var stepList []string
	for i := 0; i < int(C.get_steps_count(steps)); i++ {
		step := C.GoString((**C.char)(unsafe.Pointer(uintptr(unsafe.Pointer(C.get_steps(steps))) + uintptr(i)*unsafe.Sizeof(*C.get_steps(steps)))))
		stepList = append(stepList, step)
	}

	return result, stepList
}

// Function to call evaluate_expression from C and write results to a file
func EvaluateExpression(expression string) error {
	// C function call for evaluating the expression
	cExpression := C.CString(expression)
	steps := C.evaluate_expression(cExpression)

	// Write evaluation steps to a file
	file, err := os.Create("output.txt")
	if err != nil {
		return err
	}
	defer file.Close()

	stepCount := int(C.get_steps_count(steps))
	for i := 0; i < stepCount; i++ {
		step := C.GoString((**C.char)(unsafe.Pointer(uintptr(unsafe.Pointer(C.get_steps(steps))) + uintptr(i)*unsafe.Sizeof(*C.get_steps(steps)))))
		_, err := file.WriteString(step + "\n")
		if err != nil {
			return err
		}
	}

	return nil
}

// Function to read the .lec file
func ReadLECFile(filename string) (string, error) {
	file, err := os.Open(filename)
	if err != nil {
		return "", fmt.Errorf("could not open file: %v", err)
	}
	defer file.Close()

	var content string
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		content += scanner.Text() + "\n" // Read each line and add a newline
	}
	if err := scanner.Err(); err != nil {
		return "", fmt.Errorf("error reading file: %v", err)
	}

	return content, nil
}

func main() {
	// Check command-line argument for .lec file
	if len(os.Args) < 2 {
		fmt.Println("Usage: ./logic_compiler <input>.lec")
		return
	}

	lecFile := os.Args[1]
	expression, err := ReadLECFile(lecFile)
	if err != nil {
		fmt.Println("Error reading .lec file:", err)
		return
	}

	// Evaluate the expression from the .lec file
	err = EvaluateExpression(expression)
	if err != nil {
		fmt.Println("Error evaluating expression:", err)
	} else {
		fmt.Println("Expression evaluated and steps written to output.txt")
	}
}

package main

import (
	"log"
	"net/http"
	"os"

	"lps/api"
	"lps/jobs"
)

func main() {
	// Path to the C++ compute binary — override with LPS_BINARY env var
	binaryPath := "../lps"
	if p := os.Getenv("LPS_BINARY"); p != "" {
		binaryPath = p
	}

	port := "9000"
	if p := os.Getenv("PORT"); p != "" {
		port = p
	}

	manager := jobs.NewManager(binaryPath)
	router := api.NewRouter(manager)

	log.Printf("LPS server on http://0.0.0.0:%s  (binary: %s)\n", port, binaryPath)
	if err := http.ListenAndServe(":"+port, router); err != nil {
		log.Fatal(err)
	}
}

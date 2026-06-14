package jobs

import (
	"bufio"
	"bytes"
	"fmt"
	"log"
	"os/exec"
	"path/filepath"
	"strconv"
	"time"
)

func (m *Manager) run(job *Job) {
	job.mu.Lock()
	job.status = StatusRunning
	job.mu.Unlock()

	absPath, err := filepath.Abs(m.binaryPath)
	if err != nil {
		m.finishJob(job, fmt.Errorf("resolve binary path: %w", err), false)
		return
	}

	cmd := exec.Command(absPath,
		"--excitation", job.Config.ExcitationType,
		"--steps", strconv.Itoa(job.Config.TotalSteps))
	// Run from the binary's directory so relative shader paths resolve correctly
	cmd.Dir = filepath.Dir(absPath)

	stdout, err := cmd.StdoutPipe()
	if err != nil {
		m.finishJob(job, fmt.Errorf("stdout pipe: %w", err), false)
		return
	}
	var stderrBuf bytes.Buffer
	cmd.Stderr = &stderrBuf

	if err := cmd.Start(); err != nil {
		m.finishJob(job, fmt.Errorf("start: %w", err), false)
		return
	}

	// Kill the process if cancel is requested
	doneCh := make(chan struct{})
	go func() {
		select {
		case <-job.cancelCh:
			if cmd.Process != nil {
				cmd.Process.Kill()
			}
		case <-doneCh:
		}
	}()

	// Read readout values line-by-line as they stream in
	scanner := bufio.NewScanner(stdout)
	step := 0
	total := job.Config.TotalSteps
	for scanner.Scan() {
		val, err := strconv.ParseFloat(scanner.Text(), 32)
		if err != nil {
			continue
		}
		job.mu.Lock()
		job.results = append(job.results, float32(val))
		job.currentStep = step + 1
		if total > 0 {
			job.progress = float64(step+1) / float64(total)
		}
		job.mu.Unlock()
		step++
	}

	close(doneCh)
	cmdErr := cmd.Wait()
	if stderrBuf.Len() > 0 {
		log.Printf("[%s] stderr: %s", job.ID, stderrBuf.String())
	}

	// Was cancellation requested?
	cancelled := false
	select {
	case <-job.cancelCh:
		cancelled = true
	default:
	}

	m.finishJob(job, cmdErr, cancelled)
	log.Printf("[%s] finished (cancelled=%v err=%v)\n", job.ID, cancelled, cmdErr)
}

func (m *Manager) finishJob(job *Job, err error, cancelled bool) {
	now := time.Now()
	job.mu.Lock()
	defer job.mu.Unlock()
	job.finishedAt = &now
	switch {
	case cancelled:
		job.status = StatusCancelled
	case err != nil:
		job.status = StatusFailed
		job.errMsg = err.Error()
	default:
		job.status = StatusCompleted
	}
}

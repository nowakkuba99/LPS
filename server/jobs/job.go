package jobs

import (
	"sync"
	"time"
)

type Status string

const (
	StatusQueued    Status = "queued"
	StatusRunning   Status = "running"
	StatusCompleted Status = "completed"
	StatusFailed    Status = "failed"
	StatusCancelled Status = "cancelled"
)

type Config struct {
	ExcitationType string `json:"excitation_type"`
	TotalSteps     int    `json:"total_steps"`
}

// Job holds state for one simulation run. All exported fields are safe to read
// via View(); never read them directly across goroutines.
type Job struct {
	ID         string
	Config     Config
	CreatedAt  time.Time

	mu          sync.RWMutex
	status      Status
	currentStep int
	progress    float64
	results     []float32
	errMsg      string
	finishedAt  *time.Time

	cancelOnce sync.Once
	cancelCh   chan struct{}
}

func newJob(id string, cfg Config) *Job {
	return &Job{
		ID:        id,
		Config:    cfg,
		CreatedAt: time.Now(),
		status:    StatusQueued,
		results:   make([]float32, 0, cfg.TotalSteps),
		cancelCh:  make(chan struct{}),
	}
}

// Cancel signals the subprocess to be killed. Safe to call multiple times.
func (j *Job) Cancel() {
	j.cancelOnce.Do(func() { close(j.cancelCh) })
}

// JobView is a serialisable snapshot of a job's state.
type JobView struct {
	ID          string     `json:"id"`
	Status      Status     `json:"status"`
	CurrentStep int        `json:"current_step"`
	TotalSteps  int        `json:"total_steps"`
	Progress    float64    `json:"progress"`
	Config      Config     `json:"config"`
	Error       string     `json:"error,omitempty"`
	CreatedAt   time.Time  `json:"created_at"`
	FinishedAt  *time.Time `json:"finished_at,omitempty"`
	Results     []float32  `json:"results,omitempty"`
}

// View returns a thread-safe snapshot. Pass includeResults=true only when the
// job is completed (results slice can be large).
func (j *Job) View(includeResults bool) JobView {
	j.mu.RLock()
	defer j.mu.RUnlock()

	v := JobView{
		ID:          j.ID,
		Status:      j.status,
		CurrentStep: j.currentStep,
		TotalSteps:  j.Config.TotalSteps,
		Progress:    j.progress,
		Config:      j.Config,
		Error:       j.errMsg,
		CreatedAt:   j.CreatedAt,
		FinishedAt:  j.finishedAt,
	}
	if includeResults && j.status == StatusCompleted {
		v.Results = make([]float32, len(j.results))
		copy(v.Results, j.results)
	}
	return v
}
